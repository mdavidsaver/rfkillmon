#include <QApplication>
#include <QSystemTrayIcon>
#include <QDebug>

#include <tray.h>

int main(int argc, char *argv[])
{
    Q_INIT_RESOURCE(icons);

    qRegisterMetaType<rfStatesPtr>("rfStatesPtr");

    QApplication a(argc, argv);

    if(!QSystemTrayIcon::isSystemTrayAvailable()) {
        qCritical("No system tray available!\n");
        return 1;
    }

    Tray tray;

    QApplication::setQuitOnLastWindowClosed(false);
    
    return a.exec();
}
