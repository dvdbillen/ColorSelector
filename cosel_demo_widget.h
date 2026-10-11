#ifndef COSEL_DEMO_WIDGET_H
#define COSEL_DEMO_WIDGET_H

#include <QMainWindow>
#include <QWidget>

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget* parent = nullptr);
};


#endif // COSEL_DEMO_WIDGET_H
