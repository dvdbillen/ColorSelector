// SPDX-License-Identifier: MIT
// Copyright (c) 2026 David Billen

// Handle selection of a color from anywhere on the display.
// Currently this code has no dependency on the rest of the codebase.

// TODO: More comments

#ifndef cosel_qt_screen_picker_h
#define cosel_qt_screen_picker_h

#include <memory>
#include <QApplication>
#include <QColor>
#include <QCursor>
#include <QEventLoop>
#include <QGuiApplication>
#include <QImage>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QPixmap>
#include <QScreen>
#include <QTimer>
#include <QWidget>
#include <QWindow>

namespace cosel {
namespace details {


// This is a small, invisible widget that tracks the mouse and stays beneath it. It allows the
// picker to maintain a crosshair cursor and prevents click pass-through on secondary display
// monitors.
constexpr int block_widget_size = 33;
class mouse_block_widget : public QWidget {
public:
    mouse_block_widget() {
        setWindowFlags(Qt::Tool | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint |
                       Qt::WindowDoesNotAcceptFocus);
        setAttribute(Qt::WA_ShowWithoutActivating);
        setWindowOpacity(0.01);
        setFixedSize(block_widget_size, block_widget_size);
        connect(&timer, &QTimer::timeout, this, [this]() {
            const auto pos = QCursor::pos();

            if(pos == last_pos) {
                return;
            }

            setCursor(Qt::CrossCursor);

            move(pos.x() - block_widget_size/2, pos.y() - block_widget_size/2);
            last_pos = pos;
        });
        timer.setTimerType(Qt::PreciseTimer);
        timer.start(4);
        show();
    }
private:
    QTimer timer;
    QPoint last_pos;
};


// This is the main widget used for selecting from the display. It overlays the display that the
// specified container widget is contained in.
class screen_picker_overlay : public QWidget {
public:
    explicit screen_picker_overlay(QEventLoop& event_loop_, QWidget* container);
    QColor selection() const {
        return selection_;
    }
protected:
    void mouseMoveEvent(QMouseEvent* e) override;
    void mousePressEvent(QMouseEvent* e) override;
    void keyPressEvent(QKeyEvent* e) override;
    void paintEvent(QPaintEvent* event) override;
private:
    constexpr static QSize work_size = QSize(320, 140);
    constexpr static QSize swatch_size = QSize(64, 28);

    // Get color from global screen pos
    QColor sample_at(const QPoint& global_pos);
    // Respond to a new cursor position
    void hover_at(const QPoint& global_pos);
    // Stop selecting
    void stop();

    // Passed to ctor so this class can stop the event loop
    QEventLoop& event_loop;
    // Current selection
    QColor selection_ = QColor(0,0,0);
    // Rectamgle of specified container widget
    QRect container_rect;
    // Swatch and other display area
    QRect work_rect;
    // Screen origin of start display
    QPoint screen_origin;
    // Screen shot of start display
    QImage screen_shot;
    // Instance of blocker
    std::unique_ptr<mouse_block_widget> blocker;
    // Sample timer avoid small issues with only responding to move moves
    QTimer hover_timer;
    // Last position sampled, used to lighten sampling activity when mouse is parked
    QPoint last_sample;
};

inline screen_picker_overlay::screen_picker_overlay(QEventLoop& event_loop_, QWidget* container) :
    QWidget(nullptr),
    event_loop(event_loop_) {
    QScreen* screen = container->screen();

    if(screen == nullptr) {
        screen = QGuiApplication::screenAt(container->mapToGlobal(container->rect().center()));
    }

    if(screen == nullptr) {
        screen = QGuiApplication::primaryScreen();
    }

    screen_origin = screen->geometry().topLeft();
    screen_shot = screen->grabWindow(0).toImage();
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Window);
    setWindowModality(Qt::ApplicationModal);
    setAttribute(Qt::WA_TranslucentBackground, false);
    setMouseTracking(true);
    setStyleSheet(QStringLiteral("background: transparent;"));
    setGeometry(screen->geometry());
    show();

    if(windowHandle() != nullptr) {
        windowHandle()->setScreen(screen);
    }

    // TODO: My understanding is showFullScreen is not reliable on all versions of Linux
    showFullScreen();

    setGeometry(screen->geometry());
    activateWindow();
    raise();
    setFocus();

    QTimer::singleShot(0, this, [this, container]() {

        // Defer this code so it is executed with a functional, sized widget. (Might be belt and
        // suspenders since we activated and raised it).

        grabMouse();
        grabKeyboard();
        setCursor(Qt::CrossCursor);
        QGuiApplication::setOverrideCursor(Qt::CrossCursor);
        container_rect = QRect(container->mapToGlobal(QPoint(0, 0)), container->size());
        const auto local_center = container_rect.center() - screen_origin;
        const auto work_x = local_center.x() - work_size.width() / 2;
        const auto work_y = local_center.y() - work_size.height() / 2;
        work_rect = QRect(QPoint(work_x, work_y), work_size);
        hover_at(QCursor::pos());
        update();
    });

    // Setup the blocker
    blocker = std::make_unique<mouse_block_widget>();

    // Setup polling
    connect(&hover_timer, &QTimer::timeout, this, [this]() {
        hover_at(QCursor::pos());
    });
    hover_timer.start(16);
}

inline void screen_picker_overlay::stop() {
    QGuiApplication::restoreOverrideCursor();
    releaseMouse();
    releaseKeyboard();
    event_loop.exit();
}

inline void screen_picker_overlay::paintEvent([[maybe_unused]] QPaintEvent* event) {
    QPainter p(this);
    p.drawImage(rect(), screen_shot);

    if(work_rect.isNull()) {
        return;
    }

    p.setPen(Qt::NoPen);
    p.setBrush(QColor(0, 0, 0, 210));
    const auto rounding = work_rect.height() / 6;
    p.drawRoundedRect(work_rect, rounding, rounding);
    p.setPen(QColor(255, 245, 220));
    p.drawText(work_rect.adjusted(0, 5, 0, 0), Qt::AlignTop | Qt::AlignHCenter,
               tr("Click screen to select color\nPress Esc to cancel"));

    auto color = selection_;
    if(!color.isValid()) {
        color = QColor(0,0,0);
    }

    const auto swatch_x = work_rect.center().x() - swatch_size.width() / 2;
    const auto swatch_y = work_rect.center().y() - swatch_size.height() / 2;
    const auto swatch_rect = QRect(QPoint(swatch_x, swatch_y), swatch_size);
    p.setPen(QColor(255, 255, 255));
    p.setBrush(color);
    p.drawRect(swatch_rect);
}

inline QColor screen_picker_overlay::sample_at(const QPoint& global_pos) {

    // Note that we do not hide the blocker. That means it's possible that samples are a very tiny
    // amount inaccurate (since the blocker is not 100% transparent). However, this only affects
    // the preview. The blocker is deleted before taking the final click sample.

    if(QRect(screen_origin, size()).contains(global_pos)) {

        // Pos is inside start screen. Take sample from screen shot
        const auto dpr = screen_shot.devicePixelRatioF();
        const auto logical_offset = global_pos - screen_origin;
        const auto x = qRound(static_cast<double>(logical_offset.x()) * dpr);
        const auto y = qRound(static_cast<double>(logical_offset.y()) * dpr);
        return screen_shot.pixelColor(x, y);
    }

    // Pos is outside start screen. Take sameple from screen.

    const auto screen = QGuiApplication::screenAt(global_pos);

    if(screen == nullptr) {
        return QColor();
    }

    const auto shot = screen->grabWindow(0,
                                         global_pos.x() - screen->geometry().x(),
                                         global_pos.y() - screen->geometry().y(),
                                         1, 1);

    return shot.toImage().pixelColor(0, 0);
}

inline void screen_picker_overlay::hover_at(const QPoint& global_pos) {
    if(global_pos == last_sample) {
        return;
    }

    last_sample = global_pos;
    selection_ = sample_at(global_pos);

    if(!work_rect.isNull()) {
        update(work_rect);
    }
}

inline void screen_picker_overlay::mouseMoveEvent([[maybe_unused]] QMouseEvent* e) {
    if(cursor().shape() != Qt::CrossCursor) {
        setCursor(Qt::CrossCursor);
    }

    hover_at(QCursor::pos());
}

inline void screen_picker_overlay::mousePressEvent(QMouseEvent* e) {
    if(e->button() == Qt::LeftButton) {
        blocker.reset();
        selection_ = sample_at(QCursor::pos());
        stop();
    }
}

inline void screen_picker_overlay::keyPressEvent(QKeyEvent* e) {
    if(e->key() == Qt::Key_Escape) {
        selection_ = QColor();
        stop();
    }
}
} // namespace details

inline QColor pick_screen_color(QWidget* container) {
    QEventLoop loop;
    details::screen_picker_overlay overlay(loop, container);
    loop.exec();
    return overlay.selection();
}
} // namespace cosel
#endif // cosel_qt_screen_picker_h
