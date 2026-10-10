// SPDX-License-Identifier: MIT
// Copyright (c) 2026 David Billen

// Rendering of alpha slider gradients to QImage

#ifndef cosel_qt_alphapixmap_h
#define cosel_qt_alphapixmap_h

#include <cctype>
#include <vector>

#include <QColor>
#include <QImage>

#include "cosel_portable.h"
#include "cosel_qt_app_defs.h"
#include "cosel_qt_render_helper.h"
#include "cosel_qt_tools.h"

namespace cosel {

namespace details {

// Apply alpha to r,g,b, against bg,bg,bg. Alpha is 16 bit, all others are 8 bit
inline uint32_t modulate_bg_fg(uint_fast32_t r, uint_fast32_t g, uint_fast32_t b,
                               uint_fast32_t bg, uint_fast32_t alpha) {

    bg *= 0xFFFFu - alpha;
    r = bg + r*alpha;
    g = bg + g*alpha;
    b = bg + b*alpha;

    return image_pixel(r >> 16u, g >> 16u, b >> 16u, 0xFFu).pixel;
}

} // namespace details


////////////////////////////////////////////////////////////////////////////////////////////////////
// Return a QImage that contains a rendering of a color from transparent to full opacity with
// rounded corners
static inline QImage make_alpha_pixmap(const rgb& color, int width, int height, double rounding) {

    // This is all done in the current colorspace gamma adjusted RGB. This may cause the
    // checkerboard pattern to vary a little in different colorspaces, but the values are not
    // precise anyway, just two shades of gray without much contrast.

    const uint_fast32_t light_dark[2] =
    {konst::alpha_checkerboard_light(), konst::alpha_checkerboard_dark()}; // NOLINT

    uint16_t r = 0;
    uint16_t g = 0;
    uint16_t b = 0;
    color.to_output<8>(r, g, b);

    // Make a QImage and setup pointer
    QImage image(width, height, QImage::Format_RGBA8888);
    image.fill(qRgba(0, 0, 0, 0));
    auto* pixels = reinterpret_cast<uint32_t*>(image.bits());
    const auto pitch = image.bytesPerLine()/4;

    // Make a scanline buffer and setup pointer
    std::vector<QRgb> scanline(width);
    QRgb* NOALIAS scanlinex = scanline.data();

    const auto alpha_inc = 1.0/(width - 1);

    // Set scanline to 'even' checkerboard pattern
    for(int x = 0; x < width; ++x) {
        const auto pos = uint_fast32_t(x) >> 3u;
        //const auto alpha = uint_fast32_t(sqrtz(x*alpha_inc)*0xFFFF + .5);
        const auto alpha = uint_fast32_t(x*alpha_inc*0xFFFF + .5);
        scanline[x] = details::modulate_bg_fg(r, g, b, light_dark[pos & 1u], alpha);
    }

    // Propagate even checkerboard pattern to appropriate image lines
    for(int y = 0; y < height; y += 16) {
        const auto end = std::min(height - y, 8);

        for(int i = 0; i < end; ++i) {
            memcpy(&pixels[(y + i)*pitch], scanlinex, width*4);
        }
    }

    // Set scanline to 'odd' checkerboard pattern
    for(int x = 0; x < width; ++x) {
        const auto pos = (uint_fast32_t(x) >> 3u) + 1;
        //const auto alpha = uint_fast32_t(sqrtz(x*alpha_inc)*0xFFFF + .5);
        const auto alpha = uint_fast32_t(x*alpha_inc*0xFFFF + .5);
        scanline[x] = details::modulate_bg_fg(r, g, b, light_dark[pos & 1u], alpha);
    }

    // Propagate odd checkerboard pattern to appropriate image lines
    for(int y = 8; y < height; y += 16) {
        const auto end = std::min(height - y, 8);

        for(int i = 0; i < end; ++i) {
            memcpy(&pixels[(y + i)*pitch], scanlinex, width*4);
        }
    }

    if(!iszero(rounding)) {
        // Handle corner rounding
        const auto round_lines = std::min(height/2, details::calc_rounding(height, rounding));
        round_corners(image, round_lines);
    }

    return image;
}


} // namespace cosel


#endif // cosel_qt_alphapixmap_h
