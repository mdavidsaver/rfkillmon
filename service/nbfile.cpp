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
#include <sstream>

#include <error.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>

#include <QSocketNotifier>

#include "nbfile.h"

NBFile::NBFile(const char *s)
{
    fd = ::open(s, O_RDONLY|O_NONBLOCK);
    if(fd==-1) {
        std::ostringstream strm;
        strm<<"Failed to open "<<s<<": "<<strerror(errno);
        throw std::runtime_error(strm.str());
    }

    QSocketNotifier *notif=new QSocketNotifier(fd, QSocketNotifier::Read, this);
    connect(notif, SIGNAL(activated(int)), SIGNAL(readReady()));
}

NBFile::~NBFile()
{
    ::close(fd);
}

QByteArray NBFile::read(quint64 s)
{
    QByteArray ret(s, '\0');
    ssize_t n = ::read(fd, ret.data(), ret.size());
    if(n==0 || (n==-1 && (errno==EWOULDBLOCK || errno==EAGAIN))) {
        ret.resize(0);
        return ret;
    } else if(n==-1){
        std::ostringstream strm;
        strm<<"Failed to read: "<<strerror(errno);
        throw std::runtime_error(strm.str());
    }
    ret.resize(n);
    return ret;
}
