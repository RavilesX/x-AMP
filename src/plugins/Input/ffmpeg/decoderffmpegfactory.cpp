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

#include <QSettings>
#include <QMessageBox>
#include <QFileInfo>
#include <QSet>
#include <qmmp/cueparser.h>
#include <taglib/taglib.h>
extern "C"{
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/dict.h>
#include <libavutil/avutil.h>
}

#include "ffmpegmetadatamodel.h"
#include "ffmpegsettingsdialog.h"
#include "decoder_ffmpeg.h"
#include "decoder_ffmpegcue.h"
#include "decoder_ffmpegm4b.h"
#include "decoderffmpegfactory.h"


// DecoderFFmpegFactory

DecoderFFmpegFactory::DecoderFFmpegFactory()
{
#if (LIBAVCODEC_VERSION_INT < AV_VERSION_INT(58,10,100)) //ffmpeg-3.5
    avcodec_register_all();
    avformat_network_init();
    av_register_all();
#endif
}

bool DecoderFFmpegFactory::canDecode(QIODevice *i) const
{
    QStringList filters = properties().filters;

    AVProbeData pd;
    memset(&pd, 0, sizeof(pd));
    uint8_t buf[PROBE_BUFFER_SIZE + AVPROBE_PADDING_SIZE];
    pd.buf_size = i->peek((char*)buf, sizeof(buf) - AVPROBE_PADDING_SIZE);
    pd.buf = buf;
    if(pd.buf_size < PROBE_BUFFER_SIZE)
        return false;
    const AVInputFormat *fmt = av_probe_input_format(&pd, 1);
    if(!fmt)
        return false;

    QStringList formats = QString::fromLatin1(fmt->name).split(QLatin1Char(','));

    if(filters.contains(u"*.wma"_s) && formats.contains(u"asf"_s))
        return true;
    if(filters.contains(u"*.mp3"_s) && formats.contains(u"mp3"_s))
        return true;
    if(filters.contains(u"*.aac"_s) && formats.contains(u"aac"_s))
        return true;
    if(filters.contains(u"*.ac3"_s) && (formats.contains(u"ac3"_s) || formats.contains(u"eac3"_s)))
        return true;
    if(filters.contains(u"*.dts"_s) && formats.contains(u"dts"_s))
        return true;
    if(filters.contains(u"*.mka"_s) && (formats.contains(u"mka"_s) || formats.contains(u"matroska"_s)))
        return true;
    if(filters.contains(u"*.webm"_s) && formats.contains(u"webm"_s) &&
        (avcodec_find_decoder(AV_CODEC_ID_OPUS) || avcodec_find_decoder(AV_CODEC_ID_VORBIS)))
        return true;
    if(filters.contains(u"*.vqf"_s) && formats.contains(u"vqf"_s))
        return true;
    if(filters.contains(u"*.ape"_s) && formats.contains(u"ape"_s))
        return true;
    if(filters.contains(u"*.tta"_s) && formats.contains(u"tta"_s))
        return true;
    if(filters.contains(u"*.m4a"_s) && (formats.contains(u"m4a"_s) || formats.contains(u"mp4"_s)))
        return true;
    if(filters.contains(u"*.tak"_s) && formats.contains(u"tak"_s))
        return true;
    if(filters.contains(u"*.dsf"_s) && formats.contains(u"dsf"_s))
        return true;
    if(formats.contains(u"matroska"_s) && avcodec_find_decoder(AV_CODEC_ID_OPUS) && i->isSequential()) //audio from YouTube
        return true;

    return false;
}

DecoderProperties DecoderFFmpegFactory::properties() const
{
    QSettings settings;
    QSet<QString> filters = {
        u"*.wma"_s, u"*.ape"_s, u"*.tta"_s, u"*.m4a"_s, u"*.m4b"_s, u"*.aac"_s, u"*.mp3"_s, u"*.ra"_s, u"*.shn"_s,
        u"*.ac3"_s, u"*.dts"_s, u"*.mka"_s,  u"*.vqf"_s, u"*.tak"_s, u"*.dsf"_s, u"*.dsdiff"_s, u"*.webm"_s
    };
    const QStringList disabledFilters = settings.value(u"FFMPEG/disabled_filters"_s, { u"*.mp3"_s }).toStringList();

    for(const QString &filter : std::as_const(disabledFilters))
        filters.remove(filter);

    if(!avcodec_find_decoder(AV_CODEC_ID_WMAV1))
        filters.remove(u"*.wma"_s);
    if(!avcodec_find_decoder(AV_CODEC_ID_APE))
        filters.remove(u"*.ape"_s);
    if(!avcodec_find_decoder(AV_CODEC_ID_TTA))
        filters.remove(u"*.tta"_s);
    if(!avcodec_find_decoder(AV_CODEC_ID_AAC))
        filters.remove(u"*.aac"_s);
    if(!avcodec_find_decoder(AV_CODEC_ID_MP3))
        filters.remove(u"*.mp3"_s);
    if(!avcodec_find_decoder(AV_CODEC_ID_AAC) && !avcodec_find_decoder(AV_CODEC_ID_ALAC))
    {
        filters.remove(u"*.m4a"_s);
        filters.remove(u"*.m4b"_s);
    }
    if(!avcodec_find_decoder(AV_CODEC_ID_RA_288))
        filters.remove(u"*.ra"_s);
    if(!avcodec_find_decoder(AV_CODEC_ID_SHORTEN))
        filters.remove(u"*.shn"_s);
    if(!avcodec_find_decoder(AV_CODEC_ID_AC3) && !avcodec_find_decoder(AV_CODEC_ID_EAC3))
        filters.remove(u"*.ac3"_s);
    if(!avcodec_find_decoder(AV_CODEC_ID_DTS))
        filters.remove(u"*.dts"_s);
    if(!av_find_input_format("matroska") || !avcodec_find_decoder(AV_CODEC_ID_TRUEHD))
        filters.remove(u"*.mka"_s);
    if(!av_find_input_format("webm") || (!avcodec_find_decoder(AV_CODEC_ID_OPUS) && !avcodec_find_decoder(AV_CODEC_ID_VORBIS)))
        filters.remove(u"*.webm"_s);
    if(!avcodec_find_decoder(AV_CODEC_ID_TWINVQ))
        filters.remove(u"*.vqf"_s);
    if(!avcodec_find_decoder(AV_CODEC_ID_TAK))
        filters.remove(u"*.tak"_s);
    if(!avcodec_find_decoder(AV_CODEC_ID_DSD_LSBF))
    {
        filters.remove(u"*.dsf"_s);
        filters.remove(u"*.dsdiff"_s);
    }

    DecoderProperties properties;
    properties.name = tr("FFmpeg Plugin");
    properties.filters = QStringList(filters.cbegin(), filters.cend());
    properties.description = tr("FFmpeg Formats");
    if(filters.contains(u"*.wma"_s))
        properties.contentTypes << u"audio/x-ms-wma"_s;
    if(filters.contains(u"*.mp3"_s))
        properties.contentTypes << u"audio/mpeg"_s;
    if(filters.contains(u"*.aac"_s))
        properties.contentTypes << u"audio/aac"_s << u"audio/aacp"_s;
    if(filters.contains(u"*.shn"_s))
        properties.contentTypes << u"audio/x-ffmpeg-shorten"_s;
    if(filters.contains(u"*.m4a"_s))
    {
        properties.contentTypes << u"audio/3gpp"_s << u"audio/3gpp2"_s << u"audio/mp4"_s;
        properties.contentTypes << u"audio/MP4A-LATM"_s << u"audio/mpeg4-generic"_s;
        properties.contentTypes << u"audio/m4a"_s;
    }
    if(filters.contains(u"*.ac3"_s))
        properties.contentTypes << u"audio/ac3"_s << u"audio/eac3"_s;
    if(filters.contains(u"*.dts"_s))
        properties.contentTypes << u"audio/dts"_s;
    if(filters.contains(u"*.mka"_s))
        properties.contentTypes << u"audio/true-hd"_s << u"audio/x-matroska"_s;
    if(filters.contains(u"*.webm"_s))
        properties.contentTypes << u"audio/webm"_s;
    properties.shortName = "ffmpeg"_L1;
    properties.hasAbout = true;
    properties.hasSettings = true;
    properties.noInput = false;
    properties.protocols << u"ffmpeg"_s << u"m4b"_s;
    properties.priority = 10;
    return properties;
}

Decoder *DecoderFFmpegFactory::create(const QString &path, QIODevice *input)
{
    if(path.startsWith(u"ffmpeg://"_s))
        return new DecoderFFmpegCue(path);

    if(path.startsWith(u"m4b://"_s))
        return new DecoderFFmpegM4b(this, path);

    return new DecoderFFmpeg(path, input);
}

QList<TrackInfo> DecoderFFmpegFactory::createPlayList(const QString &path, TrackInfo::Parts parts, QStringList *)
{
    int trackNumber = -1; //cue/m4b track
    QString filePath = path;

    if(path.contains(u"://"_s)) //is it cue track?
    {
        filePath = TrackInfo::pathFromUrl(path, &trackNumber);
        parts = TrackInfo::AllParts; //extract all metadata for single cue/m4b track
    }

    TrackInfo info(filePath);

    if(parts == TrackInfo::Parts())
        return { info };

    AVFormatContext *in = nullptr;

#ifdef Q_OS_WIN
    if(avformat_open_input(&in, filePath.toUtf8().constData(), nullptr, nullptr) < 0)
#else
    if(avformat_open_input(&in, filePath.toLocal8Bit().constData(), nullptr, nullptr) < 0)
#endif
    {
        qCDebug(plugin, "unable to open file");
        return  QList<TrackInfo>();
    }

    avformat_find_stream_info(in, nullptr);

    if(parts & TrackInfo::Properties)
    {
        int idx = av_find_best_stream(in, AVMEDIA_TYPE_AUDIO, -1, -1, nullptr, 0);
        if(idx >= 0)
        {
            AVCodecParameters *c = in->streams[idx]->codecpar;
            info.setValue(Qmmp::BITRATE, int(c->bit_rate) / 1000);
            info.setValue(Qmmp::SAMPLERATE, c->sample_rate);
#if (LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(59,37,100)) //ffmpeg-5.1
            info.setValue(Qmmp::CHANNELS, c->ch_layout.nb_channels);
#else
            info.setValue(Qmmp::CHANNELS, c->channels);
#endif
            if(c->bits_per_raw_sample > 0)
                info.setValue(Qmmp::BITS_PER_SAMPLE, c->bits_per_raw_sample);
            else
                info.setValue(Qmmp::BITS_PER_SAMPLE, av_get_bytes_per_sample(static_cast<AVSampleFormat>(c->format)) * 8);

            const AVCodec *codec = avcodec_find_decoder(c->codec_id);
            if(codec)
                info.setValue(Qmmp::FORMAT_NAME, QString::fromLatin1(codec->name));
            info.setValue(Qmmp::FILE_SIZE, QFileInfo(filePath).size()); //adds file size for cue tracks
            info.setDuration(in->duration * 1000 / AV_TIME_BASE);
        }
    }

    if(parts & TrackInfo::MetaData)
    {
        AVDictionaryEntry *cuesheet = av_dict_get(in->metadata,"cuesheet",nullptr,0);
        if(cuesheet)
        {
            CueParser parser(cuesheet->value);
            parser.setDuration(info.duration());
            parser.setProperties(info.properties());
            parser.setUrl(u"ffmpeg"_s, filePath);

            avformat_close_input(&in);
            return parser.createPlayList(trackNumber);
        }

        if(trackNumber > 0 && path.startsWith(u"ffmpeg://"_s)) //invalid track
        {
            avformat_close_input(&in);
            return QList<TrackInfo>();
        }

        AVDictionaryEntry *album = av_dict_get(in->metadata,"album",nullptr,0);
        AVDictionaryEntry *album_artist = av_dict_get(in->metadata,"album_artist",nullptr,0);
        AVDictionaryEntry *artist = av_dict_get(in->metadata,"artist",nullptr,0);
        AVDictionaryEntry *composer = av_dict_get(in->metadata,"composer",nullptr,0);
        AVDictionaryEntry *comment = av_dict_get(in->metadata,"comment",nullptr,0);
        AVDictionaryEntry *genre = av_dict_get(in->metadata,"genre",nullptr,0);
        AVDictionaryEntry *title = av_dict_get(in->metadata,"title",nullptr,0);
        AVDictionaryEntry *year = av_dict_get(in->metadata,"date",nullptr,0);
        AVDictionaryEntry *track = av_dict_get(in->metadata,"track",nullptr,0);

        if(!album)
            album = av_dict_get(in->metadata,"WM/AlbumTitle",nullptr,0);

        if(!artist)
            artist = av_dict_get(in->metadata,"author",nullptr,0);

        if(!year)
            year = av_dict_get(in->metadata,"WM/Year",nullptr,0);
        if(!year)
            year = av_dict_get(in->metadata,"year",nullptr,0);

        if(!track)
            track = av_dict_get(in->metadata,"WM/Track",nullptr,0);
        if(!track)
            track = av_dict_get(in->metadata,"WM/TrackNumber",nullptr,0);

        if(album)
            info.setValue(Qmmp::ALBUM, QString::fromUtf8(album->value).trimmed());
        if(album_artist)
            info.setValue(Qmmp::ALBUMARTIST, QString::fromUtf8(album_artist->value).trimmed());
        if(artist)
            info.setValue(Qmmp::ARTIST, QString::fromUtf8(artist->value).trimmed());
        if(composer)
            info.setValue(Qmmp::COMPOSER, QString::fromUtf8(composer->value).trimmed());
        if(comment)
            info.setValue(Qmmp::COMMENT, QString::fromUtf8(comment->value).trimmed());
        if(genre)
            info.setValue(Qmmp::GENRE, QString::fromUtf8(genre->value).trimmed());
        if(title)
            info.setValue(Qmmp::TITLE, QString::fromUtf8(title->value).trimmed());
        if(year)
            info.setValue(Qmmp::YEAR, year->value);
        if(track)
            info.setValue(Qmmp::TRACK, track->value);

        if(in->nb_chapters > 1 && filePath.endsWith(u".m4b"_s, Qt::CaseInsensitive))
        {
            QList<TrackInfo> tracks = createPlayListFromChapters(in, &info, trackNumber);
            avformat_close_input(&in);
            return tracks;
        }

        if(trackNumber > 0 && path.startsWith(u"m4b://"_s)) //invalid chapter
        {
            avformat_close_input(&in);
            return QList<TrackInfo>();
        }
    }

    avformat_close_input(&in);
    return { info };
}

MetaDataModel* DecoderFFmpegFactory::createMetaDataModel(const QString &path, bool readOnly)
{
    return new FFmpegMetaDataModel(path, readOnly);
}

QDialog *DecoderFFmpegFactory::createSettings(QWidget *parent)
{
    return new FFmpegSettingsDialog(parent);
}

void DecoderFFmpegFactory::showAbout(QWidget *parent)
{
    QMessageBox::about(parent, tr("About FFmpeg Audio Plugin"),
                       tr("Qmmp FFmpeg Audio Plugin") + QChar::LineFeed +
                       tr("Compiled against:") + QChar::LineFeed +
                       QStringLiteral("libavformat-%1.%2.%3\n"
                                      "libavcodec-%4.%5.%6\n"
                                      "libavutil-%7.%8.%9\n"
                                      "TagLib-%10.%11.%12")
                       .arg(LIBAVFORMAT_VERSION_MAJOR)
                       .arg(LIBAVFORMAT_VERSION_MINOR)
                       .arg(LIBAVFORMAT_VERSION_MICRO)
                       .arg(LIBAVCODEC_VERSION_MAJOR)
                       .arg(LIBAVCODEC_VERSION_MINOR)
                       .arg(LIBAVCODEC_VERSION_MICRO)
                       .arg(LIBAVUTIL_VERSION_MAJOR)
                       .arg(LIBAVUTIL_VERSION_MINOR)
                       .arg(LIBAVUTIL_VERSION_MICRO)
                       .arg(TAGLIB_MAJOR_VERSION)
                       .arg(TAGLIB_MINOR_VERSION)
                       .arg(TAGLIB_PATCH_VERSION) + QChar::LineFeed +
                       tr("Written by: Ilya Kotov <forkotov02@ya.ru>"));
}

QString DecoderFFmpegFactory::translation() const
{
    return QLatin1String(":/ffmpeg_plugin_");
}

QList<TrackInfo> DecoderFFmpegFactory::createPlayListFromChapters(AVFormatContext *in, TrackInfo *extraInfo,
                                                                    int trackNumber)
{
    QList<TrackInfo> tracks;

    for(unsigned int i = 0; i < in->nb_chapters; ++i)
    {
        if((trackNumber > 0) && (int(i + 1) != trackNumber))
            continue;

        AVChapter *chapter = in->chapters[i];
        TrackInfo info(QStringLiteral("m4b://%1#%2").arg(extraInfo->path()).arg(i + 1));
        info.setDuration((chapter->end - chapter->start) * av_q2d(chapter->time_base) * 1000);
        info.setValues(extraInfo->properties());
        info.setValues(extraInfo->metaData());
        info.setValue(Qmmp::TRACK, i + 1);

        AVDictionaryEntry *title = av_dict_get(chapter->metadata,"title", nullptr, 0);
        if(title)
            info.setValue(Qmmp::TITLE, QString::fromUtf8(title->value).trimmed());

        tracks << info;
    }

    return tracks;
}
