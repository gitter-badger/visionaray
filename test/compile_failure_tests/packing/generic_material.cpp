// This file is distributed under the MIT license.
// See the LICENSE file for details.

#include <array>

#include "visionaray/math/vector.h"
#include "visionaray/generic_material.h"
#include "visionaray/material.h"

using namespace visionaray;


int main()
{

#if defined GENMAT_PACK_FLOAT4

    std::array<generic_material<matte<float>,emissive<float>,mirror<float>>, 4> gm_array;
    simd::generic_material4<matte<float>,emissive<float>,mirror<float>> gm = simd::pack(gm_array);

#elif defined GENMAT_PACK_FLOAT8

#if VSNRAY_SIMD_ISA >= VSNRAY_SIMD_ISA_AVX
    std::array<generic_material<matte<float>,emissive<float>,mirror<float>>, 8> gm_array;
    simd::generic_material8<matte<float>,emissive<float>,mirror<float>> gm = simd::pack(gm_array);
#endif

#elif defined GENMAT_PACK_ILLEGAL_LENGTH_1

    std::array<generic_material<matte<float>,emissive<float>,mirror<float>>, 1> gm_array;
    auto gm = simd::pack(gm_array);

#elif defined GENMAT_PACK_ILLEGAL_LENGTH_3

    std::array<generic_material<matte<float>,emissive<float>,mirror<float>>, 3> gm_array;
    auto gm = simd::pack(gm_array);

#elif defined GENMAT_PACK_ILLEGAL_INTEGRAL

    std::array<generic_material<float>, 4> gm_array;
    auto gm = simd::pack(gm_array);

#elif defined GENMAT_UNPACK_FLOAT4

    simd::generic_material4<matte<float>,emissive<float>,mirror<float>> gm({});
    std::array<generic_material<matte<float>,emissive<float>,mirror<float>>, 4> gm_array = simd::unpack(gm);

#elif defined GENMAT_UNPACK_FLOAT8

#if VSNRAY_SIMD_ISA >= VSNRAY_SIMD_ISA_AVX
    simd::generic_material8<matte<float>,emissive<float>,mirror<float>> gm({});
    std::array<generic_material<matte<float>,emissive<float>,mirror<float>>, 8> gm_array = simd::unpack(gm);
#endif

#elif defined GENMAT_UNPACK_ILLEGAL_LENGTH

    simd::generic_material4<matte<float>,emissive<float>,mirror<float>> gm({});
    std::array<generic_material<matte<float>,emissive<float>,mirror<float>>, 8> gm_array = simd::unpack(gm);

#elif defined GENMAT_UNPACK_ILLEGAL_INTEGRAL

    simd::generic_material4<float> gm({});
    auto gm_array = simd::unpack(gm);

#elif defined GENMAT_UNPACK_SINGLE

    generic_material<matte<float>,emissive<float>,mirror<float>> gm({});
    auto gm_array = simd::unpack(gm);

#endif

    return 0;
}
