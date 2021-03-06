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
#ifndef NBFILE_H
#define NBFILE_H

#include <QObject>
#include <QByteArray>

//! non-blocking file
class NBFile : public QObject
{
    Q_OBJECT
    int fd;
public:
    NBFile(const char *s);
    virtual ~NBFile();

    int handle() const{return fd;}

    QByteArray read(quint64);

signals:
    void readReady();
};

#endif // NBFILE_H
