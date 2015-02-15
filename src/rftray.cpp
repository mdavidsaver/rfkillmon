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

#include <stdexcept>

#include <QDebug>
#include <QSignalMapper>
#include <QSettings>
#include <QApplication>

#include "rftray.h"

namespace {
template<typename T>
T syncFetch(QDBusPendingReply<T> R)
{
    R.waitForFinished();
    if(R.isError()) {
        qDebug()<<"DBus error: "<<R.error();
        throw std::runtime_error("DBus error");
    }
    return R.value();
}
}

RFTray::RFTray(const QDBusConnection& c,QWidget *parent)
    :QWidget(parent)
    ,conn(c)
    ,device(NULL)
{
    QSettings settings("rfkilltray", "gui");
    deviceName = settings.value("interface").toString();

    retry.setSingleShot(true);

    connect(&retry, SIGNAL(timeout()), SLOT(adaptersChanged()));

    service = new FooRfkillServiceInterface("foo.rfkill",
                                            "/service",
                                            conn, this);
    service->setTimeout(5000);

    connect(service, SIGNAL(adaptersChanged()), SLOT(adaptersChanged()));

    systray.setContextMenu(new QMenu(this));
    systray.setIcon(QIcon(":/icon/error.svg"));
    systray.setToolTip("Starting up");

    systray.show();

    retry.start(1000);
}

RFTray::~RFTray()
{
    delete service;
}

void RFTray::adaptersChanged()
{
try{
    QString wantname(deviceName);
    qDebug()<<"Want device "<<wantname;

    int ver(syncFetch(service->version()));
    qDebug()<<"Remote is version "<<ver;

    QList<QDBusObjectPath> paths(syncFetch(service->adapters()));
    QScopedPointer<foo::rfkill::device> take;
    QStringList names;

    foreach(const QDBusObjectPath& path, paths)
    {
        QScopedPointer<foo::rfkill::device> dev;
        dev.reset(new foo::rfkill::device("foo.rfkill", path.path(),
                                          conn, this));

        QString name(dev->name());
        qDebug()<<"Consider "<<name;
        names.append(name);

        if(wantname.isEmpty() || name==wantname)
        {
            take.swap(dev);
            wantname.swap(name);
        }
    }

    // build menu
    QMenu *ctxt = new QMenu(this);
    ctxt->addAction("Adapters");
    ctxt->addSeparator();

    QSignalMapper *mapper = new QSignalMapper(ctxt);
    connect(mapper, SIGNAL(mapped(QString)), SLOT(setAdapter(QString)));

    foreach(const QString& name, names)
    {
        QAction *act = ctxt->addAction(name, mapper, SLOT(map()));
        act->setCheckable(true);
        act->setChecked(name==wantname);
        mapper->setMapping(act, name);
    }

    ctxt->addSeparator();
    ctxt->addAction("E&xit", QApplication::instance(), SLOT(quit()));

    systray.contextMenu()->deleteLater();
    systray.setContextMenu(ctxt);

    if(!take.isNull())
    {
        device.swap(take);
        deviceName.swap(wantname);

        connect(device.data(), SIGNAL(activeChanged(bool)), SLOT(activeChanged(bool)));
        activeChanged(device->active());
    }
}catch(std::exception& e){
    qWarning("Exception while fetching adapter list: %s", e.what());
    onError();
}
}

void RFTray::onError()
{
    retry.start(60000);

    QMenu *ctxt = new QMenu(this);
    ctxt->addAction("E&xit", QApplication::instance(), SLOT(quit()));

    systray.contextMenu()->deleteLater();
    systray.setContextMenu(ctxt);

    systray.setIcon(QIcon(":/icon/error.svg"));
    systray.setToolTip("Error");

    qDebug()<<"Will retry";
}

void RFTray::setAdapter(QString name)
{
    QSettings settings("rfkilltray", "gui");
    deviceName = name;
    settings.setValue("interface", deviceName);
    retry.stop();
    adaptersChanged();
}

void RFTray::activeChanged(bool b)
{
    if(b) {
        systray.setIcon(QIcon(":/icon/green.svg"));
        systray.setToolTip(QString("%1 is active").arg(deviceName));
    } else {
        systray.setIcon(QIcon(":/icon/red.svg"));
        systray.setToolTip(QString("%1 is blocked").arg(deviceName));
    }
}
