// SPDX-License-Identifier: MIT
// Copyright (c) 2026 David Billen

// Optional developer utility that outputs a fully constexpr gamut class definition. Not part of
// the public API. Include only when you need to generate a new predefined gamut.

#ifndef cosel_gamut_generator_h
#define cosel_gamut_generator_h

#include "cosel_space.h"
#include <iostream>
#include <iomanip>

namespace cosel {

// Output a complete constexpr gamut class definition to std::cout.

inline static void generate_gamut_class(const gamut& g, const char* class_name) {
    auto print_triplet = [](const triplet& t, int precision = 7) {
        std::cout << std::fixed << std::setprecision(precision)
                  << "triplet(" << t.comp[0] << ", " << t.comp[1] << ", " << t.comp[2] << ")";
    };

    std::cout <<
              "////////////////////////////////////////////////////////////////////////////////////////////////////\n"
              << "// Predefined " << class_name <<
              " gamut so that it constructs instantly and is all accessed as constants. See\n"
              << "// function comments in gamut class.\n"
              << "class " << class_name << " {\n"
              << "    constexpr static std::array<triplet, 3> f_mx = {\n"
              << "        ";
    print_triplet(g.rgb2xyz_matrix()[0]);
    std::cout << ",\n"
              << "        ";
    print_triplet(g.rgb2xyz_matrix()[1]);
    std::cout << ",\n"
              << "        ";
    print_triplet(g.rgb2xyz_matrix()[2]);
    std::cout << "\n"
              << "    };\n"
              << "    constexpr static std::array<triplet, 3> r_mx = {\n"
              << "        ";
    print_triplet(g.xyz2rgb_matrix()[0]);
    std::cout << ",\n"
              << "        ";
    print_triplet(g.xyz2rgb_matrix()[1]);
    std::cout << ",\n"
              << "        ";
    print_triplet(g.xyz2rgb_matrix()[2]);
    std::cout << ";\n"
              << "    constexpr static triplet xyz_w = ";
    print_triplet(g.xyz_white());
    std::cout << ";\n"
              << "    constexpr static triplet xyz_r = triplet(1/" << std::setprecision(
                  7) << g.xyz_white().comp[0]
              << ", 1.00000, 1/" << g.xyz_white().comp[2] << ");\n"
              << "    constexpr static triplet lum_mx = ";
    print_triplet(g.luminance_mx());
    std::cout << ";\n"
              << "public:\n"
              << "    constexpr bool d65() const {\n"
              << "        return " << (g.d65() ? "true" : "false") << ";\n"
              << "    }\n"
              << "    constexpr const std::array<triplet, 3>& rgb2xyz_matrix() const {\n"
              << "        return f_mx;\n"
              << "    }\n"
              << "    constexpr const std::array<triplet, 3>& xyz2rgb_matrix() const {\n"
              << "        return r_mx;\n"
              << "    }\n"
              << "    // Does not scale to 100.\n"
              << "    constexpr triplet rgb2xyz(const triplet& rgbt) const {\n"
              << "        return triplet(\n"
              << "                   vec::dot(rgbt.comp, f_mx[0].comp),\n"
              << "                   vec::dot(rgbt.comp, f_mx[1].comp),\n"
              << "                   vec::dot(rgbt.comp, f_mx[2].comp)\n"
              << "               );\n"
              << "    }\n"
              << "    // Expects normal XYZ (not scaled to 100).\n"
              << "    constexpr triplet xyz2rgb(const triplet& xyzt) const {\n"
              << "        return triplet(\n"
              << "                   vec::dot(xyzt.comp, r_mx[0].comp),\n"
              << "                   vec::dot(xyzt.comp, r_mx[1].comp),\n"
              << "                   vec::dot(xyzt.comp, r_mx[2].comp)\n"
              << "               );\n"
              << "    }\n"
              << "    constexpr double luminance(const triplet& rgbt) const {\n"
              << "        return vec::dot(rgbt.comp, lum_mx.comp);\n"
              << "    }\n"
              << "    constexpr const triplet& luminance_mx() const {\n"
              << "        return lum_mx;\n"
              << "    }\n"
              << "    constexpr const triplet& xyz_white() const {\n"
              << "        return xyz_w;\n"
              << "    }\n"
              << "};\n";
    std::cout << std::endl;

}

} // namespace cosel

#endif // cosel_gamut_generator_h
