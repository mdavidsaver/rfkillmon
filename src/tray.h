/* RF Kill monitor
 * Copyright 2014 Michael Davidsaver <mdavidsaver@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef TRAY_H
#define TRAY_H

#include <QObject>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QtDBus/QDBusAbstractAdaptor>
#include <QtDBus/QDBusObjectPath>

class Tray;

struct rfState {
    quint32 idx;
    QString name;
    enum Type {
        Wifi=1, Blue=2
    } type;
    enum State {
        Open, Soft, Hard
    } state;
};

class RFAdapters : public QDBusAbstractAdaptor
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "foo.rfkill")
//    Q_CLASSINFO("D-Bus Introspection", ""
//    "  <interface name=\"foo.rfkill\">\n"
//    "    <method name=\"adapters\">\n"
//    "      <arg direction=\"out\" type=\"ao\" name=\"names\"/>\n"
//    "    </method>\n"
//    "    <signal name=\"adaptersChanged\"/>\n"
//    "  </interface>\n"
//            "")
public:
    RFAdapters(Tray*);
    virtual ~RFAdapters();

public slots:
    QList<QDBusObjectPath> adapters();

signals:
    void adaptersChanged();

private:
    Tray *self;
};

typedef QMap<quint32, rfState> rfStates;

class Tray : public QObject
{
    Q_OBJECT
public:
    explicit Tray(QObject *parent = 0);
    virtual ~Tray();

public slots:
    void readEvents();

public:
    QSystemTrayIcon systray;

    int devfd;

    rfStates current;

    void updateState();

    QMap<quint32, QString> names; // cache of interface names

    QString fetchName(quint32 idx);

    RFAdapters adaptersProxy;
};

#endif // TRAY_H
