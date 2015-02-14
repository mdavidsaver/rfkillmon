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

#include <vector>
#include <algorithm>
#include <stdexcept>

#include <linux/rfkill.h>

#include <QDebug>

#include "rfservice.h"
#include "rfdevice.h"

#define DEVRFKILL "/dev/rfkill"

RFManager::RFManager(const QDBusConnection &c, QObject *par)
    :QObject(par)
    ,proxy(new Proxy(this))
    ,conn(c)
{
    connect(&retry, SIGNAL(timeout()), SLOT(retryNow()));

    retry.setSingleShot(true);
    retry.start(1000);

    if(!conn.registerObject("/service", this))
        throw std::runtime_error("Failed to register main DBus object");
}

RFManager::~RFManager()
{
    conn.unregisterObject("/service");
}

namespace {
bool operator==(const RFDevice&d, rfkill_type t)
{
    if(t==RFKILL_TYPE_ALL)
        return true;
    switch(d.type) {
    case RFDevice::Wifi: return t==RFKILL_TYPE_WLAN;
    case RFDevice::Blue: return t==RFKILL_TYPE_BLUETOOTH;
    }
    return false;
}

bool operator!=(const RFDevice&d, rfkill_type t)
{ return !(d==t); }
}

QDebug& operator<<(QDebug& dbg, const rfkill_event& evt)
{
    dbg << "IDX: " << evt.idx << "Type: " << evt.type << " Op:" << evt.op
        << "Soft: " << evt.soft << "Hard: " << evt.hard << "\n";
    return dbg;
}

static
void setDevState(RFDevice& dev, const rfkill_event& evt)
{
    if(evt.hard)
        dev.setState(RFDevice::Hard);
    else if(evt.soft)
        dev.setState(RFDevice::Soft);
    else
        dev.setState(RFDevice::On);
}

union punEvent {
    rfkill_event evt;
    char bytes[sizeof(rfkill_event)];
};

static
void processEvent(RFManager& self, const rfkill_event& evt, bool& addrem)
{
    qDebug()<<"Event "<<evt;

    switch(rfkill_operation(evt.op)) {
    case RFKILL_OP_CHANGE_ALL:{
        // change all devices of the given type
        foreach (const RFManager::device_pointer& dev, self.devices) {
            if(*dev!=rfkill_type(evt.type))
                continue;

            setDevState(*dev, evt);
        }
    }return;

    case RFKILL_OP_ADD:{
        // add a new device
        RFDevice::Type dt;
        switch(rfkill_type(evt.type)) {
        case RFKILL_TYPE_WLAN: dt=RFDevice::Wifi; break;
        case RFKILL_TYPE_BLUETOOTH: dt=RFDevice::Blue; break;
        default:
            qWarning()<<"Asked to add device of unsupported type "<<evt.idx;
            return; // ignore device type
        }

        RFManager::device_pointer ptr(new RFDevice(self.conn, dt, evt.idx));
        setDevState(*ptr, evt);
        self.devices.insert(evt.idx, ptr);
        addrem = true;

    }return;

    case RFKILL_OP_DEL:{
        RFManager::device_map::iterator it=self.devices.find(evt.idx);
        if(it==self.devices.end()) {
            qWarning()<<"Asked to remove unknown device of type "<<evt.idx;
        } else {
            self.devices.erase(it);
            addrem = true;
        }
    }return;

    case RFKILL_OP_CHANGE:{
        RFManager::device_map::iterator it=self.devices.find(evt.idx);
        if(it==self.devices.end()) {
            qWarning()<<"Asked to change unknown device "<<evt.idx;
        } else {
            setDevState(**it, evt);
        }
    }return;

    // default: omitted to trigger compiler warning if rkill_operation enum is extended
    }

    qWarning()<<"Unknown operator "<<evt.op;
}

void RFManager::readReady()
{
    qDebug()<<"Readable ";
    QByteArray buf(fd->read(10*sizeof(rfkill_event)));
    qDebug()<<"Read "<<buf.size();
    if(buf.size()%sizeof(rfkill_event)!=0) {
        qWarning("Read returned partial event?");
        onError();
        return;
    }
    QByteArray::const_iterator it=buf.begin(), end=buf.end();

    bool addrem = false;

    for(;it!=end;it+=sizeof(rfkill_event))
    {
        Q_ASSERT(it<end);
        punEvent evtbuf;
        std::copy(it, it+sizeof(rfkill_event), evtbuf.bytes);
        try{
            processEvent(*this, evtbuf.evt, addrem);
        }catch(std::exception& e){
            qWarning("Exception processing event: %s", e.what());
        }
    }

    if(addrem)
        emit proxy->adaptersChanged();
}

void RFManager::retryNow()
{
    qDebug("Opening now");
try{
    QScopedPointer<NBFile> file(new NBFile(DEVRFKILL));

    connect(file.data(), SIGNAL(readReady()), SLOT(readReady()));

    fd.swap(file);
    qDebug("Open");
}catch(std::exception& e){
    qDebug("Exception during retry: %s", e.what());
    onError();
}
}

void RFManager::onError()
{
    fd.reset();
    retry.start(60000);
}

RFManager::Proxy::Proxy(RFManager *s)
    :QDBusAbstractAdaptor(s)
    ,self(s)
{}

RFManager::Proxy::~Proxy() {}

QList<QDBusObjectPath>
RFManager::Proxy::adapters() const
{
    QList<QDBusObjectPath> ret;
    foreach (const RFManager::device_pointer& dev, self->devices) {
        ret.append(dev->path);
    }
    return ret;
}
