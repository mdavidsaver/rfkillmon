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

#include <QFile>
#include <QDebug>

#include "rfdevice.h"

static
QString
fetchName(quint32 idx)
{
    QString name(QString("/sys/class/rfkill/rfkill%1/name").arg(idx));

    QFile fp(name);

    QString ret;
    if(fp.open(QFile::ReadOnly)) {
        QByteArray arr(fp.readLine(100));
        if(!arr.isEmpty()) {
             ret = QString(arr).simplified();
        }
        fp.close();
    }

    if(ret.isNull())
        ret = QString("<device:%1>").arg(idx);

    return ret;
}


RFDevice::RFDevice(const QDBusConnection &c, Type t, quint32 id)
    :QObject()
    ,id(id)
    ,name(fetchName(id))
    ,path(QString("/devices/%1").arg(name))
    ,type(t)
    ,cur(Invalid)
    ,proxy(new Proxy(this))
    ,conn(c)
{
    if(!conn.registerObject(path.path(), this))
        qWarning()<<"Failed to register "<<path.path();
}

RFDevice::~RFDevice()
{
    conn.unregisterObject(path.path());
}

void
RFDevice::setState(State s)
{
    if(s==cur)
        return;
    bool wason = cur==On,
         nowon = s==On,
         changed = cur!=s;
    cur=s;
    if(changed)
        emit proxy->stateChanged(cur);
    if(wason ^ nowon)
        emit proxy->activeChanged(cur==On);
}

RFDevice::Proxy::Proxy(RFDevice *s)
    :QDBusAbstractAdaptor(s)
    ,self(s)
{}

RFDevice::Proxy::~Proxy() {}

QStringList RFDevice::Proxy::typeNames()
{
    QStringList ret;
    ret.reserve(2);
    ret.append("Wifi");
    ret.append("Bluetooth");
    return ret;
}

QStringList RFDevice::Proxy::stateNames()
{
    QStringList ret;
    ret.reserve(4);
    ret.append("Invalid");
    ret.append("On");
    ret.append("Soft");
    ret.append("Hard");
    return ret;
}
