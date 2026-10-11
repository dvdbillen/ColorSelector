
#include <iostream>

#include "color_select/cosel_math.h"
#include "color_select/cosel_model_hsx.h"
#include "color_select/cosel_gradient.h"

#include <cfenv>

#include "unit_test.h"

using namespace cosel;

inline std::ostream& triplet_out(std::ostream& stream, const char* name, const triplet& data) {
    return stream << name << '(' << data.comp[0] << ',' << data.comp[1] << ',' << data.comp[2] << ")";
}

inline std::ostream& operator<< (std::ostream& stream, const hsv& c) {
    return triplet_out(stream, "hsv", c.data());
}

inline std::ostream& operator<< (std::ostream& stream, const xyz& c) {
    return triplet_out(stream, "xyz", c.data());
}

inline std::ostream& operator<< (std::ostream& stream, const hsv2& c) {
    return triplet_out(stream, "hsv2", c.data());
}

inline std::ostream& operator<< (std::ostream& stream, const hsl& c) {
    return triplet_out(stream, "hsl", c.data());
}

inline std::ostream& operator<< (std::ostream& stream, const hsl2& c) {
    return triplet_out(stream, "hsl2", c.data());
}

inline std::ostream& operator<< (std::ostream& stream, const rgb& c) {
    return triplet_out(stream, "rgb", c.data());
}

inline std::ostream& operator<< (std::ostream& stream, const lab& c) {
    return triplet_out(stream, "lab", c.data());
}

inline std::ostream& operator<< (std::ostream& stream, const oklab& c) {
    return triplet_out(stream, "oklab", c.data());
}

inline std::ostream& operator<< (std::ostream& stream, const luv& c) {
    return triplet_out(stream, "luv", c.data());
}

struct test_type {
    std::string name;
    double low = 0;
    double high = 1;

    explicit test_type(const char* name_, double low_ = 0, double high_ = 1) :
        name(name_),
        low(low_),
        high(high_) {}
};

template<class T, class U, typename T2U, typename U2T>
void test_2_way(const test_type& t, int resolution, T2U t2u, U2T u2t)

{
    auto worst_error = 0.0;
    T worst_t;
    U worst_u;
    T worst_t2;

    int worst_i=0, worst_j=0, worst_k=0;

    const auto inc = 1.0/(resolution - 1);

    for(int i = 0; i < resolution; ++i) {
        const auto comp0 = lerp(t.low, t.high, i*inc);

        for(int j = 0; j < resolution; ++j) {
            const auto comp1 = lerp(t.low, t.high, j*inc);

            for(int k = 0; k < resolution; ++k) {
                const auto comp2 = lerp(t.low, t.high, k*inc);
                const auto tinst = T(comp0, comp1, comp2);
                const auto uinst = t2u(tinst);
                const auto tinst2 = u2t(uinst);

                const auto error = tinst.data().distance_squared(tinst2.data());

                if(error > worst_error) {
                    worst_error = error;
                    worst_t = tinst;
                    worst_u = uinst;
                    worst_t2 = tinst2;
                    worst_i = i;
                    worst_j = j;
                    worst_k = k;
                }
            }
        }
    }

    std::cout << "Worst error = " << sqrtz(worst_error) << " @ " <<
                 worst_i << ',' << worst_j << ',' << worst_k << '\n';
    std::cout << worst_t << " -> " << worst_u << " -> " << worst_t2 << '\n';
}

template<class U>
void rgb_2_way(const char* type, int count) {
    std::cout << "\nrgb <-> " << type << std::endl; // NOLINT;

    test_2_way<rgb, U>(test_type("rgb", .01, .9999), count,
    [](const rgb& t) {
        return U(t);
    },
    [](const U& u) {
        return rgb(u);
    });
}

template<class U>
void rgb_2_way_hsx(const char* type, int count) {
    std::cout << "\nrgb <-> " << type << std::endl; // NOLINT;

    test_2_way<rgb, U>(test_type("rgb", .01, .9999), count,
    [](const rgb& t) {
        return U(t);
    },
    [](const U& u) {
        return u.to_rgb();
    });
}

void unit_test() {
    constexpr int count = 200;

    rgb_2_way<xyz>("xyz", count);
    rgb_2_way<luv>("luv", count);
    rgb_2_way<lab>("lab", count);
    rgb_2_way<oklab>("oklab", count);
    rgb_2_way_hsx<hsv>("hsv", count);
    rgb_2_way_hsx<hsl>("hsl", count);
    rgb_2_way_hsx<hsv2>("hsv2", count);
    rgb_2_way_hsx<hsl2>("hsl2", count);
    std::cout << std::endl; // NOLINT
}
