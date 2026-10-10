// SPDX-License-Identifier: MIT
// Copyright (c) 2026 David Billen

// Rendering of gradients to QImages

// TODO: Either make segmented gradients an option or remove support function

#ifndef cosel_qt_gradpixmap_h
#define cosel_qt_gradpixmap_h

#include <vector>

#include <QColor>
#include <QImage>

#include "cosel_portable.h"
#include "cosel_qt_render_helper.h"

namespace cosel {

// Return a QImage that contains a rendering of the specified rgb vector.
static inline QImage make_gradient_image(const std::vector<rgb>& gradient, int height,
        double rounding) {
    const auto width = int(gradient.size());
    QImage image(width, height, QImage::Format_RGBA8888);
    image.fill(qRgba(0, 0, 0, 0));

    auto* NOALIAS pixels = reinterpret_cast<uint32_t*>(image.bits());

    for(unsigned i = 0; i < gradient.size(); ++i) {
        pixels[i] = to_image_pixel(gradient[i], 255);
    }

    top_line_to_all_lines(image, 0, image.height(), rounding);
    return image;
}

// Make a segmented gradient where each step occupies more than 1 pixel. Currently unused but I am
// not yet ready to discard it from the source.
static inline QImage make_gradient_image(const std::vector<rgb>& gradient, int height,
        QColor bg_color, int segment_width, int segment_height, int segment_y, int segment_gap) {

    // Clamp some values but currently we do not sanity check

    segment_gap = std::min(segment_gap, segment_width - 1);
    segment_height = std::min(segment_height, height);
    segment_y = std::min(segment_y, height - segment_height);

    const auto segment_fill = segment_width - segment_gap;
    const auto segment_offset = segment_gap/2;
    const int width = gradient.size()*segment_width;
    QImage image(width, height, QImage::Format_RGBA8888);
    image.fill(qRgba(0, 0, 0, 0));

    const auto opaque = (height == segment_height && segment_gap == 0);

    if(!opaque) {
        image.fill(bg_color);
    }

    auto* NOALIAS pixels = reinterpret_cast<uint32_t*>(image.bits());

    for(unsigned i = 0; i < gradient.size(); ++i) {
        const auto pixel = to_image_pixel(gradient[i], 255);

        const auto offset = i*segment_width;

        for(int j = 0; j < segment_fill; ++j) {
            pixels[offset + segment_offset + j] = pixel;
        }
    }

    top_line_to_all_lines(image, segment_y, segment_height, 0.0);
    return image;
}


} // namespace cosel


#endif // cosel_qt_gradpixmap_h
