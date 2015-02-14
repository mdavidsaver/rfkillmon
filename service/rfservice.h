/* RF Kill monitor
 * Copyright 2015 Michael Davidsaver <mdavidsaver@gmail.com>
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
#ifndef RFSERVICE_H
#define RFSERVICE_H

#include <QList>
#include <QMap>
#include <QScopedPointer>
#include <QSharedPointer>
#include <QSocketNotifier>
#include <QTimer>
#include <QFile>

#include <QtDBus/QDBusConnection>
#include <QtDBus/QDBusAbstractAdaptor>
#include <QtDBus/QDBusObjectPath>

#include "nbfile.h"

class RFDevice;

class RFManager : public QObject
{
    Q_OBJECT
    class Proxy;
public:
    RFManager(const QDBusConnection&, QObject *par=0);
    virtual ~RFManager();

    typedef QSharedPointer<RFDevice> device_pointer;
    typedef QMap<quint32,device_pointer> device_map;
    device_map devices;

    QScopedPointer<Proxy> proxy;

    QTimer retry;
    QScopedPointer<NBFile> fd;

    QDBusConnection conn;
private:
    void onError();
private slots:
    void readReady();
    void retryNow();
};

class RFManager::Proxy : public QDBusAbstractAdaptor
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "foo.rfkill.service")
    friend class RFManager;
public:
    Proxy(RFManager*);
    virtual ~Proxy();
public slots:
    int version() const{return 1;}
    QList<QDBusObjectPath> adapters() const;

signals:
    void adaptersChanged();

private:
    RFManager *self;
};

#endif // RFSERVICE_H
