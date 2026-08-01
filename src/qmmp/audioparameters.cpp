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

#include <QHash>
#include "audioparameters.h"

class AudioParametersPrivate
{
public:
    quint32 srate = 0;
    ChannelMap chanMap;
    Qmmp::AudioFormat format = Qmmp::PCM_S16LE;
    int sampleSize = 2;
    int validBitsPerSample = 16;
};

AudioParameters::AudioParameters() :
    d_ptr(new AudioParametersPrivate)
{}

AudioParameters::AudioParameters(const AudioParameters &other) :
    d_ptr(new AudioParametersPrivate)
{
    operator=(other);
}

AudioParameters::AudioParameters(AudioParameters &&other) noexcept :
    d_ptr(std::exchange(other.d_ptr, nullptr))
{}

AudioParameters::AudioParameters(quint32 srate, const ChannelMap &map, Qmmp::AudioFormat format) :
    d_ptr(new AudioParametersPrivate)
{
    Q_D(AudioParameters);
    d->srate = srate;
    d->chanMap = map;
    d->format = format;
    d->sampleSize = sampleSize(format);
    d->validBitsPerSample = validBitsPerSample(format);
}

AudioParameters::~AudioParameters()
{
    delete d_ptr;
}

AudioParameters &AudioParameters::operator=(const AudioParameters &p)
{
    Q_D(AudioParameters);
    d->srate = p.sampleRate();
    d->chanMap = p.channelMap();
    d->format = p.format();
    d->sampleSize = p.sampleSize();
    d->validBitsPerSample = p.validBitsPerSample();
    return *this;
}

AudioParameters &AudioParameters::operator=(AudioParameters &&p) noexcept
{
    std::swap(d_ptr, p.d_ptr);
    return *this;
}

bool AudioParameters::operator==(const AudioParameters &p) const
{
    Q_D(const AudioParameters);
    return d->srate == p.sampleRate() && d->chanMap == p.channelMap() && d->format == p.format()
            && d->validBitsPerSample == p.validBitsPerSample();
}

bool AudioParameters::operator!=(const AudioParameters &p) const
{
    return !operator==(p);
}

quint32 AudioParameters::sampleRate() const
{
    return d_ptr->srate;
}

int AudioParameters::channels() const
{
    return d_ptr->chanMap.count();
}

const ChannelMap AudioParameters::channelMap() const
{
    return d_ptr->chanMap;
}

Qmmp::AudioFormat AudioParameters::format() const
{
    return d_ptr->format;
}

int AudioParameters::sampleSize() const
{
    return d_ptr->sampleSize;
}

int AudioParameters::frameSize() const
{
    return d_ptr->sampleSize * d_ptr->chanMap.count();
}

int AudioParameters::bitsPerSample() const
{
    return d_ptr->sampleSize * 8;
}

int AudioParameters::validBitsPerSample() const
{
    return d_ptr->validBitsPerSample;
}

AudioParameters::ByteOrder AudioParameters::byteOrder() const
{
    switch(d_ptr->format)
    {
    case Qmmp::PCM_S16BE:
    case Qmmp::PCM_S24BE:
    case Qmmp::PCM_S32BE:
        return BigEndian;
    default:
        return LittleEndian;
    }
}

QString AudioParameters::toString() const
{
    Q_D(const AudioParameters);
    static const QHash<Qmmp::AudioFormat, QString> names = {
        { Qmmp::PCM_S8, u"s8"_s },
        { Qmmp::PCM_U8, u"u8"_s },
        { Qmmp::PCM_S16LE, u"s16le"_s },
        { Qmmp::PCM_S16BE, u"s16be"_s },
        { Qmmp::PCM_U16LE, u"u16le"_s },
        { Qmmp::PCM_U16BE, u"u16be"_s },
        { Qmmp::PCM_S24LE, u"s24le"_s },
        { Qmmp::PCM_S24BE, u"s24be"_s },
        { Qmmp::PCM_U24LE, u"u24le"_s },
        { Qmmp::PCM_U24BE, u"u24be"_s },
        { Qmmp::PCM_S32LE, u"s32le"_s },
        { Qmmp::PCM_S32BE, u"s32be"_s },
        { Qmmp::PCM_U32LE, u"u32le"_s },
        { Qmmp::PCM_U32BE, u"u32be"_s },
        { Qmmp::PCM_FLOAT, u"float"_s }
    };

    return QStringLiteral("%1 Hz, {%2}, %3").arg(d->srate).arg(d->chanMap.toString(), names.value(d->format, u"unknown"_s));
}

int AudioParameters::sampleSize(Qmmp::AudioFormat format)
{
    switch(format)
    {
    case Qmmp::PCM_S8:
    case Qmmp::PCM_U8:
        return 1;
    case Qmmp::PCM_S16LE:
    case Qmmp::PCM_S16BE:
    case Qmmp::PCM_U16LE:
    case Qmmp::PCM_U16BE:
        return 2;
    case Qmmp::PCM_S24LE:
    case Qmmp::PCM_S24BE:
    case Qmmp::PCM_U24LE:
    case Qmmp::PCM_U24BE:
    case Qmmp::PCM_S32LE:
    case Qmmp::PCM_S32BE:
    case Qmmp::PCM_U32LE:
    case Qmmp::PCM_U32BE:
    case Qmmp::PCM_FLOAT:
        return 4;
    default:
        return 0;
    }
}

int AudioParameters::bitsPerSample(Qmmp::AudioFormat format)
{
    return sampleSize(format) * 8;
}

int AudioParameters::validBitsPerSample(Qmmp::AudioFormat format)
{
    switch(format)
    {
    case Qmmp::PCM_S8:
    case Qmmp::PCM_U8:
        return 8;
    case Qmmp::PCM_S16LE:
    case Qmmp::PCM_S16BE:
    case Qmmp::PCM_U16LE:
    case Qmmp::PCM_U16BE:
        return 16;
    case Qmmp::PCM_S24LE:
    case Qmmp::PCM_S24BE:
    case Qmmp::PCM_U24LE:
    case Qmmp::PCM_U24BE:
        return 24;
    case Qmmp::PCM_S32LE:
    case Qmmp::PCM_S32BE:
    case Qmmp::PCM_U32LE:
    case Qmmp::PCM_U32BE:
    case Qmmp::PCM_FLOAT:
        return 32;
    default:
        return 0;
    }
}

Qmmp::AudioFormat AudioParameters::findAudioFormat(int bits, ByteOrder byteOrder)
{
    switch (bits)
    {
    case 8:
        return Qmmp::PCM_U8;
    case 16:
        return (byteOrder == LittleEndian) ? Qmmp::PCM_U16LE : Qmmp::PCM_U16BE;
    case 24:
        return (byteOrder == LittleEndian) ? Qmmp::PCM_U24LE : Qmmp::PCM_U24BE;
    case 32:
        return (byteOrder == LittleEndian) ? Qmmp::PCM_U32LE : Qmmp::PCM_U32BE;
    default:
        return Qmmp::PCM_UNKNOWN;
    }
}
