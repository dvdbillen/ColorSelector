// SPDX-License-Identifier: MIT
// Copyright (c) 2026 David Billen

// Helper class to assist in rendering via concurrently generated images.
//
// The 'throttled' behavior is because rendering from the widget update() normally handles limiting
// rendering to only when it's needed. Since we wish to pre-render into an image concurrently, this
// has to be handled ourselves. It's accomplished by using a timer then signaling the parent widget
// when an image is ready.

// TODO: Investigate using singleton timer so this could handle all slider gradients without
// spawning multiple high precision timers.

#ifndef cosel_qt_throttled_h
#define cosel_qt_throttled_h

#if defined(__cpp_lib_semaphore) && __cpp_lib_semaphore >= 201907L
#include <semaphore>
#else
#include <mutex>
#include <condition_variable>
#include <cassert>
#endif

#include <thread>

#include <QDebug>
#include <QEvent>
#include <QImage>
#include <QObject>
#include <QPixmap>
#include <QTimer>
#include <QWidget>

#include "cosel_util.h"

namespace cosel {

namespace details {
#if defined(__cpp_lib_semaphore) && __cpp_lib_semaphore >= 201907L

using std::counting_semaphore;

#else

// Partial implementation of counting_semaphore. This was added to support C++17.

template <std::ptrdiff_t LeastMaxValue =
          std::numeric_limits<std::ptrdiff_t>::max()>
class counting_semaphore {
    static_assert(LeastMaxValue >= 0,
                  "LeastMaxValue must be non-negative");

    std::mutex              mtx_;
    std::condition_variable cv_;
    std::ptrdiff_t          count_;

public:
    static constexpr std::ptrdiff_t max() noexcept {
        return LeastMaxValue;
    }

    explicit counting_semaphore(std::ptrdiff_t desired) noexcept
        : count_(desired) {
        assert(desired >= 0 && desired <= max());
    }

    ~counting_semaphore() = default;

    counting_semaphore(const counting_semaphore&)            = delete;
    counting_semaphore& operator=(const counting_semaphore&) = delete;

    void release(std::ptrdiff_t update = 1) {
        assert(update >= 0);
        {
            std::lock_guard<std::mutex> lock(mtx_);
            assert(update <= max() - count_);
            count_ += update;
        }

        if(update == 1) {
            cv_.notify_one();
        } else {
            cv_.notify_all();
        }
    }

    void acquire() {
        std::unique_lock<std::mutex> lock(mtx_);
        cv_.wait(lock, [this] { return count_ > 0; });
        --count_;
    }
};

#endif


// This parent class is needed because a QObject can't be a template
class throttled_image_base : public QObject {
public:
    explicit throttled_image_base(QWidget* par) : QObject(par) {
    }
    ~throttled_image_base() override = default;

    QWidget* widget() {
        return static_cast<QWidget*>(parent());
    }
};

} // namespace details


// The RENDER_INFO class holds rendering information and also must provide a render_image function
// that accepts a mutable reference to a QImage as an argument and renders to it. The image is
// reused so that the rendering code can check if the size is correct (common case for repeated
// rendering) and avoid reallocating it if true. eg:
//
//      void render_image(QImage& dest);
//
// Note that a single instance of the class is used with one worker thread, so the class itself
// does not need to be concerned with concurrent use.

template<class RENDER_INFO>
class throttled_image : public details::throttled_image_base {
    static constexpr int timer_rate = 8;
    static constexpr Qt::TimerType timer_type = Qt::PreciseTimer;

public:
    // The image_one_to_one flag causes the images to be returned with a device pixel ratio
    // that matches the widget for rendering pixels exactly.
    explicit throttled_image(QWidget* par, bool image_one_to_one_) :
        details::throttled_image_base(par),
        image_one_to_one(image_one_to_one_) {

        timer.setTimerType(timer_type);
        connect(&timer, &QTimer::timeout, this, [this]() {
            poll();
        });
    }

    ~throttled_image() override {
        stop();
    }
    throttled_image(const throttled_image&) = delete;
    throttled_image(throttled_image&&) = delete;
    throttled_image& operator =(const throttled_image&) = delete;
    throttled_image& operator =(throttled_image&&) = delete;

    // Widget that uses this should call stop() from hideEvent and start() from showEvent to avoid
    // useless background processing.

    // Start the worker thread and timer
    void start();

    // Stop the worker thread and timer
    void stop() noexcept;

    // Access the most recent RENDER_INFO data passed to update.
    const RENDER_INFO& info() const {
        return info_;
    }

    // Trigger a new rendering.
    void update(const RENDER_INFO& info) {
        info_ = info;
        update();
    }
    void update(RENDER_INFO&& info) {
        info_ = std::move(info);
        update();
    }

    // Last generated image as a pixmap. This could return a null pixmap (e.g. on startup)
    const QPixmap& pixmap() const {
        return pixmap_;
    }

private:
    enum thread_state_t {
        idle,
        render,
        done
    };

    RENDER_INFO info_;
    RENDER_INFO worker_info;
    QPixmap pixmap_;
    QImage worker_image;
    bool image_one_to_one = false;

    bool need_update = true;
    std::atomic<thread_state_t> thread_state = thread_state_t::idle;
    std::atomic<bool> thread_quit = false;
    details::counting_semaphore<1> sem{0};
    std::thread worker;
    QTimer timer;
    QImage scratch_image;

    void update() {
        need_update = true;

        if(timer.isActive()) {
            poll();
        }
    }

    // Timer callback for throttled background render
    void poll();

    // Worker thread function
    void thread_func();
};

template<class INFO>
inline void throttled_image<INFO>::start() {
    if(worker.joinable()) {
        if(timer.isActive()) {
            // already started
            return;
        }

        // Worker existed but timer wasn't started. This is a coding error
        assert(false);
        stop();
        return;
    }

    thread_state = thread_state_t::idle;
    thread_quit = false;
    worker = std::thread([this]() {
        thread_func();
    });

    timer.start(timer_rate);
}

template<class INFO>
inline void throttled_image<INFO>::stop() noexcept  {
    timer.stop();

    if(worker.joinable()) {
        QT_TRY {
            thread_quit = true;
            sem.release();
            worker.join();
        } QT_CATCH(...) {
            // There's not a good way to handle this. We only caught the exception so that our
            // function wouldn't throw
            qDebug() << "Stop thread exception" << __FILE__;
        }
    }

    worker_image = QImage();
}

template<class INFO>
inline void throttled_image<INFO>::poll() {

    if(thread_state == thread_state_t::render) {
        // Still busy rendering an image so try again next time
        return;
    }

    // Everything below is concurrency safe because worker is not active

    if(thread_state == thread_state_t::done) {
        // Handle previous render complete
        pixmap_ = QPixmap::fromImage(worker_image);

        if(image_one_to_one) {
            pixmap_.setDevicePixelRatio(widget()->devicePixelRatioF());
        }

        thread_state = thread_state_t::idle;
        widget()->update();
    }

    if(need_update) {
        // Start a new render
        need_update = false;
        thread_state = thread_state_t::render;
        worker_info = info_;
        sem.release();
    }
}

template<class RENDER_INFO>
inline void throttled_image<RENDER_INFO>::thread_func() {
    while(true) {
        sem.acquire();

        if(thread_quit) {
            break;
        }

        if(thread_state == thread_state_t::render) {
            worker_info.render_image(worker_image);
            thread_state = thread_state_t::done;
        }
    }
}

} // namespace cosel

#endif // cosel_qt_throttled_h
