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

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>

#include "rfeventsource.h"

#include <stdexcept>

#include <linux/rfkill.h>

#include <QSocketNotifier>
#include <QDebug>

#define DEVRFKILL "/dev/rfkill"

namespace {
QDebug& operator<<(QDebug& dbg, const rfkill_event& evt)
{
    dbg << "IDX: " << evt.idx << "Type: " << evt.type << " Op:" << evt.op
        << "Soft: " << evt.soft << "Hard: " << evt.hard << "\n";
    return dbg;
}
}

rfEventSource::rfEventSource() :
    QObject()
  ,devrfkill(-1)
  ,worker()
  ,current(new rfStates)
  ,names()
{
    this->moveToThread(&worker);

    bool C = true;

    C &= QObject::connect(&worker, SIGNAL(started()), this, SLOT(onStart()));

    if(!C)
        throw std::logic_error("Some signals not connected in rfEventSource!");
}

rfEventSource::~rfEventSource()
{
    if(worker.isRunning())
        stop();
}

void
rfEventSource::start()
{
    qDebug("Worker starting...\n");
    worker.start();
}

void
rfEventSource::stop()
{
    qDebug("Worker stopping...\n");
    worker.quit();
    worker.wait();
    ::close(devrfkill);
    qDebug("Worker done.\n");
}

void
rfEventSource::onStart()
{
    int fd = ::open(DEVRFKILL, O_RDONLY|O_NONBLOCK);
    if(fd==-1) {
        QString msg("Failed to open " DEVRFKILL " : %1");
        emit error(msg.arg(strerror(errno)));
        return;
    }

    int flags = fcntl(fd, F_GETFL);
    if(!(flags&O_NONBLOCK)) {
        qWarning("Unable to set non-blocking for " DEVRFKILL);
    }

    QSocketNotifier *note = new QSocketNotifier(fd, QSocketNotifier::Read, this);

    bool C = true;

    C &= QObject::connect(note, SIGNAL(activated(int)), this, SLOT(readEvents()));

    if(!C)
        throw std::logic_error("Some signals not connected in rfEventSource 2!");

    devrfkill = fd;
}

namespace {
    union punEvent {
        rfkill_event evt;
        char bytes[sizeof(rfkill_event)];
    };
}

void
rfEventSource::readEvents()
{
try{
    bool changed = false;

    qDebug("Events waiting...\n");

    while(true) {
        punEvent buf;

        ssize_t N = ::read(devrfkill, buf.bytes, sizeof(buf.bytes));

        if(Q_UNLIKELY(N<0 || quint64(N) < sizeof(buf.bytes))) {
            if(N<0) {
                if(errno==EWOULDBLOCK || errno==EAGAIN)
                    break;
                qCritical() << "Error reading from " DEVRFKILL " : "
                            << strerror(errno) << "\n";
                return;
            } else if(N==0) {
                qCritical("Encountered end of " DEVRFKILL "?\n");
                return;
            } else {
                qCritical("Truncated event from " DEVRFKILL "?\n");
                return;
            }
        }

        qDebug() << "RF Event\n" << buf.evt;

        if(buf.evt.op==RFKILL_OP_CHANGE_ALL) {
            qDebug("Change all");
            changed = true;
            // special global event...
            rfStates::iterator it, end=current->end();

            for(it=current->begin(); it!=end; ++it) {
                if(buf.evt.hard)
                    it.value().state = rfState::Hard;
                else if(buf.evt.soft)
                    it.value().state = rfState::Soft;
                else
                    it.value().state = rfState::Open;
            }

        } else {
            // single device event

            // lookup previous state (if known)
            rfState dev;
            rfStates::iterator it = current->find(buf.evt.idx);
            if(it==current->end()) {
                // We don't know about this device yet
                if(buf.evt.op==RFKILL_OP_DEL) {
                    // so we discovered a new device as it is disappearing...
                    continue; // ignore this event

                } else if(buf.evt.op!=RFKILL_OP_ADD) {
                    qCritical() << "New device w/o add " << buf.evt.idx << "\n";
                }
                dev.idx = buf.evt.idx;
                dev.name = fetchName(dev.idx);
                dev.type = rfState::Type(buf.evt.type);
                dev.state = rfState::Hard;

                (*current)[dev.idx] = dev;
            } else {
                if(buf.evt.op==RFKILL_OP_DEL) {
                    qDebug() << "Delete " << it.value().name;
                    changed = true;
                    // bye bye
                    names.remove(buf.evt.idx);
                    current->erase(it);
                    continue;
                }
                dev = *it;
            }

            if(buf.evt.op==RFKILL_OP_ADD || buf.evt.op==RFKILL_OP_CHANGE) {
                qDebug() << "Add/Update " << dev.name;
                if(dev.type != buf.evt.type)
                    qWarning() << "Device type changed " << dev.name << " "
                               << dev.type << " to " << buf.evt.type;

                changed = true;
                if(buf.evt.hard)
                    dev.state = rfState::Hard;
                else if(buf.evt.soft)
                    dev.state = rfState::Soft;
                else
                    dev.state = rfState::Open;

                (*current)[dev.idx] = dev;
            }
        }
    }

    if(!changed)
        return;

    QSharedPointer<rfStates> copy(new rfStates(*current.data()));

    emit newState(copy);
} catch(std::exception& e) {
    QString msg("Error: ");
    msg.append(e.what());
    emit error(msg);
    qDebug("Worker error: %s", e.what());
}
}

QString
rfEventSource::fetchName(quint32 idx)
{
    QMap<quint32, QString>::iterator it = names.find(idx);
    if(it!=names.end())
        return it.value();

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
        ret = QString("<%1>").arg(idx);

    names[idx] = ret;
    return ret;
}
