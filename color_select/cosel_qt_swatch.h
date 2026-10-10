// SPDX-License-Identifier: MIT
// Copyright (c) 2026 David Billen

// Color swatch widget

// TODO: More comments

#ifndef cosel_qt_swatch_h
#define cosel_qt_swatch_h

#include <cmath>
#include <memory>

#include <QApplication>
#include <QColor>
#include <QDrag>
#include <QImage>
#include <QMimeData>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPixmap>
#include <QSize>
#include <QWidget>

#include "cosel_geometry.h"
#include "cosel_model.h"
#include "cosel_qt_app_defs.h"
#include "cosel_qt_tools.h"
#include "cosel_model_hsx.h"


namespace cosel {

class color_swatch : public QWidget {
    Q_OBJECT;

public:
    // Construct with xyz
    explicit color_swatch(const xyz& color_,  double corner_rounding_,
                          bool droppable = true);

    // Construct with rgb
    explicit color_swatch(const rgb& color_, double corner_rounding_, bool droppable = true) :
        color_swatch(xyz(color_), corner_rounding_, droppable) {}

    // Construct empty
    explicit color_swatch(double corner_rounding_, bool droppable = true) :
        color_swatch(rgb(), corner_rounding_, droppable) {
        empty_ = true;
    }

    ~color_swatch() override = default;

    void set(const xyz& color_) {
        rgb_color = rgb(color_).clamped_gamut();
        color = qt_from_rgb(rgb_color);
        empty_ = false;

        update();
    }

    void clear() {
        empty_ = true;
        update();
    }

    rgb get() const {
        return rgb_color;
    }

    bool empty() const {
        return empty_;
    }

    void set_alpha(double n) {
        alpha_ = clamp(n);

        update();
    }

    double alpha() const {
        return alpha_;
    }

    bool selected() const {
        return selected_;
    }

    bool droppable() const {
        return droppable_;
    }

    void select(bool yesno) {
        if(selected_ != yesno) {
            selected_ = yesno;
            update();
        }
    }

protected:
    void paintEvent([[maybe_unused]] QPaintEvent* event) override;

    // clicking signals, but doesn't select
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;

    void dragEnterEvent(QDragEnterEvent* event) override;

    void dropEvent(QDropEvent* event) override;
private:
    bool empty_ = true;
    bool show_out_of_gamut = true;
    bool droppable_ = true;
    rgb rgb_color;
    QColor color;
    double alpha_ = 1.0;
    bool selected_ = false;
    double corner_rounding = 0;
    QColor bg;
    QColor fg;
    QPoint drag_pos;
    const QString mime_type;

    static std::weak_ptr<QImage> icon_image_ref;
    std::shared_ptr<QImage> icon_image;

    static std::weak_ptr<QPixmap> checkerboard_pixmap_ref;
    QPixmap checkerboard_pixmap_;

    QPixmap icon_pixmap();
    const QPixmap& checkerboard_pixmap();
signals:
    void clicked();
    void dropped();
};

inline std::weak_ptr<QImage> color_swatch::icon_image_ref;

inline color_swatch::color_swatch(const xyz& color_, double corner_rounding_,
                                  bool droppable) :
    empty_(false),
    droppable_(droppable),
    corner_rounding(corner_rounding_),
    mime_type(QStringLiteral("cosel/x-cosel-color-rgb16")) {

    set(color_);
    setMinimumSize(4, 4); // Division is used with width and height

    std::tie(bg, fg) = get_bg_fg_colors(this);
    fg = with_alpha(fg, 128);

    setAcceptDrops(droppable);
}

inline void color_swatch::paintEvent([[maybe_unused]] QPaintEvent* event) {
    QPainter p(this);

    QColor fill_color = color;
    QRect r(0, 0, width() - 1, height() - 1);

    if(isone(alpha_) && corner_rounding <= 0 && !empty_) {
        if(!empty_) {
            p.fillRect(r, fill_color);
        }
        return;
    }

    const auto rounding = std::min(r.width(), r.height())*corner_rounding*.5;
    p.setRenderHint(QPainter::Antialiasing, true);

    QRectF rf(r.x() + 1.5, r.y() + 1.5, r.width() - 1.5, r.height() - 1.5);

    if(!empty_ && !isone(alpha_)) {
        QPainterPath path;
        path.addRoundedRect(rf, rounding, rounding, Qt::AbsoluteSize);

        p.setPen(Qt::NoPen);
        p.setBrush(QBrush(checkerboard_pixmap()));
        p.drawPath(path);
    }

    p.setBrush(empty_? bg: fill_color);

    p.setPen(empty_? QPen(with_alpha(fg, 48)): Qt::NoPen);

    p.setOpacity(alpha_);
    p.drawRoundedRect(rf, rounding, rounding, Qt::AbsoluteSize);

    if(selected_) {
        rf = QRectF(r.x() + 1.5, r.y() + 1.5, r.width() - 2.5, r.height() - 2.5);
        p.setPen(bg);
        p.setBrush(Qt::NoBrush);
        p.drawRoundedRect(rf, rounding, rounding, Qt::AbsoluteSize);
    }
}

inline void color_swatch::mousePressEvent(QMouseEvent* event) {
    if(empty_) {
        return;
    }

    if(event->button() == Qt::LeftButton) {
        drag_pos = event->pos();
        emit clicked();
    }
}

inline void color_swatch::mouseMoveEvent(QMouseEvent* event) {
    if(empty_) {
        return;
    }

    if(!(event->buttons() & Qt::LeftButton)) {
        return;
    }

    if((event->pos() - drag_pos).manhattanLength() < QApplication::startDragDistance()) {
        return;
    }

    // Set internal mime type which is 16-bit gamma adjusted rgb

    uint16_t r = 0;
    uint16_t g = 0;
    uint16_t b = 0;
    rgb_color.to_bits<16>(r, g, b);

    QByteArray encoded;
    QDataStream stream(&encoded, QIODevice::WriteOnly);
    stream.setVersion(QDataStream::Qt_6_0);

    stream << r << g << b;

    auto* mimeData = new QMimeData;
    mimeData->setData(mime_type, encoded);

    // Also set external color data for compatibility with other apps

    mimeData->setColorData(QVariant(color));

    auto* drag = new QDrag(this);
    drag->setMimeData(mimeData);
    drag->setPixmap(icon_pixmap());

    drag->exec(Qt::CopyAction | Qt::MoveAction);
}

inline void color_swatch::dragEnterEvent(QDragEnterEvent* event) {
    if(!droppable_) {
        return;
    }

    if(event->mimeData()->hasFormat(mime_type)) {
        event->acceptProposedAction();
    }
}

inline void color_swatch::dropEvent(QDropEvent* event) {
    const QMimeData* mime = event->mimeData();

    if(mime->hasFormat(mime_type)) {

        // Internal color dropped (16-bit gamma adjusted RGB)

        QByteArray encoded = mime->data(mime_type);
        QDataStream stream(encoded);
        stream.setVersion(QDataStream::Qt_6_0);

        uint16_t r = 0;
        uint16_t g = 0;
        uint16_t b = 0;
        stream >> r >> g >> b;
        rgb_color = rgb::from_bits<16>(r, g, b);
        color = qt_from_rgb(rgb_color);

    } else if(mime->hasColor()) {

        // External color dropped

        const auto qt_c = qvariant_cast<QColor>(mime->colorData());
        const auto rgb_c = qt_to_rgb(qt_c);

        // Note the assumption that the color is from the same color space.
        color = qt_c;
        rgb_color = rgb_c;
    } else {
        // shouldn't happen
        return;
    }

    empty_ = false;
    emit dropped();
    update();

    event->acceptProposedAction();
}

inline QPixmap color_swatch::icon_pixmap()  {

    if(icon_image) {
        return QPixmap::fromImage(*icon_image.get());
    }

    if(!icon_image_ref.expired()) {
        icon_image = icon_image_ref.lock();
        return QPixmap::fromImage(*icon_image.get());
    }

    // The drag icon would ideally be a simple square that matches the color we're dragging.
    // However, for some reason, the fill color does not show as the color in the pixmap when used
    // as a drag cursor on some platforms. Render a small color wheel instead.

    constexpr int icon_size = 24;
    constexpr int scale = 4;
    constexpr int icon_scaled = icon_size*scale;

    auto icon = QImage(icon_scaled, icon_scaled, QImage::Format_RGBA8888);
    icon.fill(qRgba(0, 0, 0, 0));


    // radius is also the center coordinate
    const auto radius = icon_scaled/2.0;
    const auto radius_squared = radius*radius;

    auto* NOALIAS pixels = reinterpret_cast<uint32_t*>(icon.bits());

    const auto pi2_recip = 1/(pi()*2);

    for(int y = 0; y < icon_scaled; ++y) {
        const auto py = y - radius;

        for(int x = 0; x < icon_scaled; ++x) {
            const auto px = x - radius;

            if(py*py + px*px > radius_squared) {
                continue;
            }

            const auto hue = std::atan2(-py, px)*pi2_recip;
            const auto hsv_c = hsv{hue, 1, 1};
            const auto rgb_c = hsv_c.to_rgb();
            pixels[y*icon_scaled + x] = to_image_pixel(rgb_c, 255);
        }
    }

    icon_image = std::make_shared<QImage>(
                     icon.scaled(icon_size, icon_size, Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
    icon_image_ref = icon_image;

    return QPixmap::fromImage(*icon_image.get());
}

inline const QPixmap& color_swatch::checkerboard_pixmap() {
    if(!checkerboard_pixmap_.isNull()) {
        return checkerboard_pixmap_;
    }

    checkerboard_pixmap_ = QPixmap(16, 16);
    checkerboard_pixmap_.fill(QColor(konst::alpha_checkerboard_light(), konst::alpha_checkerboard_light(),
                                     konst::alpha_checkerboard_light()));
    QPainter p(&checkerboard_pixmap_);
    p.fillRect(0, 0, 8, 8, QColor(konst::alpha_checkerboard_dark(), konst::alpha_checkerboard_dark(),
                                  konst::alpha_checkerboard_dark()));
    p.fillRect(8, 8, 8, 8, QColor(konst::alpha_checkerboard_dark(), konst::alpha_checkerboard_dark(),
                                  konst::alpha_checkerboard_dark()));
    p.end();

    return checkerboard_pixmap_;
}

} // namespace cosel


#endif // cosel_qt_swatch_h
