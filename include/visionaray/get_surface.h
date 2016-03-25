// This file is distributed under the MIT license.
// See the LICENSE file for details.

#pragma once

#ifndef VSNRAY_GET_SURFACE_H
#define VSNRAY_GET_SURFACE_H 1

#include <array>
#include <iterator>
#include <type_traits>
#include <utility>

#include "generic_material.h"
#include "generic_primitive.h"
#include "get_color.h"
#include "get_normal.h"
#include "get_tex_coord.h"
#include "prim_traits.h"
#include "surface.h"
#include "tags.h"

namespace visionaray
{
namespace detail
{

//-------------------------------------------------------------------------------------------------
// Helper functions
//

// deduce surface type from params ------------------------

template <typename ...Args>
struct decl_surface;

template <
    typename Normals,
    typename Materials
    >
struct decl_surface<Normals, Materials>
{
    using type = surface<
        typename std::iterator_traits<Normals>::value_type,
        typename std::iterator_traits<Materials>::value_type
        >;
};

template <
    typename Normals,
    typename Materials,
    typename DiffuseColor
    >
struct decl_surface<Normals, Materials, DiffuseColor>
{
    using type = surface<
        typename std::iterator_traits<Normals>::value_type,
        typename std::iterator_traits<Materials>::value_type,
        DiffuseColor
        >;
};


// identify accelerators ----------------------------------

template <typename T>
struct primitive_traits
{
    using type = T;
};

template <template <typename> class Accelerator, typename T>
struct primitive_traits<Accelerator<T>>
{
    using type = T;
};


// TODO: consolidate the following with get_normal()?
// TODO: consolidate interface with get_color() and get_tex_coord()?

// dispatch function for get_normal() ---------------------

// overload w/ precalculated normals
template <
    typename Primitives,
    typename Normals,
    typename HR,
    typename Primitive,
    typename NormalBinding,
    typename = typename std::enable_if<num_normals<Primitive, NormalBinding>::value != 0>::type
    >
VSNRAY_FUNC
inline auto get_normal_dispatch(
        Primitives      primitives,
        Normals         normals,
        HR const&       hr,
        Primitive       /* */,
        NormalBinding   /* */
        )
    -> decltype( get_normal(normals, hr, Primitive{}, NormalBinding{}) )
{
    VSNRAY_UNUSED(primitives);

    return get_normal(normals, hr, Primitive{}, NormalBinding{});
}

// overload w/o precalculated normals
template <
    typename Primitives,
    typename Normals,
    typename HR,
    typename Primitive,
    typename = typename std::enable_if<num_normals<Primitive, unspecified_binding>::value == 0>::type
    >
VSNRAY_FUNC
inline auto get_normal_dispatch(
        Primitives      primitives,
        Normals         normals,
        HR const&       hr,
        Primitive       /* */
        )
    -> decltype( get_normal(hr, primitives[hr.prim_id]) )
{
    VSNRAY_UNUSED(normals);

    return get_normal(hr, primitives[hr.prim_id]);
}

// helper
template <typename NormalBinding, typename Normals, typename HR>
class get_normal_from_generic_primitive_visitor
{
public:

    using return_type = vector<3, float>;

public:

    VSNRAY_FUNC
    get_normal_from_generic_primitive_visitor(
            Normals     normals,
            HR const&   hr
            )
        : normals_(normals)
        , hr_(hr)
    {
    }

    // overload w/ precalculated normals
    template <typename Primitive>
    VSNRAY_FUNC
    return_type operator()(
            Primitive const& primitive,
            typename std::enable_if<num_normals<Primitive, NormalBinding>::value>::type* = 0
            ) const
    {
        VSNRAY_UNUSED(primitive);

        return get_normal(normals_, hr_, Primitive{}, NormalBinding{});
    }

    // overload w/o precalculated normals
    template <typename Primitive>
    VSNRAY_FUNC
    return_type operator()(
            Primitive const& primitive,
            typename std::enable_if<num_normals<Primitive, NormalBinding>::value == 0>::type* = 0
            ) const
    {
        return get_normal(hr_, primitive);
    }

private:

    Normals     normals_;
    HR const&   hr_;

};

// overload for generic_primitive
template <
    typename Primitives,
    typename Normals,
    typename HR,
    typename ...Ts,
    typename NormalBinding
    >
VSNRAY_FUNC
inline auto get_normal_dispatch(
        Primitives                  primitives,
        Normals                     normals,
        HR const&                   hr,
        generic_primitive<Ts...>    /* */,
        NormalBinding               /* */
        )
    -> typename std::iterator_traits<Normals>::value_type
{
    get_normal_from_generic_primitive_visitor<NormalBinding, Normals, HR> visitor(
            normals,
            hr
            );

    return apply_visitor( visitor, primitives[hr.prim_id] );
}


//-------------------------------------------------------------------------------------------------
//
//

template <
    typename Primitive,
    typename HR,
    typename Primitives,
    typename Materials
    >
VSNRAY_FUNC
inline auto get_surface_impl(
        has_no_normals_tag  /* */,
        has_no_colors_tag   /* */,
        has_no_textures_tag /* */,
        Primitive           /* */,
        HR const&           hr,
        Primitives          primitives,
        Materials           materials
        )
    -> typename decl_surface<vector<3, float>*, Materials>::type
{
    return make_surface(
            get_normal(hr, primitives[hr.prim_id]),
            materials[hr.geom_id]
            );
}

template <
    typename Primitive,
    typename NormalBinding,
    typename HR,
    typename Primitives,
    typename Normals,
    typename Materials
    >
VSNRAY_FUNC
inline auto get_surface_impl(
        has_normals_tag     /* */,
        has_no_colors_tag   /* */,
        has_no_textures_tag /* */,
        Primitive           /* */,
        NormalBinding       /* */,
        HR const&           hr,
        Primitives          primitives,
        Normals             normals,
        Materials           materials
        )
    -> typename decl_surface<Normals, Materials>::type
{
    return make_surface(
            get_normal_dispatch(primitives, normals, hr, Primitive{}, NormalBinding{}),
            materials[hr.geom_id]
            );
}

template <
    typename Primitive,
    typename NormalBinding,
    typename HR,
    typename Primitives,
    typename Normals,
    typename TexCoords,
    typename Materials,
    typename Textures
    >
VSNRAY_FUNC
inline auto get_surface_impl(
        has_normals_tag     /* */,
        has_no_colors_tag   /* */,
        has_textures_tag    /* */,
        Primitive           /* */,
        NormalBinding       /* */,
        HR const&           hr,
        Primitives          primitives,
        Normals             normals,
        TexCoords           tex_coords,
        Materials           materials,
        Textures            textures
        )
    -> typename decl_surface<Normals, Materials, vector<3, float>>::type
{
    using P = typename primitive_traits<Primitive>::type;
    using C = vector<3, float>;

    auto tc = get_tex_coord(tex_coords, hr, P{});

    auto const& tex = textures[hr.geom_id];
    auto tex_color = tex.width() > 0 && tex.height() > 0
                   ? C(visionaray::tex2D(tex, tc))
                   : C(1.0);

    auto normal = get_normal_dispatch(primitives, normals, hr, P{}, NormalBinding{});
    return make_surface( normal, materials[hr.geom_id], tex_color );
}

template <
    typename Primitive,
    typename ColorBinding,
    typename HR,
    typename Primitives,
    typename TexCoords,
    typename Materials,
    typename Colors,
    typename Textures
    >
VSNRAY_FUNC
inline auto get_surface_impl(
        has_no_normals_tag  /* */,
        has_colors_tag      /* */,
        has_textures_tag    /* */,
        Primitive           /* */,
        ColorBinding        /* */,
        HR const&           hr,
        Primitives          primitives,
        TexCoords           tex_coords,
        Materials           materials,
        Colors              colors,
        Textures            textures
        )
    -> typename decl_surface<vector<3, float>*, Materials, vector<3, float>>::type
{
    using P = typename primitive_traits<Primitive>::type;
    using C = vector<3, float>;

    auto color = get_color(colors, hr, P{}, ColorBinding{});
    auto tc = get_tex_coord(tex_coords, hr, P{});

    auto const& tex = textures[hr.geom_id];
    auto tex_color = tex.width() > 0 && tex.height() > 0
                   ? C(visionaray::tex2D(tex, tc))
                   : C(1.0);

    auto normal = get_normal(hr, primitives[hr.prim_id]);
    return make_surface( normal, materials[hr.geom_id], color * tex_color );
}

template <
    typename Primitive,
    typename NormalBinding,
    typename ColorBinding,
    typename HR,
    typename Primitives,
    typename Normals,
    typename TexCoords,
    typename Materials,
    typename Colors,
    typename Textures
    >
VSNRAY_FUNC
inline auto get_surface_impl(
        has_normals_tag     /* */,
        has_colors_tag      /* */,
        has_textures_tag    /* */,
        Primitive           /* */,
        NormalBinding       /* */,
        ColorBinding        /* */,
        HR const&           hr,
        Primitives          primitives,
        Normals             normals,
        TexCoords           tex_coords,
        Materials           materials,
        Colors              colors,
        Textures            textures
        )
    -> typename decl_surface<Normals, Materials, vector<3, float>>::type
{
    using P = typename primitive_traits<Primitive>::type;
    using C = vector<3, float>;

    auto color = get_color(colors, hr, P{}, ColorBinding{});
    auto tc = get_tex_coord(tex_coords, hr, P{});

    auto const& tex = textures[hr.geom_id];
    auto tex_color = tex.width() > 0 && tex.height() > 0
                   ? C(visionaray::tex2D(tex, tc))
                   : C(1.0);

    auto normal = get_normal_dispatch(primitives, normals, hr, P{}, NormalBinding{});
    return make_surface( normal, materials[hr.geom_id], color * tex_color );
}


//-------------------------------------------------------------------------------------------------
// SIMD
//

template <
    typename Primitive,
    template <typename, typename> class HR,
    typename T,
    typename HRP,
    typename Primitives,
    typename Materials,
    typename = typename std::enable_if<simd::is_simd_vector<T>::value>::type
    >
inline auto get_surface_impl(
        has_no_normals_tag              /* */,
        has_no_colors_tag               /* */,
        has_no_textures_tag             /* */,
        Primitive                       /* */,
        HR<basic_ray<T>, HRP> const&    hr,
        Primitives                      primitives,
        Materials                       materials
        ) -> decltype( simd::pack(
            std::declval<std::array<
                typename decl_surface<vector<3, float>*, Materials>::type,
                simd::num_elements<T>::value
                >>()
            ) )
{
    using N = vector<3, float>;
    using M = typename std::iterator_traits<Materials>::value_type;

    auto hrs = unpack(hr);

    std::array<typename decl_surface<vec3*, Materials>::type, simd::num_elements<T>::value> surfs;

    for (int i = 0; i < simd::num_elements<T>::value; ++i)
    {
        surfs[i] = make_surface(
            hrs[i].hit ? get_normal(hrs[i], primitives[hrs[i].prim_id]) : N{},
            hrs[i].hit ? materials[hrs[i].geom_id]                      : M{}
            );
    }

    return simd::pack(surfs);
}

template <
    typename Primitive,
    typename ColorBinding,
    template <typename, typename> class HR,
    typename T,
    typename HRP,
    typename Primitives,
    typename TexCoords,
    typename Materials,
    typename Colors,
    typename Textures,
    typename = typename std::enable_if<simd::is_simd_vector<T>::value>::type
    >
inline auto get_surface_impl(
        has_no_normals_tag              /* */,
        has_colors_tag                  /* */,
        has_textures_tag                /* */,
        Primitive                       /* */,
        ColorBinding                    /* */,
        HR<basic_ray<T>, HRP> const&    hr,
        Primitives                      primitives,
        TexCoords                       tex_coords,
        Materials                       materials,
        Colors                          colors,
        Textures                        textures
        ) -> decltype( simd::pack(
            std::declval<std::array<
                typename decl_surface<vector<3, float>*, Materials, vector<3, float>>::type,
                simd::num_elements<T>::value
                >>()
            ) )
{
    using N = vector<3, float>;
    using M = typename std::iterator_traits<Materials>::value_type;
    using P = typename primitive_traits<Primitive>::type;
    using C = vector<3, float>;

    auto hrs = unpack(hr);

    auto colorss = get_color(colors, hrs, P{}, ColorBinding{});

    auto tcs = get_tex_coord(tex_coords, hrs, P{});

    std::array<typename decl_surface<vector<3, float>*, Materials, vector<3, float>>::type, simd::num_elements<T>::value> surfs;

    for (int i = 0; i < simd::num_elements<T>::value; ++i)
    {
        auto const& tex = textures[hrs[i].geom_id];
        C tex_color = hrs[i].hit && tex.width() > 0 && tex.height() > 0
                    ? C(visionaray::tex2D(tex, tcs[i]))
                    : C(1.0);

        surfs[i] = make_surface(
            hrs[i].hit ? get_normal(hrs[i], primitives[hrs[i].prim_id]) : N{},
            hrs[i].hit ? materials[hrs[i].geom_id]                      : M{},
            hrs[i].hit ? colorss[i] * tex_color                         : C(1.0)
            );
    }

    return simd::pack(surfs);
}


template <
    typename Primitive,
    typename NormalBinding,
    template <typename, typename> class HR,
    typename T,
    typename HRP,
    typename Primitives,
    typename Normals,
    typename Materials,
    typename = typename std::enable_if<simd::is_simd_vector<T>::value>::type
    >
inline auto get_surface_impl(
        has_normals_tag                 /* */,
        has_no_colors_tag               /* */,
        has_no_textures_tag             /* */,
        Primitive                       /* */,
        NormalBinding                   /* */,
        HR<basic_ray<T>, HRP> const&    hr,
        Primitives                      primitives,
        Normals                         normals,
        Materials                       materials
        ) -> decltype( simd::pack(
            std::declval<std::array<
                typename decl_surface<Normals, Materials>::type,
                simd::num_elements<T>::value
                >>()
            ) )
{
    using N = typename std::iterator_traits<Normals>::value_type;
    using M = typename std::iterator_traits<Materials>::value_type;
    using P = typename primitive_traits<Primitive>::type;

    auto hrs = unpack(hr);

    std::array<typename decl_surface<Normals, Materials>::type, simd::num_elements<T>::value> surfs;

    for (int i = 0; i < simd::num_elements<T>::value; ++i)
    {
        surfs[i] = make_surface(
            hrs[i].hit ? get_normal_dispatch(primitives, normals, hrs[i], P{}, NormalBinding{}) : N{},
            hrs[i].hit ? materials[hrs[i].geom_id]                                              : M{}
            );
    }

    return simd::pack(surfs);
}


template <
    typename Primitive,
    typename NormalBinding,
    template <typename, typename> class HR,
    typename T,
    typename HRP,
    typename Primitives,
    typename Normals,
    typename TexCoords,
    typename Materials,
    typename Textures,
    typename = typename std::enable_if<simd::is_simd_vector<T>::value>::type
    >
inline auto get_surface_impl(
        has_normals_tag                 /* */,
        has_no_colors_tag               /* */,
        has_textures_tag                /* */,
        Primitive                       /* */,
        NormalBinding                   /* */,
        HR<basic_ray<T>, HRP> const&    hr,
        Primitives                      primitives,
        Normals                         normals,
        TexCoords                       tex_coords,
        Materials                       materials,
        Textures                        textures
        ) -> decltype( simd::pack(
            std::declval<std::array<
                typename decl_surface<Normals, Materials, vector<3, float>>::type,
                simd::num_elements<T>::value
                >>()
            ) )
{
    using N = typename std::iterator_traits<Normals>::value_type;
    using M = typename std::iterator_traits<Materials>::value_type;
    using P = typename primitive_traits<Primitive>::type;
    using C = vector<3, float>;

    auto hrs = unpack(hr);

    auto tcs = get_tex_coord(tex_coords, hrs, typename primitive_traits<Primitive>::type{});

    std::array<typename detail::decl_surface<Normals, Materials, vector<3, float>>::type, simd::num_elements<T>::value> surfs;

    for (int i = 0; i < simd::num_elements<T>::value; ++i)
    {
        auto const& tex = textures[hrs[i].geom_id];
        C tex_color = hrs[i].hit && tex.width() > 0 && tex.height() > 0
                    ? C(visionaray::tex2D(tex, tcs[i]))
                    : C(1.0);

        surfs[i] = make_surface(
            hrs[i].hit ? get_normal_dispatch(primitives, normals, hrs[i], P{}, NormalBinding{}) : N{},
            hrs[i].hit ? materials[hrs[i].geom_id]                                              : M{},
            hrs[i].hit ? tex_color                                                              : C(1.0)
            );
    }

    return simd::pack(surfs);
}

template <
    typename Primitive,
    typename NormalBinding,
    typename ColorBinding,
    template <typename, typename> class HR,
    typename T,
    typename HRP,
    typename Primitives,
    typename Normals,
    typename TexCoords,
    typename Materials,
    typename Colors,
    typename Textures,
    typename = typename std::enable_if<simd::is_simd_vector<T>::value>::type
    >
inline auto get_surface_impl(
        has_normals_tag                 /* */,
        has_colors_tag                  /* */,
        has_textures_tag                /* */,
        Primitive                       /* */,
        NormalBinding                   /* */,
        ColorBinding                    /* */,
        HR<basic_ray<T>, HRP> const&    hr,
        Primitives                      primitives,
        Normals                         normals,
        TexCoords                       tex_coords,
        Materials                       materials,
        Colors                          colors,
        Textures                        textures
        ) -> decltype( simd::pack(
            std::declval<std::array<
                typename decl_surface<Normals, Materials, vector<3, float>>::type,
                simd::num_elements<T>::value
                >>()
            ) )
{
    using N = typename std::iterator_traits<Normals>::value_type;
    using M = typename std::iterator_traits<Materials>::value_type;
    using P = typename primitive_traits<Primitive>::type;
    using C = vector<3, float>;

    auto hrs = unpack(hr);

    auto colorss = get_color(colors, hrs, P{}, ColorBinding{});

    auto tcs = get_tex_coord(tex_coords, hrs, P{});

    std::array<typename decl_surface<Normals, Materials, vector<3, float>>::type, simd::num_elements<T>::value> surfs;

    for (int i = 0; i < simd::num_elements<T>::value; ++i)
    {
        auto const& tex = textures[hrs[i].geom_id];
        C tex_color = hrs[i].hit && tex.width() > 0 && tex.height() > 0
                    ? C(visionaray::tex2D(tex, tcs[i]))
                    : C(1.0);

        surfs[i] = make_surface(
            hrs[i].hit ? get_normal_dispatch(primitives, normals, hrs[i], P{}, NormalBinding{}) : N{},
            hrs[i].hit ? materials[hrs[i].geom_id]                                              : M{},
            hrs[i].hit ? colorss[i] * tex_color                                                 : C(1.0)
            );
    }

    return simd::pack(surfs);
}


//-------------------------------------------------------------------------------------------------
// Functions to deduce appropriate surface via parameter inspection
//

// w/o normals

template <typename HR, typename Params>
VSNRAY_FUNC
inline auto get_surface_unroll_params_impl(
        has_no_normals_tag,
        has_no_colors_tag,
        has_no_textures_tag,
        HR const& hr,
        Params const& p
        )
    -> decltype( get_surface_impl(
            has_no_normals_tag{},
            has_no_colors_tag{},
            has_no_textures_tag{},
            typename Params::primitive_type{},
            hr,
            p.prims.begin,
            p.materials
            ) )
{
    return get_surface_impl(
            has_no_normals_tag{},
            has_no_colors_tag{},
            has_no_textures_tag{},
            typename Params::primitive_type{},
            hr,
            p.prims.begin,
            p.materials
            );
}

template <typename HR, typename Params>
VSNRAY_FUNC
inline auto get_surface_unroll_params_impl(
        has_no_normals_tag,
        has_no_colors_tag,
        has_textures_tag,
        HR const& hr,
        Params const& p
        )
    -> decltype( get_surface_impl(
            has_no_normals_tag{},
            has_no_colors_tag{},
            has_textures_tag{},
            typename Params::primitive_type{},
            hr,
            p.prims.begin,
            p.tex_coords,
            p.materials,
            p.textures
            ) )
{
    return get_surface_impl(
            has_no_normals_tag{},
            has_no_colors_tag{},
            has_textures_tag{},
            typename Params::primitive_type{},
            hr,
            p.prims.begin,
            p.tex_coords,
            p.materials,
            p.textures
            );
}

template <typename HR, typename Params>
VSNRAY_FUNC
inline auto get_surface_unroll_params_impl(
        has_no_normals_tag,
        has_colors_tag,
        has_textures_tag,
        HR const& hr,
        Params const& p
        )
    -> decltype( get_surface_impl(
            has_no_normals_tag{},
            has_colors_tag{},
            has_textures_tag{},
            typename Params::primitive_type{},
            typename Params::color_binding{},
            hr,
            p.prims.begin,
            p.tex_coords,
            p.materials,
            p.colors,
            p.textures
            ) )
{
    return get_surface_impl(
            has_no_normals_tag{},
            has_colors_tag{},
            has_textures_tag{},
            typename Params::primitive_type{},
            typename Params::color_binding{},
            hr,
            p.prims.begin,
            p.tex_coords,
            p.materials,
            p.colors,
            p.textures
            );
}


// w/ normals

template <typename HR, typename Params>
VSNRAY_FUNC
inline auto get_surface_unroll_params_impl(
        has_normals_tag,
        has_no_colors_tag,
        has_no_textures_tag,
        HR const& hr,
        Params const& p
        )
    -> decltype( get_surface_impl(
            has_normals_tag{},
            has_no_colors_tag{},
            has_no_textures_tag{},
            typename Params::primitive_type{},
            typename Params::normal_binding{},
            hr,
            p.prims.begin,
            p.normals,
            p.materials
            ) )
{
    return get_surface_impl(
            has_normals_tag{},
            has_no_colors_tag{},
            has_no_textures_tag{},
            typename Params::primitive_type{},
            typename Params::normal_binding{},
            hr,
            p.prims.begin,
            p.normals,
            p.materials
            );
}

template <typename HR, typename Params>
VSNRAY_FUNC
inline auto get_surface_unroll_params_impl(
        has_normals_tag,
        has_no_colors_tag,
        has_textures_tag,
        HR const& hr,
        Params const& p
        )
    -> decltype( get_surface_impl(
            has_normals_tag{},
            has_no_colors_tag{},
            has_textures_tag{},
            typename Params::primitive_type{},
            typename Params::normal_binding{},
            hr,
            p.prims.begin,
            p.normals,
            p.tex_coords,
            p.materials,
            p.textures
            ) )
{
    return get_surface_impl(
            has_normals_tag{},
            has_no_colors_tag{},
            has_textures_tag{},
            typename Params::primitive_type{},
            typename Params::normal_binding{},
            hr,
            p.prims.begin,
            p.normals,
            p.tex_coords,
            p.materials,
            p.textures
            );
}

template <typename HR, typename Params>
VSNRAY_FUNC
inline auto get_surface_unroll_params_impl(
        has_normals_tag,
        has_colors_tag,
        has_textures_tag,
        HR const& hr,
        Params const& p
        )
    -> decltype( get_surface_impl(
            has_normals_tag{},
            has_colors_tag{},
            has_textures_tag{},
            typename Params::primitive_type{},
            typename Params::normal_binding{},
            typename Params::color_binding{},
            hr,
            p.prims.begin,
            p.normals,
            p.tex_coords,
            p.materials,
            p.colors,
            p.textures
            ) )
{
    return get_surface_impl(
            has_normals_tag{},
            has_colors_tag{},
            has_textures_tag{},
            typename Params::primitive_type{},
            typename Params::normal_binding{},
            typename Params::color_binding{},
            hr,
            p.prims.begin,
            p.normals,
            p.tex_coords,
            p.materials,
            p.colors,
            p.textures
            );
}

} // detail


template <typename HR, typename Params>
VSNRAY_FUNC
inline auto get_surface(HR const& hr, Params const& p)
    -> decltype( detail::get_surface_unroll_params_impl(
            detail::has_normals<Params>{},
            detail::has_colors<Params>{},
            detail::has_textures<Params>{},
            hr,
            p
            ) )
{
    return detail::get_surface_unroll_params_impl(
            detail::has_normals<Params>{},
            detail::has_colors<Params>{},
            detail::has_textures<Params>{},
            hr,
            p
            );
}

} // visionaray

#endif // VSNRAY_SURFACE_H