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
#ifndef RFDEVICE_H
#define RFDEVICE_H

#include <QStringList>
#include <QString>
#include <QScopedPointer>

#include <QtDBus/QDBusConnection>
#include <QtDBus/QDBusAbstractAdaptor>
#include <QtDBus/QDBusObjectPath>

class RFDevice : public QObject
{
    Q_OBJECT

    class Proxy;
public:
    enum State{Invalid=0,On,Soft,Hard};
    enum Type{Wifi=0, Blue};

    RFDevice(const QDBusConnection &, Type, quint32);
    virtual ~RFDevice();

    quint32 id;
    QString name;
    QDBusObjectPath path;
    Type type;
    State cur;

    void setState(State s);

    QScopedPointer<Proxy> proxy;
    QDBusConnection conn;
};

class RFDevice::Proxy : public QDBusAbstractAdaptor
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "foo.rfkill.device")
    Q_PROPERTY(QString name READ name)
    Q_PROPERTY(int type READ type)
    Q_PROPERTY(bool active READ active NOTIFY activeChanged)
    Q_PROPERTY(int state READ state NOTIFY stateChanged)

    friend class RFDevice;
public:
    Proxy(RFDevice *);
    ~Proxy();

    QString name() const{return self->name;}
    bool active() const{return self->cur==RFDevice::On;}
    int type() const{return self->type;}
    int state() const{return self->cur;}

public slots:
    static QStringList typeNames();
    static QStringList stateNames();

signals:
    void activeChanged(bool);
    void stateChanged(int);

private:
    RFDevice *self;
};

#endif // RFDEVICE_H
