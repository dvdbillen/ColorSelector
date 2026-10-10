// SPDX-License-Identifier: MIT
// Copyright (c) 2026 David Billen

// Implements mock QQColorDialog interface


#ifndef cosel_qt_dialog_h
#define cosel_qt_dialog_h

#include <memory>

#include <QApplication>
#include <QBoxLayout>
#include <QColor>
#include <QColorDialog>
#include <QDebug>
#include <QPointer>
#include <QPushButton>
#include <QWidget>

#include "cosel_qt_app_defs.h"
#include "cosel_qt_palette.h"
#include "cosel_qt_selected_color.h"
#include "cosel_qt_select_widget.h"
#include "cosel_qt_tools.h"

namespace cosel {

class color_dialog : public QDialog {
    Q_OBJECT;

    static constexpr auto ShowAlphaChannel = QColorDialog::ColorDialogOption::ShowAlphaChannel;
    static constexpr auto NoButtons = QColorDialog::ColorDialogOption::NoButtons;
    static constexpr auto DontUseNativeDialog = QColorDialog::ColorDialogOption::DontUseNativeDialog;
    static constexpr auto NoEyeDropperButton = QColorDialog::ColorDialogOption::NoEyeDropperButton;
public:
    using options_t = select_widget::options_t;

    // Qt  compatible interface

    explicit color_dialog(QWidget* parent = nullptr);
    explicit color_dialog(const QColor& initial, QWidget* parent = nullptr);
    void setCurrentColor(const QColor& color);
    QColor currentColor() const;
    QColor selectedColor() const;
    void setOption(QColorDialog::ColorDialogOption option, bool on = true);
    bool testOption(QColorDialog::ColorDialogOption option) const;
    void setOptions(QColorDialog::ColorDialogOptions options);
    QColorDialog::ColorDialogOptions options() const;
    using QDialog::open;
    void open(QObject* receiver, const char* member);
    void done(int result) override;

    static QColor getColor(const QColor& initial = Qt::white,
                           QWidget* parent = nullptr,
                           const QString& title = QString(),
                           QColorDialog::ColorDialogOptions options = QColorDialog::ColorDialogOptions());
    static int customCount();
    static QColor customColor(int index);
    static void setCustomColor(int index, QColor color);
    static QColor standardColor(int index);
    static void setStandardColor(int index, QColor color);

    // Extended interface

    constexpr static size_t num_types = num_selector_types;
    explicit color_dialog(const options_t& options,
                          const selected_color_t& selection, QWidget* parent = nullptr);
    void set_cosel_options(const options_t& options);
    const options_t& cosel_options() const;
    const selected_color_t& selection() const;

signals:
    void currentColorChanged(const QColor& color);
    void colorSelected(const QColor& color);

protected:
    void setVisible(bool visible) override;

private:
    // Options used by cosel namespace
    options_t cosel_options_;
    // Color selection used by cosel
    selected_color_t selection_;
    // Only used by cancel, normally we handle our own selected_color_t signal
    unsigned selection_id = 0;
    // The widget that implements our cosel selector
    select_widget* cosel_widget = nullptr;

    // No buttons setting, (nothing corresponds in cosel_options_)
    bool no_buttons = false;
    // Options have changed. setVisible() will have to create a new dialog widget
    bool options_dirty = false;
    // Selected color, (the one finally selected by clicking "OK")
    QColor selected_color_;
    // Last color emitted (to avoid repeated emits)
    QColor last_emitted;
    // For the extended constructor that works on a specified selected_color_t, this is the startup
    // color to restore on cancel.
    QColor start_color;

    // These are needed by the open function to emulate QColorDialog
    QPointer<QObject> receiver_to_disconnect_;
    QByteArray member_to_disconnect_;

    // Static custom color palette
    static color_palette& palette() {
        static color_palette p;
        return p;
    }

    // Static widget state, (which tab is selected, etc). This also contains a string which defines
    // the custom palette requiring us to fetch it from the static palette before creating the
    // dialog.
    static select_widget::state_t& state() {
        static select_widget::state_t s;
        return s;
    }

    // Private helper to make the cosel selector widget
    select_widget* make_widget();
    // Private helper to make the full layout for the dialog including buttons
    QLayout* make_controls(select_widget* widget);
    // Private helper to be called from all contructors
    void initialize();
};


////////////////////////////////////////////////////////////////////////////////////////////////////
// Qt interface
////////////////////////////////////////////////////////////////////////////////////////////////////

inline color_dialog::color_dialog(QWidget* parent) : color_dialog(QColor(255, 255, 255), parent) {}

inline color_dialog::color_dialog(const QColor& initial, QWidget* parent) :
    QDialog(parent),
    selection_(std::make_shared<selected_color>(initial)),
    last_emitted(initial),
    start_color(initial) {

    initialize();
}

inline void color_dialog::setCurrentColor(const QColor& color) {
    if(color == selection_->qcolor()) {
        return;
    }

    // This notifies any classes using our selection class. That includes this class which will
    // emit the Qt signal.
    selection_->set_qcolor(color, -1);
}

inline QColor color_dialog::currentColor() const {
    return selection_->qcolor();
}

inline QColor color_dialog::selectedColor() const {
    return selected_color_;
}

inline void color_dialog::setOption(QColorDialog::ColorDialogOption option, bool on) {
    switch(option) {
        case ShowAlphaChannel:
            cosel_options_.use_alpha = on;
            break;

        case NoButtons:
            no_buttons = on;
            break;

        case DontUseNativeDialog:
            break;

        case NoEyeDropperButton:
            cosel_options_.use_screen_picker = !on;
            break;
    }

    options_dirty = true;
}

inline bool color_dialog::testOption(QColorDialog::ColorDialogOption option) const {
    switch(option) {
        case ShowAlphaChannel:
            return cosel_options_.use_alpha;

        case NoButtons:
            return no_buttons;

        case DontUseNativeDialog:
            return false;

        case NoEyeDropperButton:
            return !cosel_options_.use_screen_picker;
    }
}

inline void color_dialog::setOptions(QColorDialog::ColorDialogOptions options) {
    setOption(ShowAlphaChannel, options.testFlag(ShowAlphaChannel));
    setOption(NoButtons, options.testFlag(NoButtons));
    setOption(DontUseNativeDialog, options.testFlag(DontUseNativeDialog));
    setOption(NoEyeDropperButton, options.testFlag(NoEyeDropperButton));
}

inline QColorDialog::ColorDialogOptions color_dialog::options() const {
    constexpr auto zero = QColorDialog::ColorDialogOption(0); // NOLINT
    return
        (testOption(ShowAlphaChannel)? ShowAlphaChannel: zero) |
        (testOption(NoButtons)? NoButtons: zero) |
        (testOption(DontUseNativeDialog)? DontUseNativeDialog: zero) |
        (testOption(NoEyeDropperButton)? NoEyeDropperButton: zero);
}

inline void color_dialog::open(QObject* receiver, const char* member) {
    if(receiver_to_disconnect_ != nullptr) {
        disconnect(this, SIGNAL(colorSelected(QColor)),
                   receiver_to_disconnect_, member_to_disconnect_.constData());
        receiver_to_disconnect_ = nullptr;
        member_to_disconnect_.clear();
    }

    connect(this, SIGNAL(colorSelected(QColor)), receiver, member);
    receiver_to_disconnect_ = receiver;
    member_to_disconnect_ = member;
    QDialog::open();
}

inline void cosel::color_dialog::done(int result) {
    if(result == QDialog::Accepted) {

        // OK

        selected_color_ = selection_->qcolor();
        emit colorSelected(selected_color_);
    } else {

        // CANCEL

        selected_color_ = QColor();

        // Unlike a normal QColorDialog, our selected_color is a discrete class and other
        // application components might be using it. We therefore saved the current color on
        // construction and need to restore it on cancel.
        if(start_color.isValid()) {
            selection_->set_qcolor(start_color, selection_id);
        }
    }

    QDialog::done(result);

    if(receiver_to_disconnect_) {
        disconnect(this, SIGNAL(colorSelected(QColor)),
                   receiver_to_disconnect_, member_to_disconnect_.constData());
        receiver_to_disconnect_ = nullptr;
        member_to_disconnect_.clear();
    }
}

inline QColor color_dialog::getColor(const QColor& initial, QWidget* parent, const QString& title,
                                     QColorDialog::ColorDialogOptions options) {
    color_dialog dlg(initial, parent);

    if(!title.isEmpty()) {
        dlg.setWindowTitle(title);
    }

    dlg.setOptions(options);
    dlg.exec();
    return dlg.selectedColor();
}

inline int color_dialog::customCount() {
    return int(palette_custom_size);
}

inline QColor color_dialog::customColor(int index) {
    if(index < 0 || index >= customCount()) {
        return QColor();
    }

    return qt_from_rgb(palette().get(palette_standard_size + index).display_color());
}

inline void color_dialog::setCustomColor(int index, QColor color) {
    if(index < 0 || index >= customCount()) {
        return;
    }

    palette().set(palette_standard_size + index, qt_to_rgb(color));
}

inline QColor color_dialog::standardColor([[maybe_unused]] int index) {
    return QColor();
}

inline void color_dialog::setStandardColor([[maybe_unused]] int index, [[maybe_unused]] QColor
        color) {
    return;
}


////////////////////////////////////////////////////////////////////////////////////////////////////
// Extended interface
////////////////////////////////////////////////////////////////////////////////////////////////////

inline color_dialog::color_dialog(const options_t& options, const selected_color_t& selection,
                                  QWidget* parent) :
    QDialog(parent),
    cosel_options_(options),
    selection_(selection),
    options_dirty(true),
    last_emitted(selection->qcolor()),
    start_color(selection->qcolor()) {

    initialize();
}

inline void color_dialog::set_cosel_options(const options_t& options) {
    cosel_options_ = options;
    options_dirty = true;
}

inline const selected_color_t& color_dialog::selection() const {
    return selection_;
}

inline const color_dialog::options_t& color_dialog::cosel_options() const {
    return cosel_options_;
}


////////////////////////////////////////////////////////////////////////////////////////////////////
// Private helpers
////////////////////////////////////////////////////////////////////////////////////////////////////

inline void color_dialog::initialize() {
    selection_id = selection_->assign_id();

    connect(selection_.get(), &selected_color::changed, this, [this](unsigned id) {
        if(id == selection_id) {
            return;
        }

        const auto c = selection_->qcolor();

        if(c != last_emitted) {
            last_emitted = c;
            emit currentColorChanged(c);
        }
    });
}

inline void color_dialog::setVisible(bool visible) {
    if(!visible) {
        QDialog::setVisible(false);
        return;
    }

    if(options_dirty || cosel_widget == nullptr) {
        auto* old = layout();

        if(old != nullptr) {
            // reparent old layout of the dialog then go out of scope to destroy it
            QWidget sink;
            sink.setLayout(old);
        }

        cosel_widget = make_widget();
        setLayout(make_controls(cosel_widget));
        options_dirty = false;
    }

    QDialog::setVisible(true);
}

inline select_widget* color_dialog::make_widget()  {
    // These are the defaults, but if they are specified in the state it overrides
    auto type = selector_type_t::hsv;
    auto twod = twod_type_t::grid;

    // The calls to set/get custom colors use the palette(), but the widget initializes those
    // colors based on a string representation that is in the state() data passed to its ctor.
    const auto custom_colors = palette().custom_color_string();
    state().custom_colors = custom_colors;

    auto* w = new select_widget(cosel_options_, type, twod, selection_, state());

    // This is emitted when a tab is selected or custom colors are modified.
    connect(w, &cosel::select_widget::state_changed,
    this, [](const cosel::select_widget::state_t& st) {
        state() = st;
        // See comment above about the custom colors state/palette handling.
        palette().set_custom_color_string(st.custom_colors);

    });

    w->setMinimumWidth(640);

    return w;
}

inline QLayout* color_dialog::make_controls(select_widget* widget)  {

    // Layout the dialog contents

    auto* dialog_layout = new QVBoxLayout();
    dialog_layout->addWidget(widget);

    if(no_buttons) {
        return dialog_layout;
    }

    auto* ok = new QPushButton(tr("OK"));
    auto* cancel = new QPushButton(tr("Cancel"));

    connect(ok, &QPushButton::clicked, this, [this]() {
        accept();
    });

    connect(cancel, &QPushButton::clicked, this, [this]() {
        reject();
    });

    auto* button_layout = new QHBoxLayout();
    button_layout->addStretch();
    button_layout->addWidget(ok);
    button_layout->addWidget(cancel);

    dialog_layout->addLayout(button_layout);

    return dialog_layout;
}



} // namespace cosel

#endif // cosel_qt_dialog_h

