/***************************************************************************
 *   Copyright (C) 2016-2026 by Ilya Kotov                                 *
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

#ifndef OUTPUTWASAPI_H
#define OUTPUTWASAPI_H

#include <QPair>
#include <stdio.h>
#include <windows.h>
#include <mmdeviceapi.h>
#include <Audioclient.h>
#include <qmmp/volume.h>
#include <qmmp/output.h>

class VolumeWASAPI;

/**
    @author Ilya Kotov <forkotov02@ya.ru>
*/
class OutputWASAPI : public Output
{
public:
    OutputWASAPI();
    ~OutputWASAPI();

    bool initialize(quint32, ChannelMap map,  Qmmp::AudioFormat format) override;

    //output api
    qint64 latency() override;
    qint64 writeAudio(unsigned char *data, qint64 size) override;
    void drain() override;
    void suspend() override;
    void resume() override;
    void reset() override;

    //volume control
    ISimpleAudioVolume *simpleAudioVolume();
    static OutputWASAPI *instance;
    static VolumeWASAPI *volumeControl;


private:
    // helper functions
    void status();
    void uninitialize();

    IMMDeviceEnumerator *m_pEnumerator = nullptr;
    IMMDevice *m_pDevice = nullptr;
    IAudioClient *m_pAudioClient = nullptr;
    IAudioRenderClient *m_pRenderClient = nullptr;
    ISimpleAudioVolume *m_pSimpleAudioVolume = nullptr;

    UINT32 m_bufferFrames = 0;
    int m_frameSize = 0;
    QString m_id;
    bool m_exclusive = false;
    qint64 m_bufferSize = 1000000L; //microseconds
    static QList< QPair<Qmmp::ChannelPosition, DWORD> > m_wasapi_pos; //channel position, mask
};

/**
    @author Ilya Kotov <forkotov02@ya.ru>
*/
class VolumeWASAPI : public Volume
{
public:
    VolumeWASAPI();
    ~VolumeWASAPI();

    Volume::VolumeFlags flags() const override;
    void setVolume(const VolumeSettings &vol) override;
    VolumeSettings volume() const override;
    void setMuted(bool mute) override;
    bool isMuted() const override;
    void restore();

private:
    VolumeSettings m_volume;
    bool m_muted;
};


#endif // OUTPUTWASAPI_H
