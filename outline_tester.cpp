#include "outline_tester.h"

#include <QPushButton>

#include <QColor>
#include <QDebug>
#include <QDialog>
#include <QBoxLayout>
#include <QPainter>
#include <QString>
#include <QWidget>

#include "color_select/cosel_gamut_outline.h"

#include "color_select/cosel_geometry.h"
#include "color_select/cosel_model.h"
#include "color_select/cosel_qt_custom_slider.h"
#include "color_select/cosel_qt_tools.h"

using namespace cosel;

class outline_render_widget : public QWidget {
    gamut_outline outline_;
    luv color_;
    QColor bg;
    QColor fg;
public:
    outline_render_widget() {
        std::tie(bg, fg) = get_bg_fg_colors(this);
        setMinimumSize(512, 512);
    }

    gamut_outline& outline() {
        return outline_;
    }

    const luv& color() const {
        return color_;
    }

    inline QPointF to_pos(const point<double>& p) const {
        const auto w = width()*.5;
        const auto h = height()*.5;
        const auto scale = 1.5;

        return QPointF(w + p.x()*scale, h + p.y()*scale);
    }

    void paintEvent([[maybe_unused]] QPaintEvent* event) override {
        if(outline_.empty()) {
            return;
        }

        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing, true);

        p.setPen(with_alpha(QColor(fg), 64));
        p.drawLine(QPointF(width()*.5, 0), QPointF(width()*.5, height()));
        p.drawLine(QPointF(0, height()*.5), QPointF(width(), height()*.5));

        p.setPen(QColor(fg));

        const auto& vec = outline_.luv_outline();

        auto last = to_pos(vec[0].uv());

        for(size_t i = 1; i < vec.size(); ++i) {
            auto pos = to_pos(vec[i].uv());

            p.drawLine(last, pos);
            last = pos;
        }

        static constexpr double ray_length = 200;
        const auto ray = color_.uv().normalized()*ray_length;
        p.setPen(QColor(255, 0, 0));

        p.drawLine(to_pos(point<double>(0, 0)), to_pos(ray));

        p.setPen(fg);
        p.drawEllipse(to_pos(color_.uv()), 5, 5);
    }

    void set_L(double L) {
        outline_.set(L);
        const auto uv = outline_.max_gamut(color_.uv());
        color_ = luv(L, uv);
        update();
    }

    void set_color(double n) {
        const auto uv = outline_.pos_chromacity_to_uv(n, 1);
        //qDebug() << "Pos " << n << ", uv=" << uv.x() << ", " << uv.y();
        color_ = luv(outline_.L(), uv);
        update();
    }
};



class outline_widget : public QWidget {

    outline_render_widget* renderer = nullptr;

    custom_slider* hue = nullptr;
    custom_slider* level = nullptr;
public:
    outline_widget() :
        QWidget(nullptr, Qt::Window)

    {
        renderer = new outline_render_widget();

        hue = new custom_slider(0, 24, 24);
        level = new custom_slider(.5, 24, 24);

        connect(hue, &custom_slider::changed, this, [this]() {
            renderer->set_color(hue->value());
        });

        connect(level, &custom_slider::changed, this, [this]() {
            renderer->set_L(2 + level->value()*(100 - 2*2));
        });

        auto* layout = new QVBoxLayout();

        layout->addWidget(renderer);
        layout->addWidget(hue);
        layout->addWidget(level);

        setLayout(layout);

        renderer->set_L(50);
        renderer->set_color(0);
    }
};


class tester {

};

void outline_tester(QWidget* parent) {
    QDialog* dialog = new QDialog(parent);

    auto* widget = new outline_widget();
    auto* layout = new QVBoxLayout();
    layout->addWidget(widget);

    dialog->setLayout(layout);
    dialog->setWindowTitle(QObject::tr("Outline Tester"));

    dialog->exec();

    delete dialog;

}
