
#include <iostream>

#include <QApplication>
#include <QPalette>
#include <QStyleFactory>

#include "cosel_demo_widget.h"

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

    MainWindow w;
    w.show();

    return app.exec();
}
