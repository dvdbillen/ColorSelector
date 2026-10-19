// SPDX-License-Identifier: MIT
// Copyright (c) 2026 David Billen

// Definitions for each type of color selector group we support

#ifndef cosel_qt_slider_model_info_h
#define cosel_qt_slider_model_info_h

#include <array>
#include <QString>

namespace cosel {

///////////////////////////////////////////////////////////////////////////////////////////////////
enum class selector_type_t : std::int8_t {
    rgb,
    hsl,
    hsv,
    hsl2,
    hsv2
};
constexpr size_t num_selector_types = 5;

struct selector_info_t {
    selector_type_t type;
    QString name;
    // 3 character label
    QString component_names;
};

static inline const selector_info_t& get_selector_info(size_t type) {
    static std::array<selector_info_t, num_selector_types> info_vec = {
            selector_info_t{selector_type_t::rgb, QObject::tr("RGB"), QObject::tr("RGB")},
            selector_info_t{selector_type_t::hsl, QObject::tr("HSL"), QObject::tr("HSL")},
            selector_info_t{selector_type_t::hsv, QObject::tr("HSV"), QObject::tr("HSV")},
            selector_info_t{selector_type_t::hsl2, QObject::tr("HSL+"), QObject::tr("HSL")},
            selector_info_t{selector_type_t::hsv2, QObject::tr("HSV+"), QObject::tr("HSV")}
    };

    assert(type < num_selector_types);
    assert(info_vec[type].component_names.size() >= 3);

    return info_vec[type];
}

static inline const selector_info_t& get_selector_info(selector_type_t type) {
    return get_selector_info(size_t(type));
}


///////////////////////////////////////////////////////////////////////////////////////////////////
enum class twod_type_t : std::int8_t {
    grid,
    ring,
    info,
};
constexpr size_t num_twod_types = 3;

struct twod_info_t {
    twod_type_t type;
    QString name;
};

static inline const twod_info_t& get_twod_info(size_t type) {
    static std::array<twod_info_t, num_twod_types> info_vec = {
            twod_info_t{twod_type_t::grid, QObject::tr("Grid")},
            twod_info_t{twod_type_t::ring, QObject::tr("Ring")},
            twod_info_t{twod_type_t::info, QObject::tr("Edit")}
    };

    assert(type < num_twod_types);

    return info_vec[type];
}

static inline const twod_info_t& get_twod_info(twod_type_t type) {
    return get_twod_info(size_t(type));
}


} // namespace cosel


#endif // cosel_qt_slider_model_info_h
