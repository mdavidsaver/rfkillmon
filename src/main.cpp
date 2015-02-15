#include <QApplication>
#include <QSystemTrayIcon>
#include <QtDBus/QDBusConnection>
#include <QDebug>

#include <rftray.h>

int main(int argc, char *argv[])
{
    Q_INIT_RESOURCE(icons);

    QApplication a(argc, argv);

    if(!QSystemTrayIcon::isSystemTrayAvailable()) {
        qCritical("No system tray available!\n");
        return 1;
    }

    RFTray tray(QDBusConnection::sessionBus());

    QApplication::setQuitOnLastWindowClosed(false);
    
    return a.exec();
}
