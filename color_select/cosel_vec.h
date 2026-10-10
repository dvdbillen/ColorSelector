// SPDX-License-Identifier: MIT
// Copyright (c) 2026 David Billen

// Vector OPS
// The vec namespace contains vector ops that might be candidates for  optimization

#ifndef cosel_vec_h
#define cosel_vec_h

#include <cmath>

#include "cosel_math.h"
#include "cosel_portable.h"

namespace cosel::vec {

using in = const double*;
using out = double*;

inline constexpr void set(out NOALIAS a, double n)  noexcept {
    a[0] = a[1] = a[2] = n;
    a[3] = 0;
}
inline constexpr void set(out NOALIAS a, in NOALIAS b) noexcept {
    a[0] = b[0];
    a[1] = b[1];
    a[2] = b[2];
    a[3] = 0;
}
inline constexpr void clear(out NOALIAS a) noexcept {
    a[0] = a[1] = a[2] = a[3] = 0.0;
}
inline constexpr bool compare(in NOALIAS a, in NOALIAS b) noexcept {
    return cosel::compare(a[0], b[0], zero_epsilon) &&
           cosel::compare(a[1], b[1], zero_epsilon) &&
           cosel::compare(a[2], b[2], zero_epsilon);
}
inline constexpr void add(out NOALIAS a, in NOALIAS b) noexcept {
    a[0] += b[0];
    a[1] += b[1];
    a[2] += b[2];
}
inline constexpr void sub(out NOALIAS a, in NOALIAS b) noexcept {
    a[0] -= b[0];
    a[1] -= b[1];
    a[2] -= b[2];
}
inline constexpr void mul(out NOALIAS a, in NOALIAS b) noexcept {
    a[0] *= b[0];
    a[1] *= b[1];
    a[2] *= b[2];
}
inline constexpr void div(out NOALIAS a, in NOALIAS b) noexcept {
    a[0] /= b[0];
    a[1] /= b[1];
    a[2] /= b[2];
}
inline constexpr void add(out NOALIAS a, double b) noexcept {
    a[0] += b;
    a[1] += b;
    a[2] += b;
}
inline constexpr void sub(out NOALIAS a, double b) noexcept {
    a[0] -= b;
    a[1] -= b;
    a[2] -= b;
}
inline constexpr void mul(out NOALIAS a, double b) noexcept {
    a[0] *= b;
    a[1] *= b;
    a[2] *= b;
}
inline constexpr void div(out NOALIAS a, double b) noexcept {
    b = 1/b;
    a[0] *= b;
    a[1] *= b;
    a[2] *= b;
}
inline constexpr void interpolate(out NOALIAS a, in NOALIAS b, double n) noexcept {
    a[0] += (b[0] - a[0])*n;
    a[1] += (b[1] - a[1])*n;
    a[2] += (b[2] - a[2])*n;
}
inline constexpr void invert(out NOALIAS a, double n) noexcept {
    a[0] = n - a[0];
    a[1] = n - a[1];
    a[2] = n - a[2];
}
inline constexpr double dot(in NOALIAS a, in NOALIAS b) noexcept {
    return a[0]*b[0] + a[1]*b[1] + a[2]*b[2];
}
inline constexpr double distance_squared(in NOALIAS a, in NOALIAS b) noexcept {
    return (a[0] - b[0])*(a[0] - b[0]) + (a[1] - b[1])*(a[1] - b[1]) + (a[2] - b[2])*(a[2] - b[2]);
}
inline constexpr double delta_distance_squared(in NOALIAS a) noexcept {
    return a[0]*a[0] + a[1]*a[1] + a[2]*a[2];
}
inline double distance(in NOALIAS a, in NOALIAS b) noexcept {
    const auto n = distance_squared(a, b);
    return (n == 0)? 0: sqrt(n);
}
inline double delta_distance(in NOALIAS a) noexcept {
    const auto n = delta_distance_squared(a);
    return (n == 0)? 0: sqrt(n);
}
inline constexpr void crop(out NOALIAS a) noexcept {
    a[0] = std::clamp(a[0], 0.0, 1.0);
    a[1] = std::clamp(a[1], 0.0, 1.0);
    a[2] = std::clamp(a[2], 0.0, 1.0);
}
inline constexpr void crop_negative(out NOALIAS a) noexcept {
    a[0] = std::max<double>(a[0], 0);
    a[1] = std::max<double>(a[1], 0);
    a[2] = std::max<double>(a[2], 0);
}
inline constexpr void crop_positive(out NOALIAS a) noexcept {
    a[0] = std::min<double>(a[0], 1);
    a[1] = std::min<double>(a[1], 1);
    a[2] = std::min<double>(a[2], 1);
}
inline constexpr double min(in NOALIAS a) noexcept {
    return std::min(a[0], std::min(a[1], a[2]));
}
inline constexpr double max(in NOALIAS a) noexcept {
    return std::max(a[0], std::max(a[1], a[2]));
}

} // namespace cosel::vec


#endif // cosel_vec_h
