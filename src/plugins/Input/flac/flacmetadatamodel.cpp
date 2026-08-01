/***************************************************************************
 *   Copyright (C) 2009-2026 by Ilya Kotov                                 *
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

#include <QImage>
#include <QBuffer>
#include <taglib/tag.h>
#include <taglib/fileref.h>
#include <taglib/tmap.h>
#include <taglib/id3v2framefactory.h>
#include <taglib/flacpicture.h>
#include <taglib/id3v2tag.h>
#include <taglib/textidentificationframe.h>
#include <qmmp/metadatamanager.h>
#include "flacmetadatamodel.h"


FLACMetaDataModel::FLACMetaDataModel(const QString &path, bool readOnly) :
    MetaDataModel(readOnly, MetaDataModel::IsCoverEditable)
{
    bool valid = false;
    m_path = path.contains(u"://"_s) ? TrackInfo::pathFromUrl(path) : path;

    if(m_path.endsWith(u".flac"_s, Qt::CaseInsensitive))
    {
        m_stream = new TagLib::FileStream(QStringToFileName(m_path), readOnly);
#if TAGLIB_MAJOR_VERSION >= 2
        m_nativeFlacFile = new TagLib::FLAC::File(m_stream);
#else
        m_nativeFlacFile = new TagLib::FLAC::File(m_stream, TagLib::ID3v2::FrameFactory::instance());
#endif
        m_tag = m_nativeFlacFile->xiphComment();
        if(m_nativeFlacFile->isValid())
        {
            m_tags << new FLACVorbisCommentModel(m_nativeFlacFile);
            m_tags << new FLACID3v2TagModel(m_nativeFlacFile);
            valid = true;
            setReadOnly(m_nativeFlacFile->readOnly());
        }
    }
    else if(m_path.endsWith(u".oga"_s, Qt::CaseInsensitive))
    {
        m_stream = new TagLib::FileStream(QStringToFileName(m_path), readOnly);
        m_oggFlacFile = new TagLib::Ogg::FLAC::File(m_stream);
        m_tag = m_oggFlacFile->tag();
        if(m_oggFlacFile->isValid())
        {
            m_tags << new FLACVorbisCommentModel(m_oggFlacFile);
            valid = true;
            setReadOnly(m_oggFlacFile->readOnly());
        }
    }

    if(valid)
    {
        MetaDataModel::DialogHints hints = dialogHints() | MetaDataModel::IsCueEditable;
        if(!path.contains(u"://"_s)) //hide lyrics editor for files with embedded CUE
            hints |= MetaDataModel::IsLyricsEditable;
        setDialogHints(hints);
    }
}

FLACMetaDataModel::~FLACMetaDataModel()
{
    qDeleteAll(m_tags);
    delete m_nativeFlacFile;
    delete m_oggFlacFile;
    delete m_stream;
}

QList<TagModel* > FLACMetaDataModel::tags() const
{
    return m_tags;
}

QImage FLACMetaDataModel::cover() const
{
    TagLib::List<TagLib::FLAC::Picture *> list;

    if(m_nativeFlacFile)
    {
        list = m_nativeFlacFile->pictureList(); //native flac
    }
    else if(m_tag)
    {
        list = m_tag->pictureList(); //ogg flac
    }

    for(uint i = 0; i < list.size(); ++i)
    {
        if(list[i]->type() == TagLib::FLAC::Picture::FrontCover)
        {
            QImage cover;
            cover.loadFromData(QByteArray(list[i]->data().data(), list[i]->data().size())); //read binary picture data
            return cover;
        }
    }
    return QImage();
}

QString FLACMetaDataModel::coverPath() const
{
    return MetaDataManager::instance()->findCoverFile(m_path);
}

void FLACMetaDataModel::setCover(const QImage &img)
{
    removeCover();

    TagLib::FLAC::Picture *picture = new TagLib::FLAC::Picture();
    picture->setType(TagLib::FLAC::Picture::FrontCover);

    QByteArray data;
    QBuffer buffer(&data);
    buffer.open(QIODevice::WriteOnly);
    img.save(&buffer, "JPEG");
    picture->setMimeType("image/jpeg");
    picture->setData(TagLib::ByteVector(data.constData(), data.size()));
    if(m_nativeFlacFile)
    {
        m_nativeFlacFile->addPicture(picture);
        m_nativeFlacFile->save();
    }
    else if(m_tag && m_oggFlacFile)
    {
        m_tag->addPicture(picture);
        m_oggFlacFile->save();
    }
}

void FLACMetaDataModel::removeCover()
{
    if(m_nativeFlacFile)
    {
        const TagLib::List<TagLib::FLAC::Picture *> list = m_nativeFlacFile->pictureList(); //native flac
        bool save = false;
        for(TagLib::FLAC::Picture *p : std::as_const(list))
        {
            if(p->type() == TagLib::FLAC::Picture::FrontCover)
            {
                m_nativeFlacFile->removePicture(p, false);
                save = true;
            }
        }

        if(save)
            m_nativeFlacFile->save();
    }
    else if(m_oggFlacFile && m_tag && !m_tag->isEmpty())
    {
        const TagLib::List<TagLib::FLAC::Picture *> list = m_tag->pictureList(); //ogg flac
        bool save = false;
        for(TagLib::FLAC::Picture *p : std::as_const(list))
        {
            if(p->type() == TagLib::FLAC::Picture::FrontCover)
            {
                m_tag->removePicture(p, false);
                save = true;
            }
        }

        if(save)
            m_oggFlacFile->save();
    }
}

QString FLACMetaDataModel::cue() const
{
    if(m_tag && m_tag->fieldListMap().contains("CUESHEET"))
    {
        QByteArray data(m_tag->fieldListMap()["CUESHEET"].toString().toCString(true));
        return QString::fromUtf8(data);
    }

    return QString();
}

void FLACMetaDataModel::setCue(const QString &content)
{
    if(!m_tag && m_nativeFlacFile)
        m_tag = m_nativeFlacFile->xiphComment(true);

    if(m_tag)
        m_tag->addField("CUESHEET", QStringToTString(content), true);

    if(m_nativeFlacFile)
        m_nativeFlacFile->save();
    else if(m_oggFlacFile)
        m_oggFlacFile->save();
}

void FLACMetaDataModel::removeCue()
{
    if(m_tag)
    {
        m_tag->removeFields("CUESHEET");
        if(m_nativeFlacFile)
            m_nativeFlacFile->save();
        else if(m_oggFlacFile)
            m_oggFlacFile->save();
    }
}

QString FLACMetaDataModel::lyrics() const
{
    if(m_tag && !m_tag->isEmpty())
    {
        const TagLib::Ogg::FieldListMap map = m_tag->fieldListMap();

        if(!map["UNSYNCEDLYRICS"].isEmpty())
            return TStringToQString(map["UNSYNCEDLYRICS"].front());
    }

    return QString();
}

void FLACMetaDataModel::setLyrics(const QString &content)
{
    if(!m_tag && m_nativeFlacFile)
        m_tag = m_nativeFlacFile->xiphComment(true);

    if(m_tag)
    {
        m_tag->addField("UNSYNCEDLYRICS", QStringToTString(content), true);
    }

    if(m_nativeFlacFile)
        m_nativeFlacFile->save();
    else if(m_oggFlacFile)
        m_oggFlacFile->save();
}

void FLACMetaDataModel::removeLyrics()
{
    if(m_tag)
    {
        m_tag->removeFields("UNSYNCEDLYRICS");
        if(m_nativeFlacFile)
            m_nativeFlacFile->save();
        else if(m_oggFlacFile)
            m_oggFlacFile->save();
    }
}

FLACVorbisCommentModel::FLACVorbisCommentModel(TagLib::FLAC::File *file) :
    TagModel(),
    m_nativeFlacFile(file),
    m_tag(file->xiphComment())
{}

FLACVorbisCommentModel::FLACVorbisCommentModel(TagLib::Ogg::FLAC::File *file) :
    TagModel(),
    m_oggFlacFile(file),
    m_tag(file->tag())
{}

QString FLACVorbisCommentModel::name() const
{
    return u"Vorbis Comment"_s;
}

QString FLACVorbisCommentModel::value(Qmmp::MetaData key) const
{
    if(!m_tag)
        return QString();

    switch((int) key)
    {
    case Qmmp::TITLE:
        return TStringToQString(m_tag->title());
    case Qmmp::ARTIST:
        return TStringToQString(m_tag->artist());
    case Qmmp::ALBUMARTIST:
        if(m_tag->fieldListMap()["ALBUMARTIST"].isEmpty())
            return QString();
        else
            return TStringToQString(m_tag->fieldListMap()["ALBUMARTIST"].toString());
    case Qmmp::ALBUM:
        return TStringToQString(m_tag->album());
    case Qmmp::COMMENT:
        return TStringToQString(m_tag->comment());
    case Qmmp::GENRE:
        return TStringToQString(m_tag->genre());
    case Qmmp::COMPOSER:
        if(m_tag->fieldListMap()["COMPOSER"].isEmpty())
            return QString();
        else
            return TStringToQString(m_tag->fieldListMap()["COMPOSER"].toString());
    case Qmmp::YEAR:
        return QString::number(m_tag->year());
    case Qmmp::TRACK:
        return QString::number(m_tag->track());
    case  Qmmp::DISCNUMBER:
        if(m_tag->fieldListMap()["DISCNUMBER"].isEmpty())
            return QString();
        else
            return TStringToQString(m_tag->fieldListMap()["DISCNUMBER"].toString());
    }
    return QString();
}

void FLACVorbisCommentModel::setValue(Qmmp::MetaData key, const QString &value)
{
    if(!m_tag && m_nativeFlacFile)
        m_tag = m_nativeFlacFile->xiphComment(true);

    if(!m_tag)
        return;

    TagLib::String str = QStringToTString(value);

    switch((int) key)
    {
    case Qmmp::TITLE:
        m_tag->setTitle(str);
        return;
    case Qmmp::ARTIST:
        m_tag->setArtist(str);
        return;
    case Qmmp::ALBUMARTIST:
        m_tag->addField("ALBUMARTIST", str, true);
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
    case Qmmp::COMPOSER:
        m_tag->addField("COMPOSER", str, true);
        return;
    case Qmmp::TRACK:
        m_tag->setTrack(value.toInt());
        return;
    case Qmmp::YEAR:
        m_tag->setYear(value.toInt());
        return;
    case Qmmp::DISCNUMBER:
        if(value == "0"_L1)
            m_tag->removeFields("DISCNUMBER");
        else
            m_tag->addField("DISCNUMBER", str, true);
    }
}

void FLACVorbisCommentModel::save()
{
    if(m_nativeFlacFile)
        m_nativeFlacFile->save();
    else if(m_oggFlacFile)
        m_oggFlacFile->save();
}

FLACID3v2TagModel::FLACID3v2TagModel(TagLib::FLAC::File *file) :
    TagModel(TagModel::CreateRemove),
    m_file(file),
    m_tag(file->ID3v2Tag())
{}

QString FLACID3v2TagModel::name() const
{
    return u"ID3v2"_s;
}

QString FLACID3v2TagModel::value(Qmmp::MetaData key) const
{
    if(!m_tag)
        return QString();

    TagLib::String str;

    switch(key)
    {
    case Qmmp::TITLE:
        str = m_tag->title();
        break;
    case Qmmp::ARTIST:
        str = m_tag->artist();
        break;
    case Qmmp::ALBUMARTIST:
        if(!m_tag->frameListMap()["TPE2"].isEmpty())
        {
            str = m_tag->frameListMap()["TPE2"].front()->toString();
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
        if(!m_tag->frameListMap()["TCOM"].isEmpty())
        {
            str = m_tag->frameListMap()["TCOM"].front()->toString();
        }
        break;
    case Qmmp::YEAR:
        return QString::number(m_tag->year());
    case Qmmp::TRACK:
        return QString::number(m_tag->track());
    case  Qmmp::DISCNUMBER:
        if(!m_tag->frameListMap()["TPOS"].isEmpty())
        {
            str = m_tag->frameListMap()["TPOS"].front()->toString();
        }
        break;
    case Qmmp::UNKNOWN:
        break;
    }
    return TStringToQString(str);
}

void FLACID3v2TagModel::setValue(Qmmp::MetaData key, const QString &value)
{
    if(!m_tag)
        return;

    TagLib::String str = QStringToTString(value);
    TagLib::ID3v2::FrameFactory::instance()->setDefaultTextEncoding(TagLib::String::UTF8);

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

    switch(key)
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
    default:
        break;
    }
}

bool FLACID3v2TagModel::exists() const
{
    return m_tag != nullptr;
}

void FLACID3v2TagModel::create()
{
    m_tag = m_file->ID3v2Tag(true);
}

void FLACID3v2TagModel::remove()
{
    m_tag = nullptr;
}

void FLACID3v2TagModel::save()
{
    if(!m_tag)
        m_file->strip(TagLib::FLAC::File::ID3v2);
    m_file->save();
}
