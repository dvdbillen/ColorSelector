#include "mainwindow.h"

#include <iostream>

#include <QApplication>
#include <QDebug>
#include <QPalette>
#include <QStyleFactory>
#include <QFile>

#include "color_select/cosel.h"
#include "color_select/cosel_math.h"
#include "color_select/cosel_model_hsx.h"
#include "color_select/cosel_gradient.h"
#include "color_select/cosel_gamut_generator.h"

#include <cfenv>

using namespace cosel;

void output_gamuts() {
    constexpr triplet r{0.6800, 0.3200, 0.209492};
    constexpr triplet g{0.2650, 0.6900, 0.721595};
    constexpr triplet b{0.1500, 0.0600, 0.068913};
    constexpr triplet w{0.3140, 0.3510, 1.0};

    gamut gam{r, g, b, w};

    generate_gamut_class(gam, "dci_p3_gamut");
}

int main(int argc, char* argv[]) { // NOLINT
    QApplication app(argc, argv);

#if QT_VERSION >= QT_VERSION_CHECK(6, 7, 0)
    app.setStyle(QStyleFactory::create(QStringLiteral("windows11")));
#else
    constexpr bool dark = true;

    if constexpr(dark) {
        // Set Fusion style
        app.setStyle(QStyleFactory::create(QStringLiteral("Fusion")));

        // Define a dark palette
        QPalette darkPalette;
        darkPalette.setColor(QPalette::Window, QColor(53, 53, 53));
        darkPalette.setColor(QPalette::WindowText, Qt::white);
        darkPalette.setColor(QPalette::Base, QColor(25, 25, 25));
        darkPalette.setColor(QPalette::AlternateBase, QColor(53, 53, 53));
        darkPalette.setColor(QPalette::ToolTipBase, Qt::black);
        darkPalette.setColor(QPalette::ToolTipText, Qt::white);
        darkPalette.setColor(QPalette::Text, Qt::white);
        darkPalette.setColor(QPalette::Button, QColor(53, 53, 53));
        darkPalette.setColor(QPalette::ButtonText, Qt::white);
        darkPalette.setColor(QPalette::BrightText, Qt::red);
        darkPalette.setColor(QPalette::Link, QColor(42, 130, 218));
        darkPalette.setColor(QPalette::Highlight, QColor(42, 130, 218));
        darkPalette.setColor(QPalette::HighlightedText, Qt::black);
        // Apply the palette
        app.setPalette(darkPalette);
    }

#endif

#if 0
    const auto sat = 1;

    constexpr int rez = 10000;
    constexpr int slack = 10;
    const auto inc = 1.0/rez;

    double worst_error = 0;
    double worst_error_pos = 0;
    double worst_h = 0;
    double worst_h2 = 0;

    for(int i = 0; i < rez - slack; ++i) {
        const auto pos = i*inc;
        const auto hsv_c = hsv2(pos, sat, 1);
        const auto rgb_c = hsv_c.to_rgb();
        const auto hsv_c2 = hsv2(rgb_c);

        const auto error = std::abs(hsv_c.h() - hsv_c2.h());

        if(error > worst_error) {
            worst_error = error;
            worst_error_pos = pos;
            worst_h = hsv_c.h();
            worst_h2 = hsv_c2.h();
        }
    }

    qDebug() << "worst error =" << worst_error*360 << ", pos =" << worst_error_pos*360 <<
             ", worst hsv =" << worst_h << ", worst hsv2 = " << worst_h2;
#endif

    MainWindow w;
    w.show();

    //output_gamuts();

    return app.exec();
}
