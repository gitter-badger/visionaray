// This file is distributed under the MIT license.
// See the LICENSE file for details.

namespace MATH_NAMESPACE
{
namespace detail
{

//-------------------------------------------------------------------------------------------------
// Convert float to unorm (cf. OpenGL 4.4, 2.3.4.1
//

template <unsigned Bits>
MATH_FUNC
inline uint32_t float_to_unorm(float f)
{
    f = saturate(f);
    return static_cast<uint32_t>(f * ((1ULL << Bits) - 1));
}

template <unsigned Bits>
MATH_FUNC
inline float unorm_to_float(uint32_t u)
{
    return static_cast<float>(u) / ((1ULL << Bits) - 1);
}

} // detail


//-------------------------------------------------------------------------------------------------
// unorm members
//

template <unsigned Bits>
MATH_FUNC
unorm<Bits>::unorm(float f)
    : value(detail::float_to_unorm<Bits>(f))
{
}

template <unsigned Bits>
MATH_FUNC
unorm<Bits>::operator value_type() const
{
    return value;
}

template <unsigned Bits>
MATH_FUNC
unorm<Bits>::operator float() const
{
    return detail::unorm_to_float<Bits>(value);
}

} // MATH_NAMESPACE
