/***************************************************************************
 *   Copyright(C) 2025 by Ilya Kotov                                       *
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

#include <QSettings>
#include <QByteArray>
#include <QBuffer>
#include <taglib/tag.h>
#include <taglib/fileref.h>
#include <taglib/id3v1tag.h>
#include <taglib/id3v2tag.h>
#include <taglib/apetag.h>
#include <taglib/tfile.h>
#include <taglib/mpegfile.h>
#include <taglib/mpegheader.h>
#include <taglib/mpegproperties.h>
#include <taglib/textidentificationframe.h>
#include <taglib/attachedpictureframe.h>
#include <taglib/id3v2framefactory.h>
#include <qmmp/qmmptextcodec.h>
#include "sndfilemetadatamodel.h"

SndFileMetaDataModel::SndFileMetaDataModel(const QString &path, bool readOnly) :
    MetaDataModel(readOnly, MetaDataModel::IsCoverEditable)
{
    m_stream = new TagLib::FileStream(QStringToFileName(path), readOnly);

    if(TagLib::RIFF::WAV::File::isSupported(m_stream))
    {
        m_wavFile = new TagLib::RIFF::WAV::File(m_stream);
        m_tags << new SndFileTagModel(m_wavFile);
    }
    else if(TagLib::RIFF::AIFF::File::isSupported(m_stream))
    {
        m_aiffFile = new TagLib::RIFF::AIFF::File(m_stream);
        m_tags << new SndFileTagModel(m_aiffFile);
    }
}

SndFileMetaDataModel::~SndFileMetaDataModel()
{
    while(!m_tags.isEmpty())
        delete m_tags.takeFirst();
    delete m_wavFile;
    delete m_aiffFile;
    delete m_stream;
}

QList<TagModel* > SndFileMetaDataModel::tags() const
{
    return m_tags;
}

QImage SndFileMetaDataModel::cover() const
{
    TagLib::ID3v2::Tag *tag = nullptr;

    if(m_wavFile)
        tag = m_wavFile->ID3v2Tag();
    else if(m_aiffFile)
        tag = m_aiffFile->tag();

    if(!tag)
        return QImage();

    TagLib::ID3v2::FrameList frames = tag->frameListMap()["APIC"];
    if(frames.isEmpty())
        return QImage();

    for(TagLib::ID3v2::FrameList::Iterator it = frames.begin(); it != frames.end(); ++it)
    {
        TagLib::ID3v2::AttachedPictureFrame *frame = dynamic_cast<TagLib::ID3v2::AttachedPictureFrame *>(*it);
        if(frame && frame->type() == TagLib::ID3v2::AttachedPictureFrame::FrontCover)
        {
            QImage cover;
            cover.loadFromData((const uchar *)frame->picture().data(),
                                     frame->picture().size());
            return cover;
        }
    }
    //fallback image
    for(TagLib::ID3v2::FrameList::Iterator it = frames.begin(); it != frames.end(); ++it)
    {
        TagLib::ID3v2::AttachedPictureFrame *frame = dynamic_cast<TagLib::ID3v2::AttachedPictureFrame *>(*it);
        if(frame)
        {
            QImage cover;
            cover.loadFromData((const uchar *)frame->picture().data(),
                                     frame->picture().size());
            return cover;
        }
    }
    return QImage();
}

void SndFileMetaDataModel::setCover(const QImage &img)
{
    TagLib::ID3v2::Tag *tag = nullptr;

    if(m_wavFile)
        tag = m_wavFile->ID3v2Tag();
    else if(m_aiffFile)
        tag = m_aiffFile->tag();

    if(!tag)
        return;

    tag->removeFrames("APIC");
    TagLib::ID3v2::AttachedPictureFrame *frame = new TagLib::ID3v2::AttachedPictureFrame;
    frame->setType(TagLib::ID3v2::AttachedPictureFrame::FrontCover);
    QByteArray data;
    QBuffer buffer(&data);
    buffer.open(QIODevice::WriteOnly);
    img.save(&buffer, "JPEG");
    frame->setMimeType("image/jpeg");
    frame->setPicture(TagLib::ByteVector(data.constData(), data.size()));
    tag->addFrame(frame);

    if(m_wavFile)
        m_wavFile->save(TagLib::RIFF::WAV::File::ID3v2, TagLib::File::StripNone, TagLib::ID3v2::v4);
    else if(m_aiffFile)
        m_aiffFile->save(TagLib::ID3v2::v4);
}

void SndFileMetaDataModel::removeCover()
{
    if(m_wavFile && m_wavFile->ID3v2Tag())
    {
        m_wavFile->ID3v2Tag()->removeFrames("APIC");
        m_wavFile->save(TagLib::RIFF::WAV::File::ID3v2, TagLib::File::StripNone, TagLib::ID3v2::v4);
    }
    else if(m_aiffFile)
    {
         m_aiffFile->tag()->removeFrames("APIC");
         m_aiffFile->save(TagLib::ID3v2::v4);
    }
}

QString SndFileMetaDataModel::lyrics() const
{
    for(const TagModel *tag : std::as_const(m_tags))
    {
        const SndFileTagModel *mpegTag = static_cast<const SndFileTagModel *>(tag);
        QString lyrics = mpegTag->lyrics();
        if(!lyrics.isEmpty())
            return lyrics;
    }

    return QString();
}

SndFileTagModel::SndFileTagModel(TagLib::RIFF::WAV::File *file) : TagModel(TagModel::CreateRemove), m_wavFile(file)
{
    m_tag = m_wavFile->ID3v2Tag();
}

SndFileTagModel::SndFileTagModel(TagLib::RIFF::AIFF::File *file) : TagModel(), m_aiffFile(file)
{
    m_tag = m_aiffFile->tag();
}

QString SndFileTagModel::name() const
{
    return u"ID3v2"_s;
}

QString SndFileTagModel::value(Qmmp::MetaData key) const
{
    if(!m_tag)
        return QString();

    switch(key)
    {
    case Qmmp::TITLE:
        return TStringToQString(m_tag->title());
    case Qmmp::ARTIST:
        return TStringToQString(m_tag->artist());
    case Qmmp::ALBUMARTIST:
        if(!m_tag->frameListMap()["TPE2"].isEmpty())
        {
            return TStringToQString(m_tag->frameListMap()["TPE2"].front()->toString());
        }
        return QString();
    case Qmmp::ALBUM:
        return TStringToQString(m_tag->album());
    case Qmmp::COMMENT:
        return TStringToQString(m_tag->comment());
    case Qmmp::GENRE:
        return TStringToQString(m_tag->genre());
    case Qmmp::COMPOSER:
        if(!m_tag->frameListMap()["TCOM"].isEmpty())
        {
            return TStringToQString(m_tag->frameListMap()["TCOM"].front()->toString());
        }
        return QString();
    case Qmmp::YEAR:
        return QString::number(m_tag->year());
    case Qmmp::TRACK:
        return QString::number(m_tag->track());
    case  Qmmp::DISCNUMBER:
        if(!m_tag->frameListMap()["TPOS"].isEmpty())
        {
            return TStringToQString(m_tag->frameListMap()["TPOS"].front()->toString());
        }
        return QString();

    default:
        ;
    }
    return QString();
}

void SndFileTagModel::setValue(Qmmp::MetaData key, const QString &value)
{
    if(!m_tag)
        return;

    TagLib::String str = QStringToTString(value);

    switch(key)
    {
    case Qmmp::TITLE:
        m_tag->setTitle(str);
        return;
    case Qmmp::ARTIST:
        m_tag->setArtist(str);
        return;
    case Qmmp::ALBUM:
        m_tag->setAlbum(str);
        return;
    case Qmmp::COMMENT:
        m_tag->setComment(str);
        return;
    case Qmmp::GENRE:
        m_tag->setGenre(str);
        return;
    case Qmmp::YEAR:
        m_tag->setYear(value.toInt());
        return;
    case Qmmp::TRACK:
        m_tag->setTrack(value.toInt());
    default:
        ;
    }

    //save additional tags
    TagLib::ByteVector id3v2_key;
    if(key == Qmmp::ALBUMARTIST)
        id3v2_key = "TPE2"; //album artist
    else if(key == Qmmp::COMPOSER)
        id3v2_key = "TCOM"; //composer
    else if(key == Qmmp::DISCNUMBER)
        id3v2_key = "TPOS";  //disc number

    if(!id3v2_key.isEmpty())
    {
        if(value.isEmpty())
            m_tag->removeFrames(id3v2_key);
        else if(!m_tag->frameListMap()[id3v2_key].isEmpty())
            m_tag->frameListMap()[id3v2_key].front()->setText(str);
        else
        {
            TagLib::ID3v2::TextIdentificationFrame *frame;
            frame = new TagLib::ID3v2::TextIdentificationFrame(id3v2_key, TagLib::String::UTF8);
            frame->setText(str);
            m_tag->addFrame(frame);
        }
        return;
    }
}

bool SndFileTagModel::exists() const
{
    return (m_tag != nullptr);
}

void SndFileTagModel::create()
{
    if(m_tag)
        return;

    if(m_wavFile)
        m_tag = m_wavFile->ID3v2Tag();
    else if(m_aiffFile)
        m_tag = m_aiffFile->tag();
}

void SndFileTagModel::remove()
{
    m_tag = nullptr;
}

void SndFileTagModel::save()
{
    if(m_tag)
    {
        if(m_wavFile)
            m_wavFile->save(TagLib::RIFF::WAV::File::ID3v2, TagLib::File::StripNone, TagLib::ID3v2::v4);
        else if(m_aiffFile)
            m_aiffFile->save(TagLib::ID3v2::v4);
    }
    else
    {
        if(m_wavFile)
            m_wavFile->strip(TagLib::RIFF::WAV::File::ID3v2);
    }
}

QString SndFileTagModel::lyrics() const
{
    if(m_tag)
    {
        const TagLib::ID3v2::FrameListMap &map = m_tag->frameListMap();

        if(!map["USLT"].isEmpty())
            return TStringToQString(map["USLT"].front()->toString());
    }

    return QString();
}

