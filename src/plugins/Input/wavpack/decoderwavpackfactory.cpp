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

#include <QMessageBox>
#include <QFileInfo>
#include <qmmp/cueparser.h>
#include "wavpackmetadatamodel.h"
#include "decoder_wavpack.h"
#include "decoderwavpackfactory.h"

// DecoderWavPackFactory
bool DecoderWavPackFactory::canDecode(QIODevice *input) const
{
    char buf[4];
    return (input->peek(buf, 4) == 4 && !memcmp(buf, "wvpk", 4));
}

DecoderProperties DecoderWavPackFactory::properties() const
{
    DecoderProperties properties;
    properties.name = tr("WavPack Plugin");
    properties.filters = QStringList { u"*.wv"_s };
    properties.description = tr("WavPack Files");
    properties.contentTypes = QStringList { u"audio/x-wavpack"_s };
    properties.shortName = u"wavpack"_s;
    properties.hasAbout = true;
    properties.hasSettings = false;
    properties.noInput = false;
    properties.protocols = QStringList { u"file"_s, u"wvpack"_s };
    return properties;
}

Decoder *DecoderWavPackFactory::create(const QString &p, QIODevice *i)
{
    return new DecoderWavPack(p, i);
}

QList<TrackInfo> DecoderWavPackFactory::createPlayList(const QString &path, TrackInfo::Parts parts, QStringList *ignoredFiles)
{
    Q_UNUSED(ignoredFiles);

    int track = -1; //cue track
    QString filePath = path;

    if(path.contains(u"://"_s)) //is it cue track?
    {
        filePath = TrackInfo::pathFromUrl(path, &track);
        parts = TrackInfo::AllParts; //extract all metadata for single cue track
    }

    TrackInfo info(filePath);

    if(parts == TrackInfo::Parts())
        return { info };

    char err[80] = { 0 };

#if defined(Q_OS_WIN) && defined(OPEN_FILE_UTF8)
    WavpackContext *ctx = WavpackOpenFileInput (filePath.toUtf8().constData(),
                                                err, OPEN_WVC | OPEN_TAGS | OPEN_FILE_UTF8, 0);
#else
    WavpackContext *ctx = WavpackOpenFileInput (filePath.toLocal8Bit().constData(),
                                                err, OPEN_WVC | OPEN_TAGS, 0);
#endif
    if (!ctx)
    {
        qCWarning(plugin, "error: %s", err);
        return QList<TrackInfo>();
    }

    if(parts & TrackInfo::Properties)
    {
        info.setValue(Qmmp::BITRATE, int(WavpackGetAverageBitrate(ctx, 1) / 1000));
        info.setValue(Qmmp::SAMPLERATE, WavpackGetSampleRate(ctx));
        info.setValue(Qmmp::CHANNELS, WavpackGetNumChannels(ctx));
        info.setValue(Qmmp::BITS_PER_SAMPLE, WavpackGetBitsPerSample(ctx));
        info.setValue(Qmmp::FORMAT_NAME, u"WavPack"_s);
        info.setValue(Qmmp::FILE_SIZE, QFileInfo(filePath).size()); //adds file size for cue tracks
        info.setDuration((qint64)WavpackGetNumSamples(ctx) * 1000 / WavpackGetSampleRate(ctx));
    }

    if(parts & TrackInfo::ReplayGainInfo)
    {
        char value[200] = { 0 };
        WavpackGetTagItem(ctx, "REPLAYGAIN_TRACK_GAIN", value, sizeof(value));
        info.setValue(Qmmp::REPLAYGAIN_TRACK_GAIN, QString::fromLatin1(value));
        WavpackGetTagItem(ctx, "REPLAYGAIN_TRACK_PEAK", value, sizeof(value));
        info.setValue(Qmmp::REPLAYGAIN_TRACK_PEAK, QString::fromLatin1(value));
        WavpackGetTagItem(ctx, "REPLAYGAIN_ALBUM_GAIN", value, sizeof(value));
        info.setValue(Qmmp::REPLAYGAIN_ALBUM_GAIN, QString::fromLatin1(value));
        WavpackGetTagItem(ctx, "REPLAYGAIN_ALBUM_PEAK", value, sizeof(value));
        info.setValue(Qmmp::REPLAYGAIN_ALBUM_PEAK, QString::fromLatin1(value));
    }

    if(parts & TrackInfo::MetaData)
    {
        int cue_len = WavpackGetTagItem(ctx, "cuesheet", nullptr, 0);
        if(cue_len > 0)
        {
            QByteArray value(cue_len, '\0');
            WavpackGetTagItem(ctx, "cuesheet", value.data(), cue_len);

            CueParser parser(value);
            parser.setDuration(info.duration());
            parser.setProperties(info.properties());
            parser.setUrl(u"wvpack"_s, filePath);

            WavpackCloseFile(ctx);
            return parser.createPlayList(track);
        }

        if(track > 0)
        {
            WavpackCloseFile(ctx);
            return QList<TrackInfo>();
        }

        char value[200] = { 0 };
        WavpackGetTagItem(ctx, "Album", value, sizeof(value));
        info.setValue(Qmmp::ALBUM, QString::fromUtf8(value));
        WavpackGetTagItem(ctx, "Artist", value, sizeof(value));
        info.setValue(Qmmp::ARTIST, QString::fromUtf8(value));
        WavpackGetTagItem(ctx, "Album Artist", value, sizeof(value));
        info.setValue(Qmmp::ALBUMARTIST, QString::fromUtf8(value));
        WavpackGetTagItem(ctx, "Comment", value, sizeof(value));
        info.setValue(Qmmp::COMMENT, QString::fromUtf8(value));
        WavpackGetTagItem(ctx, "Genre", value, sizeof(value));
        info.setValue(Qmmp::GENRE, QString::fromUtf8(value));
        WavpackGetTagItem(ctx, "Title", value, sizeof(value));
        info.setValue(Qmmp::TITLE, QString::fromUtf8(value));
        WavpackGetTagItem(ctx, "Composer", value, sizeof(value));
        info.setValue(Qmmp::COMPOSER, QString::fromUtf8(value));
        WavpackGetTagItem(ctx, "Year", value, sizeof(value));
        info.setValue(Qmmp::YEAR, QString::fromUtf8(value).toInt());
        WavpackGetTagItem(ctx, "Track", value, sizeof(value));
        info.setValue(Qmmp::TRACK, QString::fromUtf8(value).toInt());
        WavpackGetTagItem(ctx, "Disc", value, sizeof(value));
        info.setValue(Qmmp::DISCNUMBER, QString::fromUtf8(value).toInt());
    }

    WavpackCloseFile (ctx);
    return { info };
}

MetaDataModel* DecoderWavPackFactory::createMetaDataModel(const QString &path, bool readOnly)
{
    if(!path.contains(u"://"_s) || path.startsWith(u"wvpack://"_s))
        return new WavPackMetaDataModel(path, readOnly);

    return nullptr;
}

QDialog *DecoderWavPackFactory::createSettings(QWidget *)
{
    return nullptr;
}

void DecoderWavPackFactory::showAbout(QWidget *parent)
{
    QMessageBox::about(parent, tr("About WavPack Audio Plugin"),
                       tr("x-AMP WavPack Audio Plugin") + QChar::LineFeed +
                       tr("WavPack library version: %1").arg(QString::fromLatin1(WavpackGetLibraryVersionString())) +
                       QChar::LineFeed +
                       tr("Written by: Ilya Kotov <forkotov02@ya.ru>"));
}

QString DecoderWavPackFactory::translation() const
{
    return QLatin1String(":/wavpack_plugin_");
}
