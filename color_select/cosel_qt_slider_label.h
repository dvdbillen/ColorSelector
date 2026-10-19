// SPDX-License-Identifier: MIT
// Copyright (c) 2026 David Billen

// Slider labels and value edits

#ifndef cosel_qt_slider_label_h
#define cosel_qt_slider_label_h

#include <QBoxLayout>
#include <QColor>
#include <QDoubleValidator>
#include <QLabel>
#include <QLineEdit>
#include <QMouseEvent>
#include <QPainter>
#include <QPaintEvent>
#include <QStackedLayout>
#include <QString>
#include <QStyle>
#include <QTimer>
#include <QWidget>

#include <cfloat>

#include "cosel_qt_app_defs.h"
#include "cosel_qt_tools.h"

namespace cosel {

class slider_label_display : public QLabel {
public:
    explicit slider_label_display(const QString& label_, QWidget* parent = nullptr) :
        QLabel(label_, parent) {
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    }
};


////////////////////////////////////////////////////////////////////////////////////////////////////

class slider_label_edit : public QWidget {
    Q_OBJECT;

public:
    slider_label_edit(double display_min, double display_max, int precision_ = default_precision,
                      QWidget* parent = nullptr);

    void set(double n) {
        if(edit != nullptr) {
            QString formatted = QString(tr("%1")).arg(n, 0, 'f', precision);
            edit->setText(formatted);
        }
    }

    // This clamps the return value so might not match the edit box
    double get() const {
        bool ok = false;
        auto value = edit->text().toDouble(&ok);

        if(ok) {
            return std::clamp(value, min, max);
        }

        return min;
    }

private:
    class slider_lineedit : public QLineEdit {
    public:
        slider_lineedit() = default;

    protected:
        void focusInEvent(QFocusEvent* event) override {
            QLineEdit::focusInEvent(event);
            defer(0, [this] { dynamic_cast<QLineEdit *>(this)->selectAll(); });
        }
    };

    constexpr static int default_precision = 1;

    double min = 0;
    double max = 100;
    int precision = default_precision;
    slider_lineedit* edit = nullptr;

signals:
    void edited(double new_val);

};

inline slider_label_edit::slider_label_edit(double display_min, double display_max, int precision_,
        QWidget* parent) :
    QWidget(parent),
    min(display_min),
    max(display_max),
    precision(precision_),
    edit(new slider_lineedit)

{
    edit->setFrame(false);
    edit->setMinimumWidth(konst::min_edit_width());
    edit->setMaximumWidth(konst::max_edit_width());
    edit->setAlignment(Qt::AlignRight);
    edit->setTextMargins(0, 0, 2, 0);

    // Setup validator
    auto* validator = new QDoubleValidator(-DBL_MAX, DBL_MAX, precision, edit);
    edit->setValidator(validator);

    // Handle post edit
    connect(edit, &QLineEdit::editingFinished, this, [this]() {
        bool ok = false;
        auto value = edit->text().toDouble(&ok);

        if(ok) {
            if(value < min) {
                value = min;
                edit->setText(QString::number(value, 'f', precision));
            } else if(value > max) {
                value = max;
                edit->setText(QString::number(value, 'f', precision));
            }
        } else {
            value = min;
            edit->setText(QString::number(value, 'f', precision));
        }

        emit edited(value);
    });

    auto* layout = new QHBoxLayout;
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(edit);
    setLayout(layout);

    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
}


} // namespace cosel



#endif // cosel_qt_slider_label_h
