// SPDX-License-Identifier: MIT
// Copyright (c) 2026 David Billen

// Rendering helpers


#ifndef cosel_qt_render_helper_h
#define cosel_qt_render_helper_h

#include <QImage>
#include <QPixmap>
#include <QWidget>

#include "cosel_qt_tools.h"

namespace cosel {

namespace details {

// Calculate corner rounding based on height
static inline int calc_rounding(int height, double rounding) {
    return std::lround(rounding*(height)*.5);
}

// Subtract n from c and clamp the unsigned result
static inline uchar subsat(uchar c, uchar n) {
    return c - std::min(c, n);
}

// Clear pixels on a scanline. Specialized function used by corner rounding.
static inline void clear_half_scanline(uchar* NOALIAS pixels, int pitch, int y, int length) {
    constexpr int alpha = 3;

    const auto offset = ptrdiff_t(y)*pitch;
    pixels = &pixels[offset];
    length = std::min(length, (pitch - 4)/2);

    // This function is called twice over the same scanline to produce the results of clearing two
    // virtual lines, thus we subtract from alpha rather than setting it to zero.

    const auto fill_length = (length*2) & ~3;

    for(int i = 0; i < fill_length; i += 4) {
        pixels[i + alpha] = subsat(pixels[i + alpha], 0x80);
        pixels[pitch - i - 4 + alpha] = subsat(pixels[pitch - i - 4 + alpha], 0x80);
    }

    if(0 != (length&1)) {
        pixels[fill_length + alpha] = subsat(pixels[fill_length + alpha], 0x40);
        pixels[pitch - fill_length - 4 + alpha] = subsat(pixels[pitch - fill_length - 4 + alpha], 0x40);
    }
}

}


////////////////////////////////////////////////////////////////////////////////////////////////////
// Round the corners of the pixels in a QImage

inline static void round_corners(QImage& image, int round_lines) {
    if(round_lines < 1) {
        return;
    }

    round_lines *= 2;

    const auto radius = double(round_lines);
    const auto pitch = image.bytesPerLine();
    const auto height = image.height();
    auto* pixels = image.bits();

    // Iterate over twice the number of actual pixels. This is done for antialiasing and is
    // integrated with how the clear_half_scanline() function works.

    for(int y = 0; y < round_lines; ++y) {
        const auto yf = radius - y - .5;
        // Solve circle equation for x given yf
        const auto xf = sqrt(radius*radius - yf*yf);
        const auto x = std::max(0, int(std::round(radius - xf)));

        const auto hy = y/2;
        // Clear top
        details::clear_half_scanline(pixels, pitch, hy, x);
        // Clear bottom
        details::clear_half_scanline(pixels, pitch, height - hy - 1, x);
    }
}


////////////////////////////////////////////////////////////////////////////////////////////////////
// Propagate top scanline of input QImage to all scanlines then round the corners

inline static void top_line_to_all_lines(QImage& image, int offset, int height, double rounding) {
    const auto pitch = image.bytesPerLine();
    auto* pixels = image.bits();

    for(int y = offset; y < height; ++y) {
        memcpy(&pixels[pitch*y], &pixels[0], pitch);
    }

    if(offset > 1) {
        memcpy(&pixels[0], &pixels[pitch], pitch);
    }

    const auto round_lines = std::min(height/2, details::calc_rounding(height, rounding));
    round_corners(image, round_lines);
}



////////////////////////////////////////////////////////////////////////////////////////////////////
// Render a pointer for a gradient slider. The specified widget is used to access the pixel ratio.

inline static QPixmap render_pointer(QWidget* widget, const QRect* kr, bool pointer_top) {
    const auto pointer_scaled = rescaled(widget, kr->size());
    const auto pointer_size = QSize(pointer_scaled.width()*2, pointer_scaled.height()*2);
    QImage image(pointer_size.width(), pointer_size.height(), QImage::Format_RGBA8888);
    image.fill(qRgba(0, 0, 0, 0));

    {
        QPainter p(&image);

        // Calculate pointer polygon coordinates

        using pt_t = point<qreal>;
        constexpr std::array<pt_t, 5> pointer_coords = {
            pt_t(0, 1), pt_t(0, 1 - 1.0/3.5),
            pt_t(.5, 0),
            pt_t(1, 1 - 1.0/3.5), pt_t(1, 1)
        };

        const qreal pen_width = 3;
        const qreal pen_hwidth = pen_width/2;
        const qreal x_scale = pointer_size.width() - 4;
        const qreal x_off = pen_hwidth;
        const qreal y_scale = (pointer_top? -pointer_size.width():pointer_size.width())/2.0;
        const qreal y_off = pointer_top? pen_hwidth - y_scale: 0;

        std::vector<QPointF> pointer_polygon;
        pointer_polygon.reserve(pointer_coords.size() + 1);

        for(const auto& pt : pointer_coords) {
            const auto x = x_off + pt.x()*x_scale;
            const auto y = y_off + pt.y()*y_scale;
            pointer_polygon.emplace_back(x, y);
        }

        pointer_polygon.emplace_back(pointer_polygon.front());

        // Render pointer

        QPen pen(QColor(0, 0, 0, 255), pen_width);
        p.setPen(pen);
        p.setBrush(QColor(255, 255, 255, 255));
        p.setRenderHint(QPainter::Antialiasing);
        p.drawPolygon(pointer_polygon.data(), pointer_polygon.size());
    }

    image = image.scaled(pointer_size.width()/2, pointer_size.height()/2,
                         Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    auto pixmap = QPixmap::fromImage(image);
    pixmap.setDevicePixelRatio(widget->devicePixelRatioF());
    return pixmap;
}



} // namespace cosel


#endif // cosel_qt_render_helper_h
