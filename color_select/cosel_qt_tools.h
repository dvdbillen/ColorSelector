// SPDX-License-Identifier: MIT
// Copyright (c) 2026 David Billen

// Misc Qt helpers

// TODO: This needs to be pruned especially the unscale/rescale stuff which is barely used

#ifndef cosel_qt_tools_h
#define cosel_qt_tools_h

#include <vector>

#include <QBrush>
#include <QColor>
#include <QtGlobal>
#include <QImage>
#include <QObject>
#include <QPainter>
#include <QPaintDevice>
#include <QPaintEngine>
#include <QPen>
#include <QPixmap>
#include <QTimer>
#include <QTransform>
#include <QWidget>

#include "cosel_geometry.h"
#include "cosel_portable.h"
#include "cosel_math.h"
#include "cosel_model.h"


// This application should support any Qt version 6.0 or higher, but this has not been fully tested
static_assert(QT_VERSION >= QT_VERSION_CHECK(6, 0, 0),
              "Qt version 6.0 or higher is required for this application.");


namespace cosel {

template<typename FUNC>
static inline void defer(int delay_millis, FUNC f) {
    QTimer::singleShot(delay_millis, f);
};

union image_pixel {
    uint32_t pixel;
    uint8_t bytes[4]; // NOLINT: Not using std::array - this is a union

    constexpr explicit image_pixel(uint8_t r, uint8_t g, uint8_t b, uint8_t a) noexcept :
        bytes {
        r, g, b, a
    } {}
};

// For setting the pixels in Qt image data. The image must have been created using
// QImage::Format_RGBA8888
static inline uint32_t to_image_pixel(const rgb& c, uint8_t alpha) {

    uint16_t r, g, b;
    c.to_output<8>(r, g, b);
    return image_pixel(r, g, b, alpha).pixel;
}

// Return background and forground colors of the specified widget palette based on QPalette::Window
// and QPalette::WindowText respectively
static inline std::pair<QColor, QColor> get_bg_fg_colors(QWidget* widget) {
    widget->ensurePolished();
    const auto& palette = widget->palette();

    auto bg = palette.color(QPalette::Window);
    auto fg = palette.color(QPalette::WindowText);
    return { bg, fg };
}


// Map src widget rect to dst
static inline QRect maprect(const QWidget* dst, const QWidget* src) {
    const auto r = src->rect();
    return QRect(src->mapTo(dst, r.topLeft()), r.size());
}


// Convert our point type to Qt
template<typename FLOAT_T>
static inline QPointF geo(const point<FLOAT_T>& pt) {
    return QPointF(pt.x(), pt.y());
}
static inline QPoint geo(const point<int>& pt) {
    return QPoint(pt.x(), pt.y());
}

// Convert Qt point type to ours
static inline point<qreal> geo(const QPointF& pt) {
    return point<qreal>(pt.x(), pt.y());
}
static inline point<int> geo(const QPoint& pt) {
    return point<int>(pt.x(), pt.y());
}


// Return QColor with specified alpha
static inline QColor with_alpha(const QColor& c, int alpha) {
    return QColor(c.red(), c.green(), c.blue(), alpha);
}

// Return our rgb type from QColor
static inline rgb qt_to_rgb(const QColor& c) {
    return rgb::from_bits<8>(c.red(), c.green(), c.blue());
}

// Return QColor from our rgb type
static inline QColor qt_from_rgb(const rgb& c, float alpha = 1) {
    const auto adjusted = c.to_output();
    QColor ret;
    ret.setRgbF(float(adjusted.r()), float(adjusted.g()), float(adjusted.b()), alpha);
    return ret;
}


// Interpolate QColor
static inline QColor interpolate(const QColor& from, const QColor& to, float amt) {
    amt = clamp(amt);
    const auto r = lerp(from.redF(), to.redF(), amt);
    const auto g = lerp(from.greenF(), to.greenF(), amt);
    const auto b = lerp(from.blueF(), to.blueF(), amt);
    return QColor::fromRgbF(r, g, b);
}
static inline QColor interpolate(const QColor& from, const QColor& to, int amt) {
    return interpolate(from, to, amt/255.0f);
}

// Return luminance for QColor
static inline int brightness(const QColor& c) {
    const auto rgb_c = qt_to_rgb(c);
    const auto b = int(color_space().to_gamma(rgb_c.level())*255 + .5);
    return std::clamp(b, 0, 255); // Shouldn't be out of range
}

// Force QColor to a specified luminance
static inline QColor force_brightness(const QColor& c, int b) {
    b = std::clamp(b, 0, 255);
    const auto rgb_c = qt_to_rgb(c).forced(color_space().to_linear(b/255.0));
    return qt_from_rgb(rgb_c.clamped_gamut());
}


// Return QPen with alpha
static inline QPen qt_pen(const rgb& c, float alpha = 1) {
    return QPen(qt_from_rgb(c, alpha));
}

// Return QBrush with alpha
static inline QBrush qt_brush(const rgb& c, float alpha = 1) {
    return QBrush(qt_from_rgb(c, alpha));
}


// Convert QSizeF to integer with fast truncation
static inline QSize trunc(const QSizeF& sz) {
    return QSize(static_cast<int>(sz.width()), static_cast<int>(sz.height()));
}

// Convert QPointF to integer with fast truncation
static inline QPoint trunc(const QPointF& pt) {
    return QPoint(static_cast<int>(pt.x()), static_cast<int>(pt.y()));
}

// Convert QRectF to integer with fast truncation
static inline QRect trunc(const QRectF& r) {
    return QRect(trunc(r.topLeft()), trunc(r.size()));
}

// Convert QSizeF to integer rounded
static inline QSize round(const QSizeF& sz) {
    return trunc(QSizeF(sz.width() + .5, sz.height() + .5));
}

// Convert QPointF to integer rounded
static inline QPoint round(const QPointF& pt) {
    return QPoint(std::round(pt.x()), std::round(pt.y()));
}

// Convert QRectF to integer rounded
static inline QRect round(const QRectF& r) {
    return QRect(round(r.topLeft()), round(r.size()));
}

// Convert QSize to QSizeF
static inline QSizeF real(const QSize& sz) {
    return QSizeF(static_cast<qreal>(sz.width()), static_cast<qreal>(sz.height()));
}

// Convert QPoint to QPointF
static inline QPointF real(const QPoint& pt) {
    return QPointF(static_cast<qreal>(pt.x()), static_cast<qreal>(pt.y()));
}

// Convert QRect to QRectF
static inline QRectF real(const QRect& r) {
    return QRectF(real(r.topLeft()), real(r.size()));
}


////////////////////////////////////////////////////////////////////////////////////////////////////
// The following all convert geometry to/from the device pixel ratio of a specified widget. They
// are useful for finding coordinates in pixmaps that are set to a 1:1 pixel ratio regardless of
// the pixel ratio of the widget that renders them.
//
// Functions that "unscale" convert from scaled size (widget) to 1:1 pixel size (pixmap)
//
// Functions that "rescale" convert a 1:1 pixel size (pixmap) to the scaled size (widget)

static inline qreal unscaled(const QWidget* widget, qreal n_scaled) {
    return n_scaled/widget->devicePixelRatioF();
}
static inline qreal rescaled(const QWidget* widget, qreal n_1_to_1) {
    return n_1_to_1*widget->devicePixelRatioF();
}
static inline int unscaled(const QWidget* widget, int n_scaled) {
    return std::roundl(unscaled(widget, qreal(n_scaled)));
}
static inline int rescaled(const QWidget* widget, int n_1_to_1) {
    return std::roundl(rescaled(widget, qreal(n_1_to_1)));
}

static inline QSizeF unscaled(const QWidget* widget, const QSizeF& size_scaled) {
    const auto rat = widget->devicePixelRatioF();
    return QSizeF(size_scaled.width()/rat, size_scaled.height()/rat);
}
static inline QSize unscaled(const QWidget* widget, const QSize& size_scaled) {
    return round(unscaled(widget, real(size_scaled)));
}
static inline QSizeF rescaled(const QWidget* widget, const QSizeF& size_1_to_1) {
    const auto rat = widget->devicePixelRatioF();
    return QSizeF(size_1_to_1.width()*rat, size_1_to_1.height()*rat);
}
static inline QSize rescaled(const QWidget* widget, const QSize& size_1_to_1) {
    return round(rescaled(widget, real(size_1_to_1)));
}

static inline QPointF unscaled(const QWidget* widget, const QPointF& point_scaled) {
    const auto rat = widget->devicePixelRatioF();
    return QPointF(point_scaled.x()/rat, point_scaled.y()/rat);
}
static inline QPoint unscaled(const QWidget* widget, const QPoint& point_scaled) {
    return round(unscaled(widget, real(point_scaled)));
}
static inline QPointF rescaled(const QWidget* widget, const QPointF& point_1_to_1) {
    const auto rat = widget->devicePixelRatioF();
    return QPointF(point_1_to_1.x()*rat, point_1_to_1.y()*rat);
}
static inline QPoint rescaled(const QWidget* widget, const QPoint& point_1_to_1) {
    return round(rescaled(widget, real(point_1_to_1)));
}

}

#endif
