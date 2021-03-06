// This file is distributed under the MIT license.
// See the LICENSE file for details.

#pragma once

#ifndef VSNRAY_RANDOM_SAMPLER_H
#define VSNRAY_RANDOM_SAMPLER_H 1

#ifdef __CUDA_ARCH__
#include <thrust/random.h>
#else
#include <random>
#endif

#include <visionaray/math/simd/simd.h>

namespace visionaray
{

//-------------------------------------------------------------------------------------------------
// random_sampler classes, uses a standard pseudo RNG to generate samples
//

template <typename T>
class random_sampler
{
public:

    using value_type = T;

public:

#ifdef __CUDA_ARCH__
    typedef thrust::default_random_engine rand_engine;
    typedef thrust::uniform_real_distribution<T> uniform_dist;
#else
    typedef std::default_random_engine rand_engine;
    typedef std::uniform_real_distribution<T> uniform_dist;
#endif

    VSNRAY_FUNC random_sampler() = default;

    VSNRAY_FUNC random_sampler(unsigned seed)
        : rng_(rand_engine(seed))
        , dist_(uniform_dist(0, 1))
    {
    }

    VSNRAY_FUNC T next()
    {
        return dist_(rng_);
    }

private:

    rand_engine  rng_;
    uniform_dist dist_;

};

template <>
class random_sampler<simd::float4>
{
public:

    using value_type = simd::float4;

public:

    typedef random_sampler<float> sampler_type;

    VSNRAY_CPU_FUNC random_sampler(unsigned seed)
        : sampler_(seed)
    {
    }

    VSNRAY_CPU_FUNC simd::float4 next()
    {
        return simd::float4(
                sampler_.next(),
                sampler_.next(),
                sampler_.next(),
                sampler_.next()
                );
    }

    // TODO: maybe don't have a random_sampler4 at all?
    sampler_type& get_sampler()
    {
        return sampler_;
    }

private:

    sampler_type sampler_;
};

#if VSNRAY_SIMD_ISA >= VSNRAY_SIMD_ISA_AVX
template <>
class random_sampler<simd::float8>
{
public:

    using value_type = simd::float8;

public:

    typedef random_sampler<float> sampler_type;

    VSNRAY_CPU_FUNC random_sampler(unsigned seed)
        : sampler_(seed)
    {
    }

    VSNRAY_CPU_FUNC simd::float8 next()
    {
        return simd::float8(
                sampler_.next(),
                sampler_.next(),
                sampler_.next(),
                sampler_.next(),
                sampler_.next(),
                sampler_.next(),
                sampler_.next(),
                sampler_.next()
                );
    }

    // TODO: maybe don't have a random_sampler4 at all?
    sampler_type& get_sampler()
    {
        return sampler_;
    }

private:

    sampler_type sampler_;
};
#endif

} // visionaray

#endif // VSNRAY_RANDOM_SAMPLER_H
