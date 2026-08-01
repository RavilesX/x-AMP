/***************************************************************************
 *   Copyright(C) 2009-2026 by Ilya Kotov                                 *
 *   forkotov02@ya.ru                                                      *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *  (at your option) any later version.                                   *
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
#include <taglib/unsynchronizedlyricsframe.h>
#include <qmmp/qmmptextcodec.h>
#include "tagextractor.h"
#include "mpegmetadatamodel.h"

MPEGMetaDataModel::MPEGMetaDataModel(bool using_rusxmms, const QString &path, bool readOnly) :
    MetaDataModel(readOnly, MetaDataModel::IsCoverEditable | MetaDataModel::IsLyricsEditable)
{
    m_stream = new TagLib::FileStream(QStringToFileName(path), readOnly);
#if TAGLIB_MAJOR_VERSION >= 2
    m_file = new TagLib::MPEG::File(m_stream);
#else
    m_file = new TagLib::MPEG::File(m_stream, TagLib::ID3v2::FrameFactory::instance());
#endif
    m_tags << new MpegFileTagModel(using_rusxmms, m_file, TagLib::MPEG::File::ID3v1);
    m_tags << new MpegFileTagModel(using_rusxmms, m_file, TagLib::MPEG::File::ID3v2);
    m_tags << new MpegFileTagModel(using_rusxmms, m_file, TagLib::MPEG::File::APE);
}

MPEGMetaDataModel::~MPEGMetaDataModel()
{
    while(!m_tags.isEmpty())
        delete m_tags.takeFirst();
    delete m_file;
    delete m_stream;
}

QList<MetaDataItem> MPEGMetaDataModel::extraProperties() const
{
    QList<MetaDataItem> ep;
    TagLib::MPEG::Properties *ap = m_file->audioProperties();

    switch(ap->channelMode())
    {
    case TagLib::MPEG::Header::Stereo:
        ep << MetaDataItem(tr("Mode"), u"Stereo"_s);
        break;
    case TagLib::MPEG::Header::JointStereo:
        ep << MetaDataItem(tr("Mode"), u"Joint stereo"_s);
        break;
    case TagLib::MPEG::Header::DualChannel:
        ep << MetaDataItem(tr("Mode"), u"Dual channel"_s);
        break;
    case TagLib::MPEG::Header::SingleChannel:
        ep << MetaDataItem(tr("Mode"), u"Single channel"_s);
        break;
    }
    ep << MetaDataItem(tr("Protection"), ap->protectionEnabled());
    ep << MetaDataItem(tr("Copyright"), ap->isCopyrighted());
    ep << MetaDataItem(tr("Original"), ap->isOriginal());

    return ep;
}

QList<TagModel* > MPEGMetaDataModel::tags() const
{
    return m_tags;
}

QImage MPEGMetaDataModel::cover() const
{
    if(!m_file->ID3v2Tag())
        return QImage();
    TagLib::ID3v2::FrameList frames = m_file->ID3v2Tag()->frameListMap()["APIC"];
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

void MPEGMetaDataModel::setCover(const QImage &img)
{
    TagLib::ID3v2::Tag *tag = m_file->ID3v2Tag(true);
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
    m_file->save(TagLib::MPEG::File::ID3v2);
}

void MPEGMetaDataModel::removeCover()
{
    if(m_file->ID3v2Tag())
    {
        m_file->ID3v2Tag()->removeFrames("APIC");
        m_file->save(TagLib::MPEG::File::ID3v2);
    }
}

QString MPEGMetaDataModel::lyrics() const
{
    for(const TagModel *tag : std::as_const(m_tags))
    {
        const MpegFileTagModel *mpegTag = static_cast<const MpegFileTagModel *>(tag);
        if(mpegTag->type() == TagLib::MPEG::File::ID3v2)
            return mpegTag->lyrics();

    }

    return QString();
}

void MPEGMetaDataModel::setLyrics(const QString &content)
{
    for(TagModel *tag : std::as_const(m_tags))
    {
        MpegFileTagModel *mpegTag = static_cast<MpegFileTagModel *>(tag);
        if(mpegTag->type() == TagLib::MPEG::File::ID3v2)
        {
            mpegTag->setLyrics(content);
            return;
        }
    }
}

MpegFileTagModel::MpegFileTagModel(bool using_rusxmms, TagLib::MPEG::File *file, TagLib::MPEG::File::TagTypes type) :
    TagModel(TagModel::CreateRemove),
    m_using_rusxmms(using_rusxmms),
    m_file(file),
    m_type(type)
{
    QByteArray codecName;
    QSettings settings;
    settings.beginGroup(u"MPEG"_s);
    if(m_type == TagLib::MPEG::File::ID3v1)
    {
        m_tag = m_file->ID3v1Tag();
        if((codecName = settings.value(u"ID3v1_encoding"_s, u"locale"_s).toByteArray()).isEmpty())
            codecName = "ISO-8859-1";

        if(codecName == "locale"_ba && !m_using_rusxmms)
            codecName = TagExtractor::charsetForLocale();
    }
    else if(m_type == TagLib::MPEG::File::ID3v2)
    {
        m_tag = m_file->ID3v2Tag();
        if((codecName = settings.value(u"ID3v2_encoding"_s, u"UTF-8"_s).toByteArray()).isEmpty())
            codecName = "UTF-8";
    }
    else
    {
        m_tag = m_file->APETag();
        codecName = "UTF-8";
    }

    if(m_using_rusxmms || codecName.contains("UTF") || codecName.isEmpty())
        codecName = "UTF-8";

    if(m_tag && !m_using_rusxmms && (m_type == TagLib::MPEG::File::ID3v1 || m_type == TagLib::MPEG::File::ID3v2) &&
            settings.value(u"detect_encoding"_s, false).toBool())
    {
        QByteArray detectedCharset = TagExtractor::detectCharset(m_tag);
        if(!detectedCharset.isEmpty())
            codecName = detectedCharset;
    }

    m_codec = new QmmpTextCodec(codecName);

    settings.endGroup();
}

MpegFileTagModel::~MpegFileTagModel()
{
    delete m_codec;
}

QString MpegFileTagModel::name() const
{
    if(m_type == TagLib::MPEG::File::ID3v1)
        return u"ID3v1"_s;

    if(m_type == TagLib::MPEG::File::ID3v2)
        return u"ID3v2"_s;

    return u"APE"_s;
}

QList<Qmmp::MetaData> MpegFileTagModel::keys() const
{
    QList<Qmmp::MetaData> list = TagModel::keys();
    if(m_type == TagLib::MPEG::File::ID3v2)
        return list;

    if(m_type == TagLib::MPEG::File::APE)
    {
        list.removeAll(Qmmp::DISCNUMBER);
        return list;
    }
    list.removeAll(Qmmp::COMPOSER);
    list.removeAll(Qmmp::ALBUMARTIST);
    list.removeAll(Qmmp::DISCNUMBER);
    return list;
}

QString MpegFileTagModel::value(Qmmp::MetaData key) const
{
    if(!m_tag)
        return QString();

    bool utf = m_codec->name().contains("UTF");

    TagLib::String str;
    switch((int) key)
    {
    case Qmmp::TITLE:
        str = m_tag->title();
        break;
    case Qmmp::ARTIST:
        str = m_tag->artist();
        break;
    case Qmmp::ALBUMARTIST:
        if(m_type == TagLib::MPEG::File::ID3v2 &&
                !m_file->ID3v2Tag()->frameListMap()["TPE2"].isEmpty())
        {
            str = m_file->ID3v2Tag()->frameListMap()["TPE2"].front()->toString();
        }
        else if(m_type == TagLib::MPEG::File::APE &&
                !m_file->APETag()->itemListMap()["ALBUM ARTIST"].isEmpty())
        {
            str = m_file->APETag()->itemListMap()["ALBUM ARTIST"].toString();
        }
        break;
    case Qmmp::ALBUM:
        str = m_tag->album();
        break;
    case Qmmp::COMMENT:
        str = m_tag->comment();
        break;
    case Qmmp::GENRE:
        str = m_tag->genre();
        break;
    case Qmmp::COMPOSER:
        if(m_type == TagLib::MPEG::File::ID3v2 &&
                !m_file->ID3v2Tag()->frameListMap()["TCOM"].isEmpty())
        {
            str = m_file->ID3v2Tag()->frameListMap()["TCOM"].front()->toString();
        }
        else if(m_type == TagLib::MPEG::File::APE &&
                !m_file->APETag()->itemListMap()["COMPOSER"].isEmpty())
        {
            str = m_file->APETag()->itemListMap()["COMPOSER"].toString();
        }
        break;
    case Qmmp::YEAR:
        return QString::number(m_tag->year());
    case Qmmp::TRACK:
        return QString::number(m_tag->track());
    case  Qmmp::DISCNUMBER:
        if(m_type == TagLib::MPEG::File::ID3v2
                && !m_file->ID3v2Tag()->frameListMap()["TPOS"].isEmpty())
            str = m_file->ID3v2Tag()->frameListMap()["TPOS"].front()->toString();
    }
    return m_codec->toUnicode(str.toCString(utf)).trimmed();
}

void MpegFileTagModel::setValue(Qmmp::MetaData key, const QString &value)
{
    if(!m_tag)
        return;

    TagLib::String::Type type = TagLib::String::Latin1;

    if(m_type == TagLib::MPEG::File::ID3v1)
    {
        if(m_codec->name().contains("UTF") && !m_using_rusxmms) //utf is unsupported
            return;

        if(m_using_rusxmms)
            type = TagLib::String::UTF8;
    }
    else if(m_type == TagLib::MPEG::File::ID3v2)
    {
        if(m_codec->name().contains("UTF"))
            type = TagLib::String::UTF8;

        TagLib::ID3v2::FrameFactory::instance()->setDefaultTextEncoding(type);

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
            TagLib::String composer = TagLib::String(m_codec->fromUnicode(value).constData(), type);
            TagLib::ID3v2::Tag *id3v2_tag = dynamic_cast<TagLib::ID3v2::Tag *>(m_tag);
            if(value.isEmpty())
                id3v2_tag->removeFrames(id3v2_key);
            else if(!id3v2_tag->frameListMap()[id3v2_key].isEmpty())
                id3v2_tag->frameListMap()[id3v2_key].front()->setText(composer);
            else
            {
                TagLib::ID3v2::TextIdentificationFrame *frame;
                frame = new TagLib::ID3v2::TextIdentificationFrame(id3v2_key, type);
                frame->setText(composer);
                id3v2_tag->addFrame(frame);
            }
            return;
        }
    }
    else if(m_type == TagLib::MPEG::File::APE)
    {
        type = TagLib::String::UTF8;
    }

    TagLib::String str = TagLib::String(m_codec->fromUnicode(value).constData(), type);

    if(m_type == TagLib::MPEG::File::APE)
    {
        if(key == Qmmp::COMPOSER)
        {
            m_file->APETag()->addValue("COMPOSER", str, true);
            return;
        }

        if(key == Qmmp::ALBUMARTIST)
        {
            m_file->APETag()->addValue("ALBUM ARTIST", str, true);
            return;
        }
    }

    switch((int) key)
    {
    case Qmmp::TITLE:
        m_tag->setTitle(str);
        break;
    case Qmmp::ARTIST:
        m_tag->setArtist(str);
        break;
    case Qmmp::ALBUM:
        m_tag->setAlbum(str);
        break;
    case Qmmp::COMMENT:
        m_tag->setComment(str);
        break;
    case Qmmp::GENRE:
        m_tag->setGenre(str);
        break;
    case Qmmp::YEAR:
        m_tag->setYear(value.toInt());
        break;
    case Qmmp::TRACK:
        m_tag->setTrack(value.toInt());
    }
}

bool MpegFileTagModel::exists() const
{
    return(m_tag != nullptr);
}

void MpegFileTagModel::create()
{
    if(m_tag)
        return;
    if(m_type == TagLib::MPEG::File::ID3v1)
        m_tag = m_file->ID3v1Tag(true);
    else if(m_type == TagLib::MPEG::File::ID3v2)
        m_tag = m_file->ID3v2Tag(true);
    else if(m_type == TagLib::MPEG::File::APE)
        m_tag = m_file->APETag(true);
}

void MpegFileTagModel::remove()
{
    m_tag = nullptr;
}

void MpegFileTagModel::save()
{
    if(m_tag)
        m_file->save(m_type, TagLib::File::StripNone, TagLib::ID3v2::v4, TagLib::File::DoNotDuplicate);
    else
        m_file->strip(m_type);
}

TagLib::MPEG::File::TagTypes MpegFileTagModel::type() const
{
    return m_type;
}

QString MpegFileTagModel::lyrics() const
{
    if(m_type == TagLib::MPEG::File::ID3v2 && m_tag)
    {
        bool utf = m_codec->name().contains("UTF");
        TagLib::ID3v2::Tag *id3v2_tag = static_cast<TagLib::ID3v2::Tag *>(m_tag);

        const TagLib::ID3v2::FrameListMap& map = id3v2_tag->frameListMap();

        if(!map["USLT"].isEmpty())
            return m_codec->toUnicode(map["USLT"].front()->toString().toCString(utf));
    }

    return QString();
}

void MpegFileTagModel::setLyrics(const QString &content)
{
    if(m_type == TagLib::MPEG::File::ID3v2 && m_tag)
    {
        TagLib::ID3v2::Tag *id3v2_tag = static_cast<TagLib::ID3v2::Tag *>(m_tag);

        if(content.isEmpty())
        {
            id3v2_tag->removeFrames("USLT");
        }
        else
        {
            TagLib::String::Type type = m_codec->name().contains("UTF") ?  TagLib::String::UTF8 : TagLib::String::Latin1;
            TagLib::ID3v2::FrameFactory::instance()->setDefaultTextEncoding(type);
            TagLib::String lyrics = TagLib::String(m_codec->fromUnicode(content).constData(), type);

            if(!id3v2_tag->frameListMap()["USLT"].isEmpty())
                id3v2_tag->frameListMap()["USLT"].front()->setText(lyrics);
            else
            {
                TagLib::ID3v2::UnsynchronizedLyricsFrame *frame = new TagLib::ID3v2::UnsynchronizedLyricsFrame(type);
                frame->setText(lyrics);
                id3v2_tag->addFrame(frame);
            }
        }

        m_file->save(m_type, TagLib::File::StripNone, TagLib::ID3v2::v4, TagLib::File::DoNotDuplicate);
    }
}
