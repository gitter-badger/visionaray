// This file is distributed under the MIT license.
// See the LICENSE file for details.

#include <stdexcept>

#include <GL/glew.h>

#include <cuda_gl_interop.h>

#include <visionaray/gl/handle.h>

#include "../graphics_resource.h"


namespace visionaray
{
namespace cuda
{

struct pixel_pack_buffer::impl
{
    graphics_resource   resource;
    gl::buffer          buffer;
    recti               viewport;
    pixel_format        format      = PF_UNSPECIFIED;
};

pixel_pack_buffer::pixel_pack_buffer()
    : impl_(new impl())
{
    cudaError_t err = cudaSuccess;

    int dev = 0;
    cudaDeviceProp prop;
    err = cudaChooseDevice(&dev, &prop);
    if (err != cudaSuccess)
    {
        throw std::runtime_error("choose device");
    }

    err = cudaGLSetGLDevice(dev);

/*    if (err == cudaErrorSetOnActiveProcess)
    {
        err = cudaDeviceReset();
        err = cudaGLSetGLDevice(dev);
    }

    if (err != cudaSuccess)
    {
        throw std::runtime_error("set GL device");
    }*/
}

void pixel_pack_buffer::map(recti viewport, pixel_format format)
{
    auto info = map_pixel_format(format);

    if (impl_->viewport != viewport || impl_->format != format)
    {
        // GL buffer
        impl_->buffer.reset( gl::create_buffer() );
        glBindBuffer(GL_PIXEL_PACK_BUFFER, impl_->buffer.get());
        glBufferData(GL_PIXEL_PACK_BUFFER, viewport.w * viewport.h * info.size, 0, GL_STREAM_COPY);
        glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);

        // Register buffer object with CUDA
        impl_->resource.register_buffer(impl_->buffer.get(), cudaGraphicsRegisterFlagsReadOnly);

        // Update state
        impl_->viewport = viewport;
        impl_->format   = format;
    }

    // Transfer pixels
    glBindBuffer(GL_PIXEL_PACK_BUFFER, impl_->buffer.get());
    glReadPixels(
            viewport.x,
            viewport.y,
            viewport.w,
            viewport.h,
            info.format,
            info.type,
            0
            );
    glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);

    // Map graphics resource
    if (impl_->resource.map() == 0)
    {
        throw std::runtime_error("bad resource mapped");
    }
}

void pixel_pack_buffer::unmap()
{
    impl_->resource.unmap();
}

void const* pixel_pack_buffer::data() const
{
    return impl_->resource.dev_ptr();
}

} // cuda
} // visionaray
