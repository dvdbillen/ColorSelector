// SPDX-License-Identifier: MIT
// Copyright (c) 2026 David Billen

// Triplet class
// The triplet class is a storage class with some helpers used by all color models

#ifndef cosel_triplet_h
#define cosel_triplet_h

#include "cosel_portable.h"
#include "cosel_vec.h"

namespace cosel {

struct alignas(32) triplet {
    // Fourth component is for alignment (unused)
    double comp[4] = {}; // NOLINT: Don't wish to use array

    constexpr triplet() noexcept = default;
    constexpr explicit triplet(double c1, double c2, double c3) noexcept
        : comp {
        c1, c2, c3
    } {}
    constexpr explicit triplet(double n) noexcept
        : comp {
        n, n, n
    } {}
    constexpr explicit triplet(const double* comp_) noexcept
        : comp {
        comp_[0], comp_[1], comp_[2]
    } {}
    constexpr triplet& operator =(double n) noexcept {
        vec::set(comp, n);
        return *this;
    }
    constexpr void clear() noexcept {
        vec::clear(comp);
    }
    [[nodiscard]] constexpr bool null() const noexcept {
        return comp[0] == 0 && comp[1] == 0 && comp[2] == 0 ;
    }
    [[nodiscard]] constexpr bool normal() const noexcept {
        return comp[0] >= 0.0 && comp[0] <= 1.0 &&
               comp[1] >= 0.0 && comp[1] <= 1.0 &&
               comp[2] >= 0.0 && comp[2] <= 1.0;
    }
    [[nodiscard]] constexpr bool bad() const noexcept {
        return cosel::bad(comp[0]) || cosel::bad(comp[1]) || cosel::bad(comp[2]);
    }
    constexpr void fix() noexcept {
        comp[0] = cosel::fix(comp[0]);
        comp[1] = cosel::fix(comp[1]);
        comp[2] = cosel::fix(comp[2]);
    }
    [[nodiscard]] constexpr triplet fixed() const noexcept {
        return triplet(cosel::fix(comp[0]), cosel::fix(comp[1]), cosel::fix(comp[2]));
    }
    [[nodiscard]] constexpr bool operator==(const triplet& d) const noexcept {
        return vec::compare(comp, d.comp);
    }
    [[nodiscard]] constexpr bool operator!=(const triplet& d) const noexcept {
        return !vec::compare(comp, d.comp);
    }
    constexpr void set(double n) noexcept {
        return vec::set(comp, n);
    }
    [[nodiscard]] constexpr double min() const noexcept {
        return vec::min(comp);
    }
    [[nodiscard]] constexpr double max() const noexcept {
        return vec::max(comp);
    }
    constexpr void interpolate(const triplet& NOALIAS_REF d, double amt) noexcept {
        vec::interpolate(comp, d.comp, amt);
    }
    [[nodiscard]] constexpr triplet interpolated(const triplet& NOALIAS_REF d,
            double amt) const noexcept {
        auto ret(*this);
        ret.interpolate(d, amt);
        return ret;
    }
    [[nodiscard]] constexpr triplet operator+(const triplet& NOALIAS_REF d) const noexcept {
        auto ret(*this);
        vec::add(ret.comp, d.comp);
        return ret;
    }
    [[nodiscard]] constexpr triplet operator-(const triplet& NOALIAS_REF d) const noexcept {
        auto ret(*this);
        vec::sub(ret.comp, d.comp);
        return ret;
    }
    [[nodiscard]] constexpr triplet operator*(const triplet& NOALIAS_REF d) const noexcept {
        auto ret(*this);
        vec::mul(ret.comp, d.comp);
        return ret;
    }
    // This does not check for zero
    [[nodiscard]] constexpr triplet operator/(const triplet& NOALIAS_REF d) const noexcept {
        auto ret(*this);
        vec::div(ret.comp, d.comp);
        return ret;
    }
    [[nodiscard]] constexpr triplet operator+(double n) const noexcept {
        auto ret(*this);
        vec::add(ret.comp, n);
        return ret;
    }
    [[nodiscard]] constexpr triplet operator-(double n) const noexcept {
        auto ret(*this);
        vec::sub(ret.comp, n);
        return ret;
    }
    [[nodiscard]] constexpr triplet operator*(double n) const noexcept {
        auto ret(*this);
        vec::mul(ret.comp, n);
        return ret;
    }
    [[nodiscard]] constexpr triplet operator/(double n) const noexcept {
        auto ret(*this);
        vec::mul(ret.comp, 1/n);
        return ret;
    }
    constexpr void operator+=(const triplet& NOALIAS_REF d) noexcept {
        vec::add(comp, d.comp);
    }
    constexpr void operator-=(const triplet& NOALIAS_REF d) noexcept {
        vec::sub(comp, d.comp);
    }
    constexpr void operator*=(const triplet& NOALIAS_REF d) noexcept {
        vec::mul(comp, d.comp);
    }
    constexpr void operator+=(double n) noexcept {
        vec::add(comp, n);
    }
    constexpr void operator-=(double n) noexcept {
        vec::sub(comp, n);
    }
    constexpr void operator*=(double n) noexcept {
        vec::mul(comp, n);
    }
    constexpr void operator/=(double n) noexcept {
        vec::mul(comp, 1/n);
    }
    [[nodiscard]] constexpr triplet inverted() const noexcept {
        auto ret(*this);
        vec::invert(ret.comp, 1);
        return ret;
    }
    [[nodiscard]] constexpr double dot(const triplet& NOALIAS_REF d) const noexcept {
        return vec::dot(comp, d.comp);
    }
    [[nodiscard]] constexpr double delta_distance_squared() const noexcept {
        return vec::delta_distance_squared(comp);
    }
    [[nodiscard]] constexpr double distance_squared(const triplet& NOALIAS_REF d) const noexcept {
        return vec::distance_squared(comp, d.comp);
    }
    [[nodiscard]] double delta_distance() const noexcept {
        return vec::delta_distance(comp);
    }
    [[nodiscard]] double distance(const triplet& NOALIAS_REF d) const noexcept {
        return vec::distance(comp, d.comp);
    }
    [[nodiscard]] constexpr triplet cropped_negative() const noexcept {
        auto ret(*this);
        vec::crop_negative(ret.comp);
        return ret;
    }
    [[nodiscard]] constexpr triplet cropped_positive() const noexcept {
        auto ret(*this);
        vec::crop_positive(ret.comp);
        return ret;
    }
    [[nodiscard]] constexpr triplet cropped() const noexcept {
        auto ret(*this);
        vec::crop(ret.comp);
        return ret;
    }
    [[nodiscard]] constexpr triplet cropped(double low, double high) const noexcept {
        auto ret(*this);
        ret.comp[0] = std::clamp<double>(ret.comp[0], low, high);
        ret.comp[1] = std::clamp<double>(ret.comp[1], low, high);
        ret.comp[2] = std::clamp<double>(ret.comp[2], low, high);
        return ret;
    }

    [[nodiscard]] triplet mat3_mul(const double m[3][3]) const noexcept { // NOLINT
        return triplet(
                   vec::dot(&m[0][0], &comp[0]),
                   vec::dot(&m[1][0], &comp[0]),
                   vec::dot(&m[2][0], &comp[0]));
    }
};

} // namespace cosel


#endif // cosel_triplet_h
