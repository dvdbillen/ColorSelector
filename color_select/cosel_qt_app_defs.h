// SPDX-License-Identifier: MIT
// Copyright (c) 2026 David Billen

// Constant definitions

#ifndef cosel_cosel_qt_app_defs_h
#define cosel_cosel_qt_app_defs_h

#include <algorithm>
#include <array>
#include <cassert>
#include <numbers>
#include <vector>

#include "cosel_portable.h"
#include <QObject>
#include <QSize>
#include <QString>

namespace cosel::konst {

// Defaults for select_widget options_t if unspecified
inline static constexpr double default_corner_rounding() {
    // This is 1 - sqrt(2)/2
    return .2928;
}
inline static constexpr int default_slider_height() {
    return 28;
}
inline static constexpr int default_2d_resolution() {
    return 12;
}
inline static constexpr bool default_2d_bordered() {
    return true;
}


// Default 2d widget size. Currently it's not resizable so this is the size.
inline static constexpr QSize default_2d_size() {
    return {232, 232};
}

// Height of the saturation slider in the 2D widtet
inline static constexpr int slider_2d_height() {
    return 16;
}

// Size hint width for a group of color selector sliders
inline static constexpr int default_selector_width() {
    return 150;
}

// Size hint height padding for a group of color selector sliders
inline static constexpr int selector_height_padding() {
    return 4;
}

// Size of the main swatch
inline static constexpr QSize swatch_size() {
    return QSize{64, 64};
}

// Size of swatches in the palette(s)
inline static constexpr int palette_swatch_size() {
    return 20;
}

// Min/max size of numeric edit boxes
inline static constexpr int min_edit_width() {
    return 55;
}
inline static constexpr int max_edit_width() {
    return 60;
}

// Min/max size of labels in info widget
inline static constexpr int min_label_width() {
    return 65;
}
inline static constexpr int max_label_width() {
    return 65;
}

// Checkerboard gray levels for displaying alpha. Note the results may vary at different color
// space gamma settings
inline static constexpr uint8_t alpha_checkerboard_dark() {
    return 100;
}
inline static constexpr uint8_t alpha_checkerboard_light() {
    return 155;
}


} // namespace cosel::konst


#endif // cosel_cosel_qt_app_defs_h
