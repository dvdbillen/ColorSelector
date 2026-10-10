#ifndef SELECT_WIDGET_H
#define SELECT_WIDGET_H

#include <QWidget>

#include "color_select/cosel_model.h"

class color_select_info {
    using rgb = cosel::rgb;

    rgb selection_;
    bool fixed_ = false;
    int fixed_pos_ = 0;
public:
    color_select_info() = default;
    explicit color_select_info(rgb selection) :
        selection_(selection) {}

    const rgb& selection() const {
        return selection_;
    }

    bool fixed() const {
        return fixed_;
    }

    int fixed_pos() const {
        return fixed_pos_;
    }

    void set_selection(const rgb& selection) {
        selection_ = selection;
    }

    void set_fixed(bool yesno) {
        fixed_ = yesno;
    }

    void set_fixed_pos(int pos) {
        fixed_pos_ = pos;
    }
};

class color_select_widget : public QWidget {
    Q_OBJECT
public:
    explicit color_select_widget(QWidget* parent = nullptr);

signals:

};

#endif // SELECT_WIDGET_H
