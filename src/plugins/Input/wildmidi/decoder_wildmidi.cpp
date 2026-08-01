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

#include <stdint.h>
#include "wildmidihelper.h"
#include "decoder_wildmidi.h"

// Decoder class
DecoderWildMidi::DecoderWildMidi(QIODevice *input) : Decoder(input)
{}

DecoderWildMidi::~DecoderWildMidi()
{
    if(m_midiPtr)
    {
        WildMidiHelper::instance()->removePtr(m_midiPtr);
        WildMidi_Close(m_midiPtr);
    }
}

bool DecoderWildMidi::initialize()
{
    m_totalTime = 0;

    if(!WildMidiHelper::instance()->initialize())
    {
        qCWarning(plugin, "initialization failed");
        return false;
    }

    m_buffer = input()->readAll();
    if(m_buffer.isEmpty())
    {
        qCWarning(plugin, "unable to read file");
        return false;
    }

    m_midiPtr = WildMidi_OpenBuffer(reinterpret_cast<const uint8_t *>(m_buffer.constData()), m_buffer.size());

    if(!m_midiPtr)
    {
        qCWarning(plugin, "unable to open file");
        return false;
    }
    WildMidiHelper::instance()->addPtr(m_midiPtr);

    m_sampleRate = WildMidiHelper::instance()->sampleRate();
    _WM_Info *wm_info = WildMidi_GetInfo(m_midiPtr);
    m_totalTime = (qint64)wm_info->approx_total_samples * 1000 / WildMidiHelper::instance()->sampleRate();
    configure(m_sampleRate, 2, Qmmp::PCM_S16LE);
    qCDebug(plugin, "initialize succes");
    return true;
}

qint64 DecoderWildMidi::totalTime() const
{
    return m_totalTime;
}

void DecoderWildMidi::seek(qint64 pos)
{
    ulong sample = (ulong)m_sampleRate * pos / 1000;
    WildMidi_FastSeek(m_midiPtr, &sample);
}

int DecoderWildMidi::bitrate() const
{
    return 8;
}

qint64 DecoderWildMidi::read(unsigned char *data, qint64 size)
{
    return WildMidi_GetOutput(m_midiPtr, reinterpret_cast<int8_t *>(data), size);
}
