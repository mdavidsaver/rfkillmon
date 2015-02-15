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
#ifndef RFTRAY_H
#define RFTRAY_H

#include <QObject>
#include <QScopedPointer>

#include <QSystemTrayIcon>
#include <QMenu>
#include <QtDBus/QDBusConnection>
#include <QtDBus/QDBusAbstractInterface>
#include <QtDBus/QDBusObjectPath>

#include "serviceinterface.h"
#include "deviceinterface.h"

class RFTray : public QWidget
{
    Q_OBJECT
public:
    explicit RFTray(const QDBusConnection&, QWidget *parent = 0);
    virtual ~RFTray();

    QSystemTrayIcon systray;
    QDBusConnection conn;
    QTimer retry;

    ::foo::rfkill::service *service;
    QScopedPointer<foo::rfkill::device> device;
    QString deviceName;

    void onError();

private slots:
    void adaptersChanged();
    void activeChanged(bool);
    void setAdapter(QString);
};

#endif // RFTRAY_H
