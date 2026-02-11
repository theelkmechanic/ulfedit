#include <QApplication>
#include "MainWindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("X16 Unilib Font Editor");
    app.setOrganizationName("x16unifontedit");

    QFont appFont = app.font();
    appFont.setPointSize(appFont.pointSize() + 2);
    app.setFont(appFont);

    MainWindow w;
    w.resize(1600, 950);
    w.show();

    if (argc > 1)
        w.openFile(QString::fromLocal8Bit(argv[1]));

    return app.exec();
}
