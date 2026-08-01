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
#include "wildmidihelper.h"
#include "decoder_wildmidi.h"
#include "wildmidisettingsdialog.h"
#include "decoderwildmidifactory.h"

// DecoderWildMidiFactory

bool DecoderWildMidiFactory::canDecode(QIODevice *input) const
{
    char buf[4];
    if(input->peek(buf, 4) != 4)
        return false;

    return !memcmp(buf, "MThd", 4) || !memcmp(buf, "MUS", 3) || !memcmp(buf, "FORM", 4);
}

DecoderProperties DecoderWildMidiFactory::properties() const
{
    DecoderProperties properties;
    properties.name = tr("WildMidi Plugin");
    properties.filters = QStringList { u"*.mid"_s, u"*.mus"_s, u"*.xmi"_s };
    properties.description = tr("Midi Files");
    //properties.contentType = ;
    properties.shortName = "wildmidi"_L1;
    properties.hasAbout = true;
    properties.hasSettings = true;
    properties.noInput = false;
    properties.protocols = QStringList { u"file"_s };
    return properties;
}

Decoder *DecoderWildMidiFactory::create(const QString &path, QIODevice *input)
{
    Q_UNUSED(path);
    return new DecoderWildMidi(input);
}

QList<TrackInfo> DecoderWildMidiFactory::createPlayList(const QString &path, TrackInfo::Parts parts, QStringList *)
{
    TrackInfo info(path);
    WildMidiHelper *helper = WildMidiHelper::instance();

    if((parts & TrackInfo::Properties) && helper->initialize() && helper->sampleRate())
    {
        void *midi_ptr = WildMidi_Open (path.toLocal8Bit().constData());
        if(midi_ptr)
        {
            WildMidiHelper::instance()->addPtr(midi_ptr);
            _WM_Info *wm_info = WildMidi_GetInfo(midi_ptr);
            info.setValue(Qmmp::SAMPLERATE, helper->sampleRate());
            info.setValue(Qmmp::FORMAT_NAME, u"midi"_s);
            info.setDuration((qint64)wm_info->approx_total_samples * 1000 / helper->sampleRate());
            WildMidi_Close(midi_ptr);
            WildMidiHelper::instance()->removePtr(midi_ptr);
        }
    }
    return { info };
}

MetaDataModel* DecoderWildMidiFactory::createMetaDataModel(const QString &path, bool readOnly)
{
    Q_UNUSED(path);
    Q_UNUSED(readOnly);
    return nullptr;
}

QDialog *DecoderWildMidiFactory::createSettings(QWidget *parent)
{
    return new WildMidiSettingsDialog(parent);
}

void DecoderWildMidiFactory::showAbout(QWidget *parent)
{
    QMessageBox::about(parent, tr("About WildMidi Audio Plugin"),
                       tr("x-AMP WildMidi Audio Plugin") + QChar::LineFeed +
                       tr("This plugin uses WildMidi library to play midi files") + QChar::LineFeed +
                       tr("Compiled against libWildMidi-%1.%2.%3")
                               .arg(LIBWILDMIDI_VER_MAJOR)
                               .arg(LIBWILDMIDI_VER_MINOR)
                               .arg(LIBWILDMIDI_VER_MICRO) + QChar::LineFeed +
                       tr("Written by: Ilya Kotov <forkotov02@ya.ru>"));
}

QString DecoderWildMidiFactory::translation() const
{
    return QLatin1String(":/wildmidi_plugin_");
}
