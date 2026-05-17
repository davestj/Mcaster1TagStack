// main.cpp — Mcaster1 TagStack desktop entry point.
//
// Uses Qt's default native style (no QStyle override, no QPalette tweaks,
// no stylesheet). On macOS that's the native Cocoa look, on Linux Fusion,
// on Windows native. Theme work is out of scope for this phase.

#include "MainWindow.h"

#include <QApplication>

int main(int argc, char** argv) {
    QApplication app(argc, argv);
    QApplication::setApplicationName("Mcaster1 TagStack");
    QApplication::setOrganizationName("Mcaster1");
    QApplication::setOrganizationDomain("mcaster1.com");

    MainWindow w;
    w.show();
    return app.exec();
}
