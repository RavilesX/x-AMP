/***************************************************************************
 *   Copyright (C) 2008-2026 by Ilya Kotov                                 *
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

#include <QDialog>
#include <QMessageBox>
#include <QSettings>
#include <QFile>
#include <taglib/tag.h>
#include <taglib/fileref.h>
#include <taglib/id3v1tag.h>
#include <taglib/id3v2tag.h>
#include <taglib/apetag.h>
#include <taglib/tfile.h>
#include <taglib/mpegfile.h>
#include <taglib/tfilestream.h>
#include <taglib/id3v2header.h>
#include <taglib/textidentificationframe.h>
#include <taglib/id3v2framefactory.h>
#include <qmmp/qmmptextcodec.h>
#include "tagextractor.h"
#include "mpegmetadatamodel.h"
#include "mpegsettingsdialog.h"
#ifdef WITH_MAD
#include "decoder_mad.h"
#endif
#ifdef WITH_MPG123
#include "decoder_mpg123.h"
#endif
#include "decodermpegfactory.h"

#define CSTR_TO_QSTR(str,utf) codec->toUnicode(str.toCString(utf)).trimmed()

// DecoderMPEGFactory

DecoderMpegFactory::DecoderMpegFactory()
{
    //detecting rusxmms patch
    m_using_rusxmms = false;
    char str[] = { char(0xF2), char(0xE5), char(0xF1), char(0xF2), '\0'};
    QmmpTextCodec codec("windows-1251");
    TagLib::String tstr(str);
    if(codec.toUnicode(str) == QString::fromUtf8(tstr.toCString(true)))
    {
        qCDebug(plugin, "found taglib with rusxmms patch");
        m_using_rusxmms = true;
        TagExtractor::setForceUtf8(m_using_rusxmms);
    }
}

bool DecoderMpegFactory::canDecode(QIODevice *input) const
{
    char buf[8192];
    qint64 dataSize = sizeof(buf);

    if(input->peek(buf, sizeof(buf)) != sizeof(buf))
        return false;

    if (!memcmp(buf, "FLV", 3)) //skip Macromedia Flash Video
        return false;

    if (!memcmp(buf + 8, "WAVE", 4))
        return !memcmp(buf + 20, "U" ,1);

    if(!memcmp(buf, "ID3", 3))
    {
        TagLib::ByteVector byteVector(buf, dataSize);
        TagLib::ID3v2::Header header(byteVector);

        //skip id3v2tag if possible
        if(input->isSequential())
        {
            if(header.tagSize() >= dataSize)
                return false;

            dataSize -= header.tagSize();
            memmove(buf, buf + header.tagSize(), dataSize);
        }
        else
        {
            input->seek(header.tagSize());
            dataSize = input->read(buf, sizeof(buf));
            input->seek(0); //restore initial position
        }
    }

    if(dataSize <= 0)
        return false;

#if defined(WITH_MAD) && defined(WITH_MPG123)
    QSettings settings;
    QString decoderName = settings.value(u"MPEG/decoder"_s, u"mpg123"_s).toString();
#elif defined(WITH_MAD)
    QString decoderName = u"mad"_s;
#elif defined(WITH_MPG123)
    QString decoderName = u"mpg123"_s;
#endif

#ifdef WITH_MAD
    if(decoderName != "mpg123"_L1)
    {
        struct mad_stream stream;
        struct mad_header header;
        struct mad_frame frame;
        int dec_res;

        mad_stream_init(&stream);
        mad_header_init(&header);
        mad_frame_init(&frame);
        mad_stream_buffer (&stream, (unsigned char *) buf, dataSize);
        stream.error = MAD_ERROR_NONE;

        while ((dec_res = mad_header_decode(&header, &stream)) == -1
               && MAD_RECOVERABLE(stream.error))
            ;

        if(dec_res == 0)
        {
            while ((dec_res = mad_frame_decode(&frame, &stream)) == -1
                   && MAD_RECOVERABLE(stream.error))
                ;
        }

        mad_stream_finish(&stream);
        mad_frame_finish(&frame);
        return dec_res == 0;
    }
#endif

#ifdef WITH_MPG123
    if (decoderName == "mpg123"_L1)
    {
        mpg123_init();
        mpg123_handle *handle = mpg123_new(nullptr, nullptr);
        if (!handle)
            return false;
        if(mpg123_open_feed(handle) != MPG123_OK)
        {
            mpg123_delete(handle);
            return false;
        }
        if (mpg123_format(handle, 44100, MPG123_STEREO, MPG123_ENC_SIGNED_16) != MPG123_OK)
        {
            mpg123_close(handle);
            mpg123_delete(handle);
            return false;
        }
        size_t out_size = 0;
        int ret = mpg123_decode(handle, (unsigned char*) buf, dataSize, nullptr, 0, &out_size);
        mpg123_close(handle);
        mpg123_delete(handle);
        return ret == MPG123_DONE || ret == MPG123_NEW_FORMAT;
    }
#endif
    return false;
}

DecoderProperties DecoderMpegFactory::properties() const
{
    DecoderProperties properties;
    properties.name = tr("MPEG Plugin");
    properties.shortName = "mpeg"_L1;
    properties.filters = QStringList { u"*.mp1"_s, u"*.mp2"_s, u"*.mp3"_s, u"*.wav"_s };
    properties.description = tr("MPEG Files");
    properties.contentTypes = QStringList { u"audio/mp3"_s, u"audio/mpeg"_s };
    properties.hasAbout = true;
    properties.hasSettings = true;
    return properties;
}

Decoder *DecoderMpegFactory::create(const QString &, QIODevice *input)
{
    Decoder *d = nullptr;
#if defined(WITH_MAD) && defined(WITH_MPG123)
    QSettings settings;
    if(settings.value(u"MPEG/decoder"_s, u"mad"_s).toString() == "mpg123"_L1)
    {
        qCDebug(plugin, "using mpg123 decoder");
        d = new DecoderMPG123(input);
    }
    else
    {
        qCDebug(plugin, "using MAD decoder");
        bool crc = settings.value(u"MPEG/enable_crc"_s, false).toBool();
        d = new DecoderMAD(crc, input);
    }
#elif defined(WITH_MAD)
    QSettings settings;
    bool crc = settings.value(u"MPEG/enable_crc"_s, false).toBool();
    d = new DecoderMAD(crc, input);
#elif defined(WITH_MPG123)
    d = new DecoderMPG123(input);
#endif
    return d;
}

QList<TrackInfo> DecoderMpegFactory::createPlayList(const QString &path, TrackInfo::Parts parts, QStringList *)
{
    TrackInfo info(path);

    if(parts == TrackInfo::Parts())
        return { info };

    TagLib::FileStream stream(QStringToFileName(path), true);
#if TAGLIB_MAJOR_VERSION >= 2
    TagLib::MPEG::File fileRef(&stream);
#else
    TagLib::MPEG::File fileRef(&stream, TagLib::ID3v2::FrameFactory::instance());
#endif

    if (parts & TrackInfo::MetaData)
    {
        QSettings settings;
        settings.beginGroup("MPEG"_L1);

        QList< QMap<Qmmp::MetaData, QString> > metaData;
        uint tag_array[3];
        tag_array[0] = settings.value(u"tag_1"_s, MpegSettingsDialog::ID3v2).toInt();
        tag_array[1] = settings.value(u"tag_2"_s, MpegSettingsDialog::APE).toInt();
        tag_array[2] = settings.value(u"tag_3"_s, MpegSettingsDialog::ID3v1).toInt();
        bool merge = settings.value(u"merge_tags"_s, false).toBool();

        for (int i = 0; i < 3; ++i)
        {
            TagLib::Tag *tag = nullptr;
            QByteArray codecName;

            switch ((uint) tag_array[i])
            {
            case MpegSettingsDialog::ID3v1:
                codecName = settings.value(u"ID3v1_encoding"_s, u"locale"_s).toByteArray();
                if(codecName == "locale"_ba && !m_using_rusxmms)
                    codecName = TagExtractor::charsetForLocale();
                tag = fileRef.ID3v1Tag();
                break;
            case MpegSettingsDialog::ID3v2:
                codecName = settings.value(u"ID3v2_encoding"_s, u"UTF-8"_s).toByteArray();
                tag = fileRef.ID3v2Tag();
                break;
            case MpegSettingsDialog::APE:
                codecName = "UTF-8";
                tag = fileRef.APETag();
                break;
            case MpegSettingsDialog::Disabled:
                break;
            }

            if(m_using_rusxmms || codecName.contains("UTF") || codecName.isEmpty())
                codecName = "UTF-8";

            if (tag && !tag->isEmpty())
            {
                if((tag == fileRef.ID3v1Tag() || tag == fileRef.ID3v2Tag()) && !m_using_rusxmms &&
                        settings.value(u"detect_encoding"_s, false).toBool())
                {
                    QByteArray detectedCharset = TagExtractor::detectCharset(tag);
                    if(!detectedCharset.isEmpty())
                        codecName = detectedCharset;
                }

                QmmpTextCodec *codec = new QmmpTextCodec(codecName);

                bool utf = codec->name().contains("UTF");

                QMap<Qmmp::MetaData, QString> tags = {
                    { Qmmp::ARTIST, CSTR_TO_QSTR(tag->artist(), utf) },
                    { Qmmp::ALBUM, CSTR_TO_QSTR(tag->album(), utf) },
                    { Qmmp::COMMENT, CSTR_TO_QSTR(tag->comment(), utf) },
                    { Qmmp::GENRE, CSTR_TO_QSTR(tag->genre(), utf) },
                    { Qmmp::TITLE, CSTR_TO_QSTR(tag->title(), utf) },
                    { Qmmp::YEAR, QString::number(tag->year()) },
                    { Qmmp::TRACK, QString::number(tag->track()) },
                };

                if(tag == fileRef.ID3v2Tag())
                {
                    if(!fileRef.ID3v2Tag()->frameListMap()["TPE2"].isEmpty())
                    {
                        TagLib::String albumArtist = fileRef.ID3v2Tag()->frameListMap()["TPE2"].front()->toString();
                        tags.insert(Qmmp::ALBUMARTIST, CSTR_TO_QSTR(albumArtist, utf));
                    }
                    if(!fileRef.ID3v2Tag()->frameListMap()["TCOM"].isEmpty())
                    {
                        TagLib::String composer = fileRef.ID3v2Tag()->frameListMap()["TCOM"].front()->toString();
                        tags.insert(Qmmp::COMPOSER, CSTR_TO_QSTR(composer, utf));
                    }
                    if(!fileRef.ID3v2Tag()->frameListMap()["TPOS"].isEmpty())
                    {
                        TagLib::String disc = fileRef.ID3v2Tag()->frameListMap()["TPOS"].front()->toString();
                        tags.insert(Qmmp::DISCNUMBER, CSTR_TO_QSTR(disc, utf));
                    }
                }
                else if(tag == fileRef.APETag())
                {
                    TagLib::APE::Item fld;
                    if(!(fld = fileRef.APETag()->itemListMap()["ALBUM ARTIST"]).isEmpty())
                        tags.insert(Qmmp::ALBUMARTIST, CSTR_TO_QSTR(fld.toString(), true));
                    if(!(fld = fileRef.APETag()->itemListMap()["COMPOSER"]).isEmpty())
                        tags.insert(Qmmp::COMPOSER, CSTR_TO_QSTR(fld.toString(), true));
                }

                metaData << tags;

                delete codec;

                if(!merge)
                    break;
            }
        }
        settings.endGroup();

        for(const QMap<Qmmp::MetaData, QString> &tags : std::as_const(metaData))
        {
            for(auto it = tags.cbegin(); it != tags.cend(); ++it)
            {
                if(info.value(it.key()).length() < it.value().length())
                    info.setValue(it.key(), it.value());
            }
        }
    }

    if((parts & TrackInfo::Properties) && fileRef.audioProperties())
    {
        info.setValue(Qmmp::BITRATE, fileRef.audioProperties()->bitrate());
        info.setValue(Qmmp::SAMPLERATE, fileRef.audioProperties()->sampleRate());
        info.setValue(Qmmp::CHANNELS, fileRef.audioProperties()->channels());
        info.setValue(Qmmp::BITS_PER_SAMPLE, 32); //float
        switch(fileRef.audioProperties()->version())
        {
        case TagLib::MPEG::Header::Version1:
            info.setValue(Qmmp::FORMAT_NAME, QStringLiteral("MPEG-1 layer %1").arg(fileRef.audioProperties()->layer()));
            break;
        case TagLib::MPEG::Header::Version2:
            info.setValue(Qmmp::FORMAT_NAME, QStringLiteral("MPEG-2 layer %1").arg(fileRef.audioProperties()->layer()));
            break;
        case TagLib::MPEG::Header::Version2_5:
            info.setValue(Qmmp::FORMAT_NAME, QStringLiteral("MPEG-2.5 layer %1").arg(fileRef.audioProperties()->layer()));
            break;
#if TAGLIB_MAJOR_VERSION >= 2
        case TagLib::MPEG::Header::Version4:
            info.setValue(Qmmp::FORMAT_NAME, QStringLiteral("MPEG-4 layer %1").arg(fileRef.audioProperties()->layer()));
#endif
        }
        info.setDuration(fileRef.audioProperties()->lengthInMilliseconds());
    }

    if(parts & TrackInfo::ReplayGainInfo)
    {
        if(fileRef.ID3v2Tag() && !fileRef.ID3v2Tag()->isEmpty())
        {
            TagLib::ID3v2::Tag *tag = fileRef.ID3v2Tag();
            TagLib::ID3v2::UserTextIdentificationFrame *frame = nullptr;
            TagLib::ID3v2::FrameList frames = tag->frameList("TXXX");
            for(TagLib::ID3v2::FrameList::Iterator it = frames.begin(); it != frames.end(); ++it)
            {
                frame = dynamic_cast<TagLib::ID3v2::UserTextIdentificationFrame*>(*it);
                if(!frame || frame->fieldList().size() < 2)
                    continue;

                TagLib::String desc = frame->description().upper();
                if (desc == "REPLAYGAIN_TRACK_GAIN")
                    info.setValue(Qmmp::REPLAYGAIN_TRACK_GAIN, TStringToQString(frame->fieldList()[1]));
                else if (desc == "REPLAYGAIN_TRACK_PEAK")
                    info.setValue(Qmmp::REPLAYGAIN_TRACK_PEAK, TStringToQString(frame->fieldList()[1]));
                else if (desc == "REPLAYGAIN_ALBUM_GAIN")
                    info.setValue(Qmmp::REPLAYGAIN_ALBUM_GAIN, TStringToQString(frame->fieldList()[1]));
                else if (desc == "REPLAYGAIN_ALBUM_PEAK")
                    info.setValue(Qmmp::REPLAYGAIN_ALBUM_PEAK, TStringToQString(frame->fieldList()[1]));
            }
        }
        if(info.replayGainInfo().isEmpty() && fileRef.APETag() && !fileRef.APETag()->isEmpty())
        {
            TagLib::APE::Tag *tag = fileRef.APETag();
            TagLib::APE::ItemListMap items = tag->itemListMap();
            if (items.contains("REPLAYGAIN_TRACK_GAIN"))
                info.setValue(Qmmp::REPLAYGAIN_TRACK_GAIN,TStringToQString(items["REPLAYGAIN_TRACK_GAIN"].values()[0]));
            if (items.contains("REPLAYGAIN_TRACK_PEAK"))
                info.setValue(Qmmp::REPLAYGAIN_TRACK_PEAK,TStringToQString(items["REPLAYGAIN_TRACK_PEAK"].values()[0]));
            if (items.contains("REPLAYGAIN_ALBUM_GAIN"))
                info.setValue(Qmmp::REPLAYGAIN_ALBUM_GAIN,TStringToQString(items["REPLAYGAIN_ALBUM_GAIN"].values()[0]));
            if (items.contains("REPLAYGAIN_ALBUM_PEAK"))
                info.setValue(Qmmp::REPLAYGAIN_ALBUM_PEAK,TStringToQString(items["REPLAYGAIN_ALBUM_PEAK"].values()[0]));
        }
    }

    return { info };
}

MetaDataModel* DecoderMpegFactory::createMetaDataModel(const QString &path, bool readOnly)
{
   return new MPEGMetaDataModel(m_using_rusxmms, path, readOnly);
}

QDialog *DecoderMpegFactory::createSettings(QWidget *parent)
{
    return new MpegSettingsDialog(m_using_rusxmms, parent);
}

void DecoderMpegFactory::showAbout(QWidget *parent)
{
    QMessageBox::about(parent, tr("About MPEG Audio Plugin"),
                       tr("MPEG 1.0/2.0/2.5 layer 1/2/3 audio decoder") + QChar::LineFeed +
                       tr("Compiled against:") + QChar::LineFeed +
#ifdef WITH_MAD
                       QStringLiteral("libmad-%1.%2.%3%4")
                       .arg(MAD_VERSION_MAJOR)
                       .arg(MAD_VERSION_MINOR)
                       .arg(MAD_VERSION_PATCH)
                       .arg(QLatin1StringView(MAD_VERSION_EXTRA)) + QChar::LineFeed +
#endif
#ifdef WITH_MPG123
                       tr("mpg123, API version: %1")
                       .arg(MPG123_API_VERSION) + QChar::LineFeed +
#endif
                       QStringLiteral("TagLib-%1.%2.%3\n").arg(TAGLIB_MAJOR_VERSION).arg(TAGLIB_MINOR_VERSION).arg(TAGLIB_PATCH_VERSION) +
                       tr("Written by: Ilya Kotov <forkotov02@ya.ru>") + QChar::LineFeed +
                       tr("Source code based on mq3 and madplay projects")
                       );
}

QString DecoderMpegFactory::translation() const
{
    return QLatin1String(":/mpeg_plugin_");
}
