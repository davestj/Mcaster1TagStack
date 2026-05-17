// main.cpp — Mcaster1TagStack desktop entry point (Phase 1 stub).
// Phase 3 will replace this with the real Library + ICY 2.2 windows.

#include <QApplication>
#include <QLabel>
#include <QMainWindow>

int main(int argc, char** argv) {
    QApplication app(argc, argv);
    QMainWindow w;
    w.setWindowTitle("Mcaster1 TagStack");
    auto* label = new QLabel("TagStack v2 — Phase 1 skeleton.\n"
                             "Phase 3 will replace this with the real UI.", &w);
    label->setAlignment(Qt::AlignCenter);
    w.setCentralWidget(label);
    w.resize(720, 420);
    w.show();
    return app.exec();
}
