/***************************************************************************
 *   Copyright (C) 2016-2026 by Ilya Kotov                                 *
 *   forkotov02@ya.ru                                                      *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/

#include <QRegularExpression>
#include <qmmp/qmmp.h>
#include "archiveinputdevice.h"

using namespace Qt::Literals::StringLiterals;

ArchiveInputDevice::ArchiveInputDevice(const QString &url, QObject *parent)  : QIODevice(parent)
{
    QString filePath = url.section(QLatin1Char('#'), -1);
    QString archivePath = url;
    archivePath.remove(QRegularExpression(u"^.+://"_s));
    archivePath.remove(QRegularExpression(u"#.+$"_s));

    m_archive = archive_read_new();
    archive_read_support_filter_all(m_archive);
    archive_read_support_format_all(m_archive);

    int r = archive_read_open_filename(m_archive, archivePath.toLocal8Bit().constData(), 10240);
    if (r != ARCHIVE_OK)
    {
        qCWarning(plugin, "unable to open file '%s', libarchive error: %s",
                 qPrintable(archivePath), archive_error_string(m_archive));
        return;
    }

    while (archive_read_next_header(m_archive, &m_entry) == ARCHIVE_OK)
    {
        QString pathName = QString::fromLocal8Bit(archive_entry_pathname(m_entry));
        if(!pathName.startsWith(QLatin1Char('/')))
            pathName.prepend(QLatin1Char('/'));

        if(archive_entry_filetype(m_entry) == AE_IFREG && filePath == pathName)
        {
            open(QIODevice::ReadOnly);
            m_buffer.open(QBuffer::ReadWrite);
            break;
        }
        archive_read_data_skip(m_archive);
    }
}

ArchiveInputDevice::ArchiveInputDevice(archive *a, archive_entry *e, QObject *parent) : QIODevice(parent)
{
    m_archive = a;
    m_entry = e;
    open(QIODevice::ReadOnly);
    m_buffer.open(QBuffer::ReadWrite);
    m_close_libarchive = false;
}

ArchiveInputDevice::~ArchiveInputDevice()
{
    if(m_close_libarchive && m_archive)
    {
        archive_read_close(m_archive);
        archive_read_free(m_archive);
        m_archive = nullptr;
    }
}

bool ArchiveInputDevice::seek(qint64 pos)
{
    if(!isOpen())
        return false;

    QIODevice::seek(pos);

    if(pos > m_buffer.size())
    {
        char tmp[1024];
        qint64 delta = pos - m_buffer.size();
        while (delta > 0)
        {
            qint64 r = qMin(qint64(sizeof(tmp)), delta);
            r = archive_read_data(m_archive, tmp, r);
            if(r > 0)
            {
                m_buffer.buffer().append(tmp, r);
                delta -= r;
                continue;
            }

            if(r < 0)
            {
                qCWarning(plugin, "seeking failed; libarchive error: %s", archive_error_string(m_archive));
                setErrorString(QString::fromLocal8Bit(archive_error_string(m_archive)));
                close();
            }
            return false;
        }
    }

    return m_buffer.seek(pos);
}

qint64 ArchiveInputDevice::size() const
{
    return archive_entry_size(m_entry);
}

qint64 ArchiveInputDevice::readData(char *data, qint64 maxSize)
{
    if(!isOpen())
        return -1;

    if(m_buffer.pos() + maxSize > m_buffer.size())
    {
        qint64 l = m_buffer.pos() + maxSize - m_buffer.size();
        char *tmp = new char[l];
        int r = archive_read_data(m_archive, tmp, l);
        if(r > 0)
            m_buffer.buffer().append(tmp, r);
        else if(r < 0)
        {
            qCWarning(plugin, "reading failed; libarchive error: %s", archive_error_string(m_archive));
            setErrorString(QString::fromLocal8Bit(archive_error_string(m_archive)));
            delete [] tmp;
            return -1;
        }
        delete [] tmp;
    }
    return m_buffer.read(data, maxSize);
}

qint64 ArchiveInputDevice::writeData(const char *, qint64)
{
    return -1;
}
