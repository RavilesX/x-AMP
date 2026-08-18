/***************************************************************************
 *   Copyright (C) 2009-2026 by Ilya Kotov <forkotov02@ya.ru>              *
 *   Copyright (C) 2009 by Sebastian Pipping <sebastian@pipping.org>       *
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
#include <qmmp/qmmp.h>
#include <qmmp/statehandler.h>
#include <qmmp/soundcore.h>
#include "crossfadeplugin.h"

CrossfadePlugin::CrossfadePlugin() : Effect()
{
    m_core = SoundCore::instance() ;
    m_handler = StateHandler::instance();
    QSettings settings;
    m_overlap = settings.value(u"Crossfade/overlap"_s, 6000).toLongLong();
}

CrossfadePlugin::~CrossfadePlugin()
{
    //the loud one: if this fires while a tail is held, the engine threw the
    //effect away mid-crossfade and the buffered audio is lost
    if(m_buffer_at > 0)
        qCWarning(plugin, "crossfade: DESTROYED holding %zu buffered samples -- fade lost",
                  m_buffer_at);
    if(m_buffer)
        free(m_buffer);
}

void CrossfadePlugin::applyEffect(Buffer *b)
{
    switch (m_state)
    {
    case WAITING:
        if((m_core->duration() > m_overlap + 2000)
                && (m_core->duration() - m_handler->elapsed() < m_overlap + 2000))
        {
            StateHandler::instance()->sendNextTrackRequest();
            m_state = CHECKING;
            qCDebug(plugin, "crossfade: WAITING -> CHECKING (duration %lld, elapsed %lld)",
                    m_core->duration(), m_handler->elapsed());
        }
        break;
    case CHECKING:
        //next source has been received and current engine will be used to play it
        if(SoundCore::instance()->nextTrackAccepted())
        {
            //x-AMP: A track in another format makes the engine rebuild the
            //effect chain, and this object is destroyed with it -- taking the
            //tail it had held back. That was worse than not fading at all:
            //the current track went silent seconds early and the next one
            //still began at full volume. Stand aside instead, so the tail
            //plays where it belongs and only the fade is lost. Resampling it
            //is the only way to fade across a rate change, and that does not
            //belong in here.
            const AudioParameters next = m_core->nextAudioParameters();
            if(next.sampleRate() && (next.sampleRate() != sampleRate()
                                     || next.channelMap() != channelMap()))
            {
                qCDebug(plugin, "crossfade: next track is %u Hz / %d channels against "
                                "%u Hz / %d -- no fade across the change",
                        next.sampleRate(), int(next.channelMap().count()),
                        sampleRate(), channels());
                m_state = SKIPPING;
                break;
            }
            m_state = PREPARING;
            qCDebug(plugin, "crossfade: CHECKING -> PREPARING");
        }
        break;
    case PREPARING:
        if(m_core->duration() && (m_core->duration() - m_handler->elapsed() <  m_overlap))
        {
            if(m_buffer_at + b->samples > m_buffer_size)
            {
                m_buffer_size = m_buffer_at + b->samples;
                float *tmp = m_buffer;
                m_buffer = (float *)realloc(m_buffer, m_buffer_size * sizeof(float));
                if(!m_buffer)
                {
                    qCWarning(plugin, "unable to allocate  %zu bytes", m_buffer_size);
                    m_buffer_size = 0;
                    if(tmp)
                        free(tmp);
                }
            }

            if(m_buffer)
            {
                memcpy(m_buffer + m_buffer_at, b->data, b->samples * sizeof(float));
                m_buffer_at += b->samples;
                b->samples = 0;
            }
        }
        else if(m_buffer_at > 0)
        {
            m_state = PROCESSING;
            qCDebug(plugin, "crossfade: PREPARING -> PROCESSING (%zu samples buffered, "
                            "new duration %lld, elapsed %lld)",
                    m_buffer_at, m_core->duration(), m_handler->elapsed());
        }
        break;
    case PROCESSING:
        if (m_buffer_at > 0)
        {
            double volume = (double)m_buffer_at/m_buffer_size;
            size_t samples = qMin(m_buffer_at, b->samples);
            mix(b->data, m_buffer, samples, volume);
            m_buffer_at -= samples;
            memmove(m_buffer, m_buffer + samples, m_buffer_at * sizeof(float));
        }
        else
        {
            m_state = WAITING;
            qCDebug(plugin, "crossfade: PROCESSING -> WAITING (fade finished)");
        }
        break;
    case SKIPPING:
        //nothing to do: the buffer stays empty and the audio passes through
        //untouched until the engine replaces this object
        break;
    default:
        ;
    }
    return;
}

void CrossfadePlugin::configure(quint32 freq, ChannelMap map)
{
    qCDebug(plugin, "crossfade: configure(%u Hz, %d channels)", freq, int(map.count()));
    Effect::configure(freq, map);
}

void CrossfadePlugin::mix(float *cur_buf, float *prev_buf, uint samples, double volume)
{
    for (uint i = 0; i < samples; i++)
    {
        cur_buf[i] = cur_buf[i] * (1.0 - volume) + prev_buf[i]*volume;
        cur_buf[i] = qBound(-1.0f, cur_buf[i], 1.0f);
    }
}
