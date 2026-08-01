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

#ifndef DECODER_WAVPACK_H
#define DECODER_WAVPACK_H

extern "C"{
#include <wavpack/wavpack.h>
}
#include <qmmp/decoder.h>

class CueParser;

class DecoderWavPack : public Decoder
{
public:
    explicit DecoderWavPack(const QString &path, QIODevice *i);
    virtual ~DecoderWavPack();

    // Standard Decoder API
    bool initialize() override;
    qint64 totalTime() const override;
    int bitrate() const override;
    qint64 read(unsigned char *data, qint64 maxSize) override;
    void seek(qint64 time) override;
    QString nextURL() const override;
    void next() override;

private:
    // helper functions
    void openCorrectionFile(const QString &path);
    void deinit();
    qint64 wavpack_decode(unsigned char *data, qint64 size);
    ChannelMap findChannelMap(int channels);
    // WavPack callbacks
    static int32_t wv_read_bytes(void *id, void *data, int32_t bcount);
    static int64_t wv_get_pos(void *id);
    static int wv_set_pos_abs(void *id, int64_t pos);
    static int wv_set_pos_rel(void *id, int64_t delta, int mode);
    static int wv_push_back_byte(void *id, int c);
    static int64_t wv_get_length(void *id);
    static int wv_can_seek(void *id);

    static WavpackStreamReader64 m_reader;
    WavpackContext *m_context = nullptr;
    int32_t *m_output_buf = nullptr; // output buffer
    int m_chan = 0;
    qint64 m_totalTime = 0;
    qint64 m_length_in_bytes = 0;
    qint64 m_totalBytes = 0;
    qint64 m_offset = 0;
    qint64 m_length = 0;
    QString m_path;
    CueParser *m_parser = nullptr;
    QIODevice *m_input = nullptr;
    QIODevice *m_wvc_input = nullptr;
    int m_track = 0;
    int m_bps = 0;
    qint64 m_frame_size = 0; //frame size
};

#endif // DECODER_WAVPACK_H
