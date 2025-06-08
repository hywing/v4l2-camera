#include <QApplication>
#include "main_window.h"
#include "camera_view.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    MainWindow mainWindow;
    mainWindow.show();

    return app.exec();
}

#include "main.moc"
