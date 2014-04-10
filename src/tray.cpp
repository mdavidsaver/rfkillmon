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
#include "tray.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>

#include <stdexcept>

#include <linux/rfkill.h>

#include <QApplication>
#include <QSocketNotifier>
#include <QFile>
#include <QDebug>

#define DEVRFKILL "/dev/rfkill"

Tray::Tray(QObject *parent) :
    QObject(parent)
  ,systray(this)
  ,current()
{
    QMenu *menu = new QMenu;

    menu->addAction("E&xit", qApp, SLOT(quit()));

    systray.setContextMenu(menu);
    systray.setIcon(QIcon(":/icon/error.svg"));
    systray.setToolTip("Starting up");

    devfd = ::open(DEVRFKILL, O_RDONLY|O_NONBLOCK);
    if(devfd==-1) {
        qCritical("Failed to open " DEVRFKILL " : %s", strerror(errno));
        return;
    }
    qDebug("Open %p %d", this, devfd);

    int flags = fcntl(devfd, F_GETFL);
    if(!(flags&O_NONBLOCK)) {
        qWarning("Unable to set non-blocking for " DEVRFKILL);
    }

    QSocketNotifier *note = new QSocketNotifier(devfd, QSocketNotifier::Read, this);

    bool C = true;

    C &= QObject::connect(note, SIGNAL(activated(int)), this, SLOT(readEvents()));

    if(!C)
        throw std::logic_error("Some signals not connected in Tray!");

    systray.show();
}

Tray::~Tray()
{
    ::close(devfd);
}

static const char * const rfstates[3] = {
    "On", "Soft", "Hard"
};

static const char * const Iname[4] = {
    ":/icon/green.svg",
    ":/icon/yellow.svg",
    ":/icon/red.svg",
    ":/icon/error.svg",
};


namespace {
    QDebug& operator<<(QDebug& dbg, const rfkill_event& evt)
    {
        dbg << "IDX: " << evt.idx << "Type: " << evt.type << " Op:" << evt.op
            << "Soft: " << evt.soft << "Hard: " << evt.hard << "\n";
        return dbg;
    }
    union punEvent {
        rfkill_event evt;
        char bytes[sizeof(rfkill_event)];
    };
}

void
Tray::readEvents()
{
try {
    bool changed = false;

    qDebug("Events waiting...\n");

    while(true) {
        punEvent buf;

        qDebug("Have %p %d", this, devfd);
        int flags = fcntl(devfd, F_GETFL);
        if(!(flags&O_NONBLOCK)) {
            qWarning("Unable to set non-blocking for " DEVRFKILL);
        }

        ssize_t N = ::read(devfd, buf.bytes, sizeof(buf.bytes));

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
            rfStates::iterator it, end=current.end();

            for(it=current.begin(); it!=end; ++it) {
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
            rfStates::iterator it = current.find(buf.evt.idx);
            if(it==current.end()) {
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

                current[dev.idx] = dev;
            } else {
                if(buf.evt.op==RFKILL_OP_DEL) {
                    qDebug() << "Delete " << it.value().name;
                    changed = true;
                    // bye bye
                    names.remove(buf.evt.idx);
                    current.erase(it);
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

                current[dev.idx] = dev;
            }
        }
    }

    if(changed)
        updateState();
} catch(std::exception& e) {
    QString msg("Error: ");
    msg.append(e.what());
    systray.setIcon(QIcon(":/icon/error.svg"));
    systray.setToolTip(msg);
    qDebug("Error: %s", e.what());
}
}

void
Tray::updateState()
{
    qDebug("Update states");
    int ms = 0;
    QString tip("RFKill States\n");
    for(rfStates::const_iterator it = current.begin(), end = current.end();
        it!=end;
        ++it)
    {
        const char * sname = "Invalid";
        int sidx = int(it.value().state);
        if(sidx>=0 && sidx<3)
            sname = rfstates[sidx];
        QString msg("%1: %2\n");

        tip.append(msg.arg(it.value().name).arg(sname));
        ms = qMax(ms, sidx);
    }

    ms = qMin(ms, 3);

    systray.setIcon(QIcon(Iname[ms]));
    systray.setToolTip(tip);
}

QString
Tray::fetchName(quint32 idx)
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
