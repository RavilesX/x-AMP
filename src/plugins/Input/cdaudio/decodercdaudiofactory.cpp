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

#include <QMessageBox>
#include <QSettings>
#include <cdio/version.h>
#ifdef WITH_LIBCDDB
#include <cddb/version.h>
#endif
#include "cdaudiosettingsdialog.h"
#include "decoder_cdaudio.h"
#include "decodercdaudiofactory.h"


// DecoderCDAudioFactory

DecoderCDAudioFactory::DecoderCDAudioFactory()
{
    QSettings settings;
    if(settings.value("cdaudio/cddb_server"_L1).toByteArray() == "freedb.org")
    {
        qCDebug(plugin, "switching to gnudb.org");
        settings.setValue(u"cdaudio/cddb_server"_s, u"gnudb.org"_s);
    }
}

bool DecoderCDAudioFactory::canDecode(QIODevice *) const
{
    return false;
}

DecoderProperties DecoderCDAudioFactory::properties() const
{
    DecoderProperties properties;
    properties.name = tr("CD Audio Plugin");
    properties.shortName = "cdaudio"_L1;
    properties.protocols = QStringList { u"cdda"_s };
    properties.hasAbout = true;
    properties.noInput = true;
    properties.hasSettings = true;
    return properties;
}

Decoder *DecoderCDAudioFactory::create(const QString &url, QIODevice *input)
{
    Q_UNUSED(input);
    return new DecoderCDAudio(url);
}

QList<TrackInfo> DecoderCDAudioFactory::createPlayList(const QString &path, TrackInfo::Parts parts, QStringList *)
{
    QList<TrackInfo> list;

    if(path.contains(QLatin1Char('#')))
        return list;

    QString device_path = path;
    device_path.remove(u"cdda://"_s);
    QList<CDATrack> tracks = DecoderCDAudio::generateTrackList(device_path, parts);
    for(const CDATrack &t : std::as_const(tracks))
    {
        list << TrackInfo(t.info);
    }
    return list;
}

MetaDataModel* DecoderCDAudioFactory::createMetaDataModel(const QString &path, bool readOnly)
{
    Q_UNUSED(readOnly);
    Q_UNUSED(path);
    return nullptr;
}

QDialog *DecoderCDAudioFactory::createSettings(QWidget *parent)
{
    return new CDAudioSettingsDialog(parent);
}

void DecoderCDAudioFactory::showAbout(QWidget *parent)
{
    QMessageBox::about(parent, tr("About CD Audio Plugin"),
                       tr("Qmmp CD Audio Plugin") + QChar::LineFeed +
#ifdef WITH_LIBCDDB
                       tr("Compiled against libcdio-%1 and libcddb-%2").arg(QLatin1StringView(CDIO_VERSION),
                                                                            QLatin1StringView(CDDB_VERSION)) +
#else
                       tr("Compiled against libcdio-%1").arg(QLatin1StringView(CDIO_VERSION)) +
#endif
                       QChar::LineFeed +
                       tr("Written by: Ilya Kotov <forkotov02@ya.ru>") + QChar::LineFeed +
                       tr("Usage: open cdda:/// using Add URL dialog or command line"));
}

QString DecoderCDAudioFactory::translation() const
{
    return QLatin1String(":/cdaudio_plugin_");
}
