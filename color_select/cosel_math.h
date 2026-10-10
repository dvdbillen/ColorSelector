// SPDX-License-Identifier: MIT
// Copyright (c) 2026 David Billen

// General math helpers

#ifndef cosel_math_h
#define cosel_math_h

#include <cmath>
#include <numbers>
#include <stdexcept>

#include "cosel_portable.h"

namespace cosel {

constexpr double zero_epsilon = 5e-6;

// C++17 comptible test for floating point template type
template<class T> constexpr inline void check_float() {
    static_assert(std::is_floating_point<T>(),
                  "Arguments must be floating point");
};

// Check for nan or inf
template<class T> constexpr inline bool bad(T n) {
    check_float<T>();
    return (std::isnan(n) || std::isinf(n));
}

// Convert nan or inf to 0
template<class T> constexpr inline T fix(T n)  {
    check_float<T>();
    return bad(n)? 0: n;
}

// Compare to zero with epsilon
static inline constexpr bool iszero(double n) {
    return std::abs(n) < zero_epsilon;
}

// Compare to one with epsilon
static inline constexpr bool isone(double n) {
    return n >= 1 - zero_epsilon;
}

// Compare floats with epsilon. This does not default to the zero_epsilon default
template<typename T>
inline constexpr bool compare(T a, T b, T epsilon = std::numeric_limits<T>::epsilon()*1000) {
    check_float<T>();
    return std::abs(a - b) <= epsilon;
}

// Clamp a normalized float
template<typename T>
[[nodiscard]] static inline constexpr double clamp(T n) {
    check_float<T>();
    return std::clamp<T>(n, 0, 1);
}

// Linear interpolation
template <typename T>
inline constexpr T lerp(T src, T dst, T amount) {
    check_float<T>();
    const auto range = dst - src;

    return src + range*amount;
}

// Reverse a linear interpolation
template <typename T>
inline constexpr T unlerp(T src, T dst, T result) {
    check_float<T>();
    const auto range = dst - src;

    if(compare<T>(range, 0)) {
        return 0;
    }

    return (result - src)/range;
}

// Return PI as template type
template<typename T>
inline constexpr T pi() {
    check_float<T>();

    return T(3.1415926535897932384626433832795);
}

// Return pi as a double with less clutter
inline constexpr double pi() {
    return pi<double>();
}

// Return pi as a float with less clutter
inline constexpr float pif() {
    return pi<float>();
}

// Checked sqrt
template <typename T> inline T sqrtz(T _X) {
    check_float<T>();

    if(_X <= 0) {
        return _X;
    }

    return std::sqrt(_X);
}

// These are useful IMO for clarity (especially when n is an expression)
template<typename T>
inline constexpr T square(T n) {
    return n*n;
}
template<typename T>
inline constexpr T cube(T n) {
    return n*n*n;
}

// Handles negatives differently than fmod(). The result is circular.
template<typename T>
inline constexpr T modulus(T n, T range) {
    check_float<T>();
    return n - range*std::floor(n/range);
}

// This is like modulus but is easier to read and a little faster for the common 1.0 case
template<typename T>
inline constexpr T mod1(T n) {
    check_float<T>();
    return n - std::floor(n);
}

// Puts a signed radian into the range 0 to PI*2.
template<typename T>
inline constexpr T abs_radian(T r) {
    check_float<T>();
    return modulus(r, T(pi<T>()*2));
}

// Degrees to radians
template<typename T>
inline constexpr T radians(T degrees) {
    check_float<T>();
    constexpr T co = pi<T>()/180;
    return degrees*co;
}

// Radians to degrees
template<typename T>
inline constexpr T degrees(T radians) {
    check_float<T>();
    constexpr T co = 180/pi<T>();
    return radians*co;
}

}

#endif
