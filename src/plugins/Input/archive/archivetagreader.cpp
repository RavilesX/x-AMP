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

#include <taglib/tiostream.h>
#include "archivetagreader.h"

class IODeviceStream : public TagLib::IOStream
{
public:
    IODeviceStream(QIODevice *input, const QString &url) : m_input(input),
#ifdef Q_OS_WIN
          m_fileName(url.section(QLatin1Char('/'), -1))
#else
          m_fileName(url.section(QLatin1Char('/'), -1).toLocal8Bit())
#endif
    {}

    virtual ~IODeviceStream() {}

    virtual TagLib::FileName name() const override
    {
#ifdef Q_OS_WIN
        return QStringToFileName(m_fileName);
#else
        return m_fileName.constData();
#endif
    }
#if TAGLIB_MAJOR_VERSION >= 2
    virtual TagLib::ByteVector readBlock(size_t length) override
#else
    virtual TagLib::ByteVector readBlock(unsigned long length) override
#endif
    {
        TagLib::ByteVector data(length, 0);
        qint64 l = m_input->read(data.data(), length);
        if(l < 0)
        {
            m_input->close();
            return TagLib::ByteVector();
        }
        data.resize(l);
        return data;
    }
    virtual void writeBlock(const TagLib::ByteVector &) override
    {}
#if TAGLIB_MAJOR_VERSION >= 2
    virtual void insert(const TagLib::ByteVector &, TagLib::offset_t, size_t) override
    {}
    virtual void removeBlock(TagLib::offset_t, size_t) override
    {}
#else
    virtual void insert(const TagLib::ByteVector &, unsigned long, unsigned long) override
    {}
    virtual void removeBlock(unsigned long, unsigned long) override
    {}
#endif
    virtual bool readOnly() const override
    {
        return true;
    }
    virtual bool isOpen() const override
    {
        return m_input->isOpen();
    }
#if TAGLIB_MAJOR_VERSION >= 2
    virtual void seek(TagLib::offset_t offset, Position p = Beginning) override
#else
    virtual void seek(long offset, Position p = Beginning) override
#endif
    {
        switch (p)
        {
        case Beginning:
            m_input->seek(offset);
            break;
        case Current:
            m_input->seek(m_input->pos() + offset);
            break;
        case End:
            m_input->seek(m_input->size() + offset);
            break;
        }
    }
    virtual void clear() override
    {
        m_input->seek(0);
        TagLib::IOStream::clear();
    }
#if TAGLIB_MAJOR_VERSION >= 2
    virtual TagLib::offset_t tell() const override
#else
    virtual long tell() const override
#endif
    {
        return m_input->pos();
    }
#if TAGLIB_MAJOR_VERSION >= 2
    virtual TagLib::offset_t length() override
#else
    virtual long length() override
#endif
    {
        return m_input->size();
    }
#if TAGLIB_MAJOR_VERSION >= 2
    virtual void truncate(TagLib::offset_t) override
#else
    virtual void truncate(long) override
#endif
    {}

private:
    QIODevice *m_input;
#ifdef Q_OS_WIN
    QString m_fileName;
#else
    QByteArray m_fileName;
#endif
};

ArchiveTagReader::ArchiveTagReader(QIODevice *input, const QString &url)
{
    m_stream = new IODeviceStream(input, url);
    m_file = new TagLib::FileRef(m_stream);
}

ArchiveTagReader::~ArchiveTagReader()
{
    delete m_file;
    delete m_stream;
}

QMap<Qmmp::MetaData, QString> ArchiveTagReader::metaData() const
{
    QMap<Qmmp::MetaData, QString> m;

    TagLib::Tag *tag = m_file->tag();
    if(tag)
    {
        m[Qmmp::ALBUM] = TStringToQString(tag->album()).trimmed();
        m[Qmmp::ARTIST] = TStringToQString(tag->artist()).trimmed();
        m[Qmmp::COMMENT] = TStringToQString(tag->comment()).trimmed();
        m[Qmmp::GENRE] = TStringToQString(tag->genre()).trimmed();
        m[Qmmp::TITLE] = TStringToQString(tag->title()).trimmed();
        m[Qmmp::YEAR] = QString::number(tag->year());
        m[Qmmp::TRACK] = QString::number(tag->track());
    }
    return m;
}

TagLib::AudioProperties *ArchiveTagReader::audioProperties() const
{
    return m_file->audioProperties();
}
