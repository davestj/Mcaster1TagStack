// main.cpp — Mcaster1 TagStack desktop entry point.
//
// Lifecycle:
//   1. Construct ApiClient, point it at https://tagstack.mcaster1.com:9890.
//   2. If a token is cached in QSettings, jump straight to MainWindow.
//   3. Otherwise show LoginDialog modally; on Accept proceed to MainWindow.
//   4. From MainWindow, "Sign out" closes the window — main.cpp loops back
//      to step 2 so the next session starts cleanly.
//
// Default Qt native theme is used — no QStyle/QPalette/stylesheet overrides.

#include "ApiClient.h"
#include "LoginDialog.h"
#include "MainWindow.h"

#include <QApplication>
#include <QDialog>

int main(int argc, char** argv) {
    QApplication app(argc, argv);
    QApplication::setApplicationName("Mcaster1 TagStack");
    QApplication::setOrganizationName("Mcaster1");
    QApplication::setOrganizationDomain("mcaster1.com");

    ApiClient api;

    while (true) {
        if (!api.hasToken()) api.restoreToken();
        if (!api.hasToken()) {
            LoginDialog dlg(&api);
            if (dlg.exec() != QDialog::Accepted) {
                return 0;  // user cancelled / quit
            }
        }

        MainWindow w(&api);
        w.show();
        int rc = app.exec();
        if (rc != 0) return rc;
        if (!api.hasToken()) continue;  // signed out — loop back to login
        return 0;                       // window closed while still signed in
    }
}
