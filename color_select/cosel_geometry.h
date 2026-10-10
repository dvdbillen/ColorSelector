// SPDX-License-Identifier: MIT
// Copyright (c) 2026 David Billen

// Non-Qt Point class.

#ifndef cosel_geometry_h
#define cosel_geometry_h

#include <algorithm>
#include <complex>
#include <cmath>
#include <numeric>

#include "cosel_portable.h"
#include "cosel_math.h"

namespace cosel {

template <typename T> class point {
public:
    using value_type = T;

    constexpr point()=default;
    constexpr explicit point(T x, T y) noexcept : x_(x), y_(y) {}
    template <typename U> constexpr explicit point(const point<U>& other) noexcept {
        if constexpr(!std::is_floating_point<T>() && std::is_floating_point<U>()) {
            x_ = static_cast<U>(std::round(other.x()));
            y_ = static_cast<U>(std::round(other.y()));
        } else {
            x_ = static_cast<U>(other.x());
            y_ = static_cast<U>(other.y());
        }
    }
    template <typename U> constexpr explicit operator point<U>() const noexcept {
        return point<U>(*this);
    }
    // Conversion to/from complex can be implied
    point(const std::complex<T>& cplx) noexcept : x_(cplx.real()), y_(cplx.imag()) {}
    operator std::complex<T>() const {
        return std::complex<T>(x_, y_);
    }

    T x() const {
        return x_;
    }
    T y() const {
        return y_;
    }
    void set_x(T n) {
        x_ = n;
    }
    void set_y(T n) {
        y_ = n;
    }
    void set(T x, T y) {
        x_ = x;
        y_ = y;
    }

    [[nodiscard]] bool zero() const {
        return x_ == 0 && y_ == 0;
    }
    [[nodiscard]] bool is_positive() const {
        return x_ >= 0 && y_ >= 0;
    }
    bool compare(const point<T>& other, T epsilon) const {
        return cosel::compare(x_, other.x_, epsilon) &&
               cosel::compare(y_, other.y_, epsilon);
    }

    [[nodiscard]] point<T> operator+(const point<T>& other) const {
        return point<T>(x_ + other.x_, y_ + other.y_);
    }
    [[nodiscard]] point<T> operator+(T n) const {
        return point<T>(x_ + n, y_ + n);
    }
    [[nodiscard]] point<T> operator-(const point<T>& other) const {
        return point<T>(x_ - other.x_, y_ - other.y_);
    }
    [[nodiscard]] point<T> operator-(T n) const {
        return point<T>(x_ - n, y_ - n);
    }
    [[nodiscard]] point<T> operator*(const point<T>& other) const {
        return point<T>(x_ * other.x_, y_ * other.y_);
    }
    [[nodiscard]] point<T> operator*(T n) const {
        return point<T>(x_ * n, y_ * n);
    }
    [[nodiscard]] point<T> operator/(const point<T>& other) const {
        return point<T>(x_ / other.x_, y_ / other.y_);
    }
    [[nodiscard]] point<T> operator/(T n) const {
        return point<T>(x_ / n, y_ / n);
    }
    void operator +=(const point<T>& other) {
        *this = *this + other;
    }
    void operator -=(const point<T>& other) {
        *this = *this - other;
    }
    void operator *=(const point<T>& other) {
        *this = *this * other;
    }
    bool operator==(const point<T>& other) const {
        return x_ == other.x_ && y_ == other.y_;
    }
    bool operator!=(const point<T>& other) const {
        return x_ != other.x_ && y_ != other.y_;
    }
    void operator +=(T n) {
        *this = *this + n;
    }
    void operator -=(T n) {
        *this = *this - n;
    }
    void operator *=(T n) {
        *this = *this * n;
    }

    [[nodiscard]] point<T> with_x(T x) const {
        return point<T>(x, y_);
    }
    [[nodiscard]] point<T> with_y(T y) const {
        return point<T>(x_, y);
    }
    [[nodiscard]] T distance_squared() const {
        return x_*x_ + y_*y_;
    }
    [[nodiscard]] T distance() const {
        require_float();
        const auto sq = distance_squared();
        return (sq > 0)? std::sqrt(sq): 0;
    }
    [[nodiscard]] T distance_squared(const point<T>& other) const {
        return point(*this - other).distance_squared();
    }
    [[nodiscard]] T distance(const  point<T>& other) const {
        return point(*this - other).distance();
    }
    [[nodiscard]] point<T> normalized() const {
        require_float();
        auto n = distance_squared();

        if(n <= 0) {
            return point<T>(1, 0);
        }

        n = 1/std::sqrt(n);
        return point<T>(x_*n, y_*n);
    }
    [[nodiscard]] point<T> perpendicular() const {
        return point<T>(-y_, x_);
    }
    // Complex multiply
    [[nodiscard]] point<T> cmulled(const point<T>& other) const {
        require_float();
        return point<T>(x_*other.x_ - y_*other.y_, x_*other.y_ + y_*other.x_);
    }
    [[nodiscard]] T dot(const point<T>& other) const {
        return x_*other.x_ + y_*other.y_;
    }
    [[nodiscard]] T cross(const point<T>& other) const {
        return x_*other.y_ - y_*other.x_;
    }
    // Determinate
    [[nodiscard]] T det(const point<T>& other) const {
        return (other.x_ - x_)*(other.y_ + y_);
    }
    [[nodiscard]] T sum() const {
        return x_ + y_;
    }
    [[nodiscard]] point<T> interpolated(const point<T>& other, T amt) const {
        require_float();
        return point<T>(static_cast<T>(x_ + (other.x_ - x_)*amt),
                        static_cast<T>(y_ + (other.y_ - y_)*amt));
    }
    [[nodiscard]] T angle(const point<T>& other) const {
        require_float();
        return std::atan2(other.y_ - y_, other.x_ - x_);
    }
    [[nodiscard]] T angle() const {
        require_float();
        return std::atan2(y_, x_);
    }
    [[nodiscard]] point<T> rotated(T radians) const {
        require_float();
        return cmulled(point<T>(std::cos(radians), std::sin(radians)));
    }
    [[nodiscard]] point<T> rotated_around(const point<T>& other, T radians) const {
        return (*this - other).rotated(radians) + other;
    }
    [[nodiscard]] point<T> offseted(T radians, T distance) const {
        require_float();
        return *this + point<T>(std::cos(radians), std::sin(radians))*distance;
    }
    [[nodiscard]] point<T> rotated_around_to_distance(const point<T>& other, T dist) const {
        return (*this - other).rotated_to_distance(dist) + other;
    }
    [[nodiscard]] point<T> rotated_90() const {
        return point<T>(-y(), x());
    }
    [[nodiscard]] point<T> rotated_neg90() const {
        return point<T>(y(), -x());
    }
    [[nodiscard]] point<T> rotated_90_around(const point<T>& other) const {
        return (*this - other).rotated_90() + other;
    }
    [[nodiscard]] point<T> rotated_neg90_around(const point<T>& other) const {
        return (*this - other).rotated_neg90() + other;
    }
    [[nodiscard]] point<T> rotated_180() const {
        return point<T>(-x(), -y());
    }
    [[nodiscard]] point<T> rotated_180_around(const point<T>& other) const {
        return (*this - other).rotated_180() + other;
    }
    [[nodiscard]] point<T> flipped_x() const {
        return point<T>(-x(), y());
    }
    [[nodiscard]] point<T> flipped_y() const {
        return point<T>(x(), -y());
    }
    template<typename FLOAT_T>
    [[nodiscard]] static point<T> at_angle(FLOAT_T angle, FLOAT_T magnitude) {
        static_assert(std::is_floating_point<FLOAT_T>(),
                      "Arguments must be floating point");

        const auto x = std::cos(angle)*magnitude;
        const auto y = std::sin(angle)*magnitude;
        return point<T>(x, y);
    }

private:
    T x_ = 0;
    T y_ = 0;
    constexpr static void require_float() {
        static_assert(std::is_floating_point<T>(),
                      "Function not available for integer types");
    }
};


} // namespace cosel

#endif
