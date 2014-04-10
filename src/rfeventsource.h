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
#ifndef RFEVENTSOURCE_H
#define RFEVENTSOURCE_H

#include <QObject>
#include <QThread>
#include <QFile>
#include <QSharedPointer>
#include <QMap>

struct rfState {
    quint32 idx;
    QString name;
    enum Type {
        Wifi=1, Blue=2
    } type;
    enum State {
        Open, Soft, Hard
    } state;
};

typedef QMap<quint32, rfState> rfStates;

typedef QSharedPointer<const rfStates> rfStatesPtr;

class rfEventSource : public QObject
{
    Q_OBJECT
public:
    explicit rfEventSource();
    virtual ~rfEventSource();

    void start();
    void stop();
signals:
    void newState(rfStatesPtr);
    void error(QString);

private slots:
    void onStart();
    void readEvents();

private:
    int devrfkill;
    QThread worker;

    QSharedPointer<rfStates> current;

    QMap<quint32, QString> names; // cache of interface names

    QString fetchName(quint32 idx);
};

#endif // RFEVENTSOURCE_H
