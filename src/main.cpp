#include <QApplication>
#include "ui/MainWindow.h"
#include "config/settings.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName("AppGrader");
    app.setOrganizationName("AppGrader");
    app.setQuitOnLastWindowClosed(false);  /* keep alive in tray */

    AppPaths::ensureDirsExist();

    MainWindow w;
    w.show();

    return app.exec();
}
