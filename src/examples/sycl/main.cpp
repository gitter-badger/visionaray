// This file is distributed under the MIT license.
// See the LICENSE file for details.

#include <memory>

#include <GL/glew.h>

#include <Support/CmdLine.h>
#include <Support/CmdLineUtil.h>

#include <visionaray/detail/platform.h>

#include <visionaray/bvh.h>
#include <visionaray/camera.h>
#include <visionaray/cpu_buffer_rt.h>
#include <visionaray/scheduler.h>
#include <visionaray/surface.h>
#include <visionaray/detail/sycl_sched.h> // TODO
#include <visionaray/scheduler.h>
#include <visionaray/traverse.h>

#include <common/manip/arcball_manipulator.h>
#include <common/manip/pan_manipulator.h>
#include <common/manip/zoom_manipulator.h>

#include <common/model.h>
#include <common/obj_loader.h>
#include <common/timer.h>
#include <common/viewer_glut.h>

using namespace visionaray;

using viewer_type = viewer_glut;


//-------------------------------------------------------------------------------------------------
// struct with state variables
//

struct renderer : viewer_type
{
    using ray_type = basic_ray<float>;

    using host_bvh_type   = index_bvh<model::triangle_list::value_type>;
    using device_bvh_type = sycl_index_bvh<model::triangle_list::value_type>;

    renderer()
        : viewer_type(512, 512, "Visionaray SYCL Example")
    {
        namespace CL = support::cl;

        add_cmdline_option( CL::makeOption<std::string&>(
            CL::Parser<>(),
            "filename",
            CL::Desc("Input file in wavefront obj format"),
            CL::Positional,
            CL::Required,
            CL::init(this->filename)
            ) );
    }

    camera                                      cam;
    cpu_buffer_rt<PF_RGBA32F, PF_UNSPECIFIED>   sycl_rt;
    sycl_sched<ray_type>                        sched;

    std::string                                 filename;

    model mod;
    host_bvh_type                               host_bvh;
    device_bvh_type                             device_bvh;
    unsigned                                    frame_num       = 0;

protected:

    void on_display();
    void on_mouse_move(visionaray::mouse_event const& event);
    void on_resize(int w, int h);

};


//-------------------------------------------------------------------------------------------------
// Display function, implements the AO kernel
//

void renderer::on_display()
{
    // some setup

    using R = renderer::ray_type;
    using S = R::scalar_type;
    using C = vector<4, S>;
    using V = vector<3, S>;

    auto sparams = make_sched_params(
            pixel_sampler::jittered_blend_type{},
            cam,
            sycl_rt
            );


    using bvh_ref = device_bvh_type::bvh_ref;

    std::vector<bvh_ref> bvhs;
    bvhs.push_back(device_bvh.ref());

    auto prims_begin = bvhs.data();
    auto prims_end   = bvhs.data() + bvhs.size();

    auto bgcolor = background_color();

    sched.frame([&](R ray, sampler<S>& samp) -> result_record<S>
    {
        result_record<S> result;
        result.color = C(bgcolor, 1.0f);

        auto hit_rec = closest_hit(
                ray,
                prims_begin,
                prims_end
                );

        result.hit = hit_rec.hit;

        if (any(hit_rec.hit))
        {
            hit_rec.isect_pos = ray.ori + ray.dir * hit_rec.t;
            result.isect_pos  = hit_rec.isect_pos;

            C clr(1.0);

            auto n = get_normal(mod.normals.data(), hit_rec, normals_per_face_binding());

            // Make an ortho basis (TODO: move to library)
            auto w = n;
            auto v = select(
                    abs(w.x) > abs(w.y),
                    normalize( V(-w.z, S(0.0), w.x) ),
                    normalize( V(S(0.0), w.z, -w.y) )
                    );
            auto u = cross(v, w);

            static const int AO_Samples = 8;
            S radius(0.1);


            for (int i = 0; i < AO_Samples; ++i)
            {
                auto sp = cosine_sample_hemisphere(samp.next(), samp.next());

                auto dir = normalize( sp.x * u + sp.y * v + sp.z * w );

                R ao_ray;
                ao_ray.ori = hit_rec.isect_pos + dir * S(1E-3f);
                ao_ray.dir = dir;

                auto ao_rec = any_hit(
                        ao_ray,
                        prims_begin,
                        prims_end,
                        R::scalar_type(radius)
                        );

                clr = select(
                        ao_rec.hit,
                        clr - S(1.0f / AO_Samples),
                        clr
                        );
            }

            result.color      = select( hit_rec.hit, C(clr.xyz(), S(1.0)), result.color );

        }

        return result;
    }, sparams, ++frame_num);


    // display the rendered image

    glClearColor(bgcolor.x, bgcolor.y, bgcolor.z, 1.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    sycl_rt.display_color_buffer();
}


//-------------------------------------------------------------------------------------------------
// mouse handling
//

void renderer::on_mouse_move(visionaray::mouse_event const& event)
{
    if (event.get_buttons() != mouse::NoButton)
    {
        frame_num = 0;
    }

    viewer_type::on_mouse_move(event);
}


//-------------------------------------------------------------------------------------------------
// resize event
//

void renderer::on_resize(int w, int h)
{
    frame_num = 0;

    cam.set_viewport(0, 0, w, h);
    float aspect = w / static_cast<float>(h);
    cam.perspective(45.0f * constants::degrees_to_radians<float>(), aspect, 0.001f, 1000.0f);
    sycl_rt.resize(w, h);

    viewer_type::on_resize(w, h);
}


//-------------------------------------------------------------------------------------------------
// Main function, performs initialization
//

int main(int argc, char** argv)
{
    renderer rend;

    try
    {
        rend.init(argc, argv);
    }
    catch (std::exception& e)
    {
        std::cerr << e.what() << '\n';
        return EXIT_FAILURE;
    }

    try
    {
        visionaray::load_obj(rend.filename, rend.mod);
    }
    catch (std::exception& e)
    {
        std::cerr << "Failed loading obj model: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    std::cout << "Creating BVH...\n";

    rend.host_bvh = build<index_bvh<model::triangle_list::value_type>>(
            rend.mod.primitives.data(),
            rend.mod.primitives.size()
            );

    std::cout << "Ready\n";


    // Copy data to GPU
    rend.device_bvh = renderer::device_bvh_type(rend.host_bvh);
/*    rend.device_normals = rend.mod.normals;
    rend.device_materials = rend.mod.materials;*/


    float aspect = rend.width() / static_cast<float>(rend.height());

    rend.cam.perspective(45.0f * constants::degrees_to_radians<float>(), aspect, 0.001f, 1000.0f);
    rend.cam.view_all( rend.mod.bbox );

    rend.add_manipulator( std::make_shared<arcball_manipulator>(rend.cam, mouse::Left) );
    rend.add_manipulator( std::make_shared<pan_manipulator>(rend.cam, mouse::Middle) );
    // Additional "Alt + LMB" pan manipulator for setups w/o middle mouse button
    rend.add_manipulator( std::make_shared<pan_manipulator>(rend.cam, mouse::Left, keyboard::Alt) );
    rend.add_manipulator( std::make_shared<zoom_manipulator>(rend.cam, mouse::Right) );

    rend.event_loop();
}
