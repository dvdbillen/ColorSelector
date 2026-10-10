#include "mainwindow.h"

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent) {
    setCentralWidget(new gradient_demo_widget());
    setMinimumSize(500, 150);
    move(50, 50);
}

MainWindow::~MainWindow() {
}

