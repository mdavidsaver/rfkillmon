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

#include <stdexcept>

#include <QCoreApplication>

Tray::Tray(QObject *parent) :
    QObject(parent)
  ,systray(this)
  ,evtsrc()
{
    QMenu *menu = new QMenu;

    menu->addAction("E&xit", this, SLOT(shutdown()));

    systray.setContextMenu(menu);
    showError("Starting up");

    bool C = true;

    C &= QObject::connect(&evtsrc, SIGNAL(newState(rfStatesPtr)),
                          this, SLOT(updateState(rfStatesPtr)));

    if(!C)
        throw std::logic_error("Some signals not connected in Tray!");

    systray.show();
    evtsrc.start();
}

void
Tray::shutdown()
{
    evtsrc.stop();
    qApp->quit();
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

void
Tray::updateState(rfStatesPtr states)
{
    qDebug("Update states");
    int ms = 0;
    QString tip("RFKill States\n");
    for(rfStates::const_iterator it = states->begin(), end = states->end();
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

void
Tray::showError(QString msg)
{
    systray.setIcon(QIcon(":/icon/error.svg"));
    systray.setToolTip(msg);
}
