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

#include "rfeventsource.h"

class Tray : public QObject
{
    Q_OBJECT
public:
    explicit Tray(QObject *parent = 0);
    
public slots:
    void updateState(rfStatesPtr);
    void showError(QString);

private slots:
    void shutdown();
private:
    QSystemTrayIcon systray;

    rfEventSource evtsrc;
};

#endif // TRAY_H
