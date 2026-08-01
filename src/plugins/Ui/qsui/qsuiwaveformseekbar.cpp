/***************************************************************************
 *   Copyright (C) 2020-2026 by Ilya Kotov                                 *
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

#include <QPainter>
#include <QPaintEvent>
#include <QSettings>
#include <QToolTip>
#include <QMenu>
#include <QAction>
#include <QMutexLocker>
#include <QStyleHints>
#include <cmath>
#include <numeric>
#include <qmmp/soundcore.h>
#include <qmmp/inputsource.h>
#include <qmmp/decoder.h>
#include <qmmp/decoderfactory.h>
#include <qmmp/audioconverter.h>
#include <qmmp/buffer.h>
#include <qmmp/qmmpsettings.h>
#include <qmmpui/metadataformatter.h>
#include "qsuicolorscheme.h"
#include "qsuiwaveformseekbar.h"

#define NUMBER_OF_VALUES (4096)
#define FLOATS_PER_LANE (4) // Number of floats fitting into a 128-bit XMM register
#define VECTORIZE_HINT (8)

QSUiWaveformSeekBar::QSUiWaveformSeekBar(QWidget *parent) : QWidget(parent)
{
    m_core = SoundCore::instance();
    connect(m_core, &SoundCore::stateChanged, this, &QSUiWaveformSeekBar::onStateChanged);
    connect(m_core, &SoundCore::elapsedChanged, this, &QSUiWaveformSeekBar::onElapsedChanged);
    createMenu();
    readSettings();
}

QSize QSUiWaveformSeekBar::sizeHint() const
{
    return QSize(200, 100);
}

void QSUiWaveformSeekBar::readSettings()
{
    QSettings settings;
    settings.beginGroup(u"Simple"_s);

    QString colorMode = settings.value(u"color_mode"_s, u"light"_s).toString();
    QSUiColorScheme scheme;
    if(colorMode == QLatin1String("light"))
        scheme.load(&settings, false);
    else if(colorMode == QLatin1String("dark"))
        scheme.load(&settings, true);
    else
#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
        scheme.load(&settings, qApp->styleHints()->colorScheme() == Qt::ColorScheme::Dark);
#else
        scheme.load(&settings, false);
#endif

    m_bgColor = scheme.color(QSUiColorScheme::WFSB_BACKGROUND);
    m_rmsColor = scheme.color(QSUiColorScheme::WFSB_RMS);
    m_waveFormColor = scheme.color(QSUiColorScheme::WFSB_WAVEFORM);
    m_progressBar = scheme.color(QSUiColorScheme::WFSB_PROGRESSBAR);

    if(!m_update)
    {
        m_update = true;
        m_showTwoChannelsAction->setChecked(settings.value(u"wfsb_show_two_channels"_s, true).toBool());
        m_showRmsAction->setChecked(settings.value(u"wfsb_show_rms"_s, true).toBool());
    }
    settings.endGroup();
    drawWaveform();
}


void QSUiWaveformSeekBar::onStateChanged(Qmmp::State state)
{
    switch (state)
    {
    case Qmmp::Playing:
    {
        if(!m_scanner && isVisible() && m_data.isEmpty())
        {
            m_scanner = new QSUiWaveformScanner(this);
            connect(m_scanner, &QSUiWaveformScanner::finished, this, &QSUiWaveformSeekBar::onScanFinished);
            connect(m_scanner, &QSUiWaveformScanner::dataChanged, this, &QSUiWaveformSeekBar::onDataChanged);
        }
        if(m_scanner)
            m_scanner->scan(m_core->path());
    }
        break;
    case Qmmp::Stopped:
    case Qmmp::FatalError:
    case Qmmp::NormalError:
    {
        if(m_scanner)
        {
            m_scanner->stop();
            delete m_scanner;
            m_scanner = nullptr;
        }
        m_data.clear();
        m_elapsed = 0;
        m_duration = 0;
        drawWaveform();
    }
        break;
    default:
        break;
    }
}

void QSUiWaveformSeekBar::onScanFinished()
{
    if(!m_scanner)
        return;

    m_data = m_scanner->data();
    m_channels = m_scanner->audioParameters().channels();
    delete m_scanner;
    m_scanner = nullptr;
    drawWaveform();
}

void QSUiWaveformSeekBar::onDataChanged()
{
    if(!m_scanner || !m_scanner->isRunning())
        return;

    m_data = m_scanner->data();
    m_channels = m_scanner->audioParameters().channels();
    drawWaveform();
}

void QSUiWaveformSeekBar::onElapsedChanged(qint64 elapsed)
{
    m_elapsed = elapsed;
    m_duration = m_core->duration();
    if(isVisible())
        update();
}

void QSUiWaveformSeekBar::writeSettings()
{
    QSettings settings;
    settings.beginGroup(u"Simple"_s);
    settings.setValue(u"wfsb_show_two_channels"_s, m_showTwoChannelsAction->isChecked());
    settings.setValue(u"wfsb_show_rms"_s, m_showRmsAction->isChecked());
    settings.endGroup();
    drawWaveform();
}

void QSUiWaveformSeekBar::paintEvent(QPaintEvent *e)
{
    QPainter painter(this);
    painter.fillRect(e->rect(), m_bgColor);

    if(!m_pixmap.isNull())
        painter.drawPixmap(0, 0, width(), height(), m_pixmap);

    if(m_duration > 0)
    {
        int x = m_pressedPos >= 0 ? m_pressedPos : (width() * m_elapsed / m_duration);
        QColor color = m_progressBar;
        QBrush brush(color);
        painter.fillRect(0, 0, x, height(), brush);
        color.setAlpha(255);
        painter.setPen(color);
        painter.drawLine(x, 0, x, height());
    }
}

void QSUiWaveformSeekBar::resizeEvent(QResizeEvent *)
{
    drawWaveform();
}

void QSUiWaveformSeekBar::showEvent(QShowEvent *)
{
    if(m_data.isEmpty() && (m_core->state() == Qmmp::Playing || m_core->state() == Qmmp::Paused))
        onStateChanged(Qmmp::Playing); //force redraw
}

void QSUiWaveformSeekBar::mousePressEvent(QMouseEvent *e)
{
    if(e->button() == Qt::LeftButton)
    {
        m_pressedPos = e->pos().x();
        update();
    }
    else if(e->button() == Qt::RightButton)
        m_menu->exec(e->globalPosition().toPoint());
}

void QSUiWaveformSeekBar::mouseReleaseEvent(QMouseEvent *)
{
    if(m_pressedPos >= 0)
    {
        if(m_duration > 0)
            m_core->seek(m_pressedPos * m_duration / width());
        m_pressedPos = -1;
    }
}

void QSUiWaveformSeekBar::mouseMoveEvent(QMouseEvent *e)
{
    if(m_pressedPos >= 0)
    {
        m_pressedPos = qBound(0, e->pos().x(), width());
        QToolTip::showText(mapToGlobal(e->pos()), MetaDataFormatter::formatDuration(m_pressedPos * m_duration / width()),
                           this, QRect());
        update();
    }
}

void QSUiWaveformSeekBar::drawWaveform()
{
    if(m_data.isEmpty())
    {
        m_pixmap = QPixmap();
        update();
        return;
    }

    bool showTwoChannels = m_showTwoChannelsAction->isChecked();
    bool showRms = m_showRmsAction->isChecked();

    m_pixmap = QPixmap(width(), height());
    m_pixmap.fill(m_bgColor);

    float step = float(width()) / NUMBER_OF_VALUES;

    QPainter painter(&m_pixmap);
    painter.setPen(m_waveFormColor);
    painter.setBrush(m_waveFormColor);

    for(int i = 0; i < m_data.size() - m_channels * 3 - 1; i+=3)
    {
        int ch = (i / 3) % m_channels;
        float x1 = step * (i / m_channels / 3);
        float x2 = step * (i / m_channels / 3 + 1);
        bool draw = false;
        float zeroPos = 0, ratio = 0;
        if(ch == 0 && (m_channels == 1 || !showTwoChannels))
        {
            zeroPos = height() / 2;
            ratio = float(height() / 4) / 1000;
            draw = true;
        }
        else if(ch == 0 || (ch == 1 && showTwoChannels))
        {
            zeroPos = ((ch == 0) ? 1 : 3) * height() / 4;
            ratio = float(height() / 8) / 1000;
            draw = true;
        }

        if(draw)
        {
            float y1 = zeroPos - m_data[i] * ratio;
            float y2 = zeroPos - m_data[i + 1] * ratio;
            float y3 = zeroPos - m_data[i + m_channels * 3] * ratio;
            float y4 = zeroPos - m_data[i + m_channels * 3 + 1] * ratio;

            QPointF points[4] = {
                { x1, y1 },
                { x1, y2 },
                { x2, y4 },
                { x2, y3 }
            };

            painter.drawPolygon(points, 4);
        }
    }

    if(!showRms)
    {
        update();
        return;
    }

    painter.setPen(m_rmsColor);
    painter.setBrush(m_rmsColor);

    for(int i = 0; i < m_data.size() - m_channels * 3 - 2; i+=3)
    {
        int ch = (i / 3) % m_channels;
        float x1 = step * (i / m_channels / 3);
        float x2 = step * (i / m_channels / 3 + 1);
        bool draw = false;
        float zeroPos = 0, ratio = 0;
        if(ch == 0 && (m_channels == 1 || !showTwoChannels))
        {
            zeroPos = height() / 2;
            ratio = float(height() / 4) / 1000;
            draw = true;
        }
        else if(ch == 0 || (ch == 1 && showTwoChannels))
        {
            zeroPos = ((ch == 0) ? 1 : 3) * height() / 4;
            ratio = float(height() / 8) / 1000;
            draw = true;
        }

        if(draw)
        {
            float y1 = zeroPos + m_data[i + 2] * ratio;
            float y2 = zeroPos - m_data[i + 2] * ratio;
            float y3 = zeroPos + m_data[i + m_channels * 3 + 2] * ratio;
            float y4 = zeroPos - m_data[i + m_channels * 3 + 2] * ratio;


            QPointF points[4] = {
                { x1, y1 },
                { x1, y2 },
                { x2, y4 },
                { x2, y3 }
            };

            painter.drawPolygon(points, 4);
        }
    }
    update();
}

void QSUiWaveformSeekBar::createMenu()
{
    m_menu = new QMenu(this);
    m_showTwoChannelsAction = m_menu->addAction(tr("2 Channels"), this, &QSUiWaveformSeekBar::writeSettings);
    m_showTwoChannelsAction->setCheckable(true);
    //: Root mean square
    m_showRmsAction = m_menu->addAction(tr("RMS"), this, &QSUiWaveformSeekBar::writeSettings);
    m_showRmsAction->setCheckable(true);
}

QSUiWaveformScanner::QSUiWaveformScanner(QObject *parent) : QThread(parent)
{}

QSUiWaveformScanner::~QSUiWaveformScanner()
{
    stop();
}

bool QSUiWaveformScanner::scan(const QString &path)
{
    //skip streams
    if(path.contains(u"://"_s) && InputSource::findByUrl(path))
        return false;

    InputSource *source = InputSource::create(path, this);
    if(!source->initialize())
    {
        delete source;
        qCWarning(plugin, "invalid path");
        return false;
    }

    if(source->ioDevice() && !source->ioDevice()->open(QIODevice::ReadOnly))
    {
        source->deleteLater();
        qCWarning(plugin, "cannot open input stream, error: %s",
                 qPrintable(source->ioDevice()->errorString()));
        return false;

    }

    DecoderFactory *factory = nullptr;

    if(!source->path().contains(u"://"_s))
        factory = Decoder::findByFilePath(source->path(), QmmpSettings::instance()->determineFileTypeByContent());
    if(!factory)
        factory = Decoder::findByMime(source->contentType());
    if(!factory && source->ioDevice() && source->path().contains(u"://"_s)) //ignore content of local files
        factory = Decoder::findByContent(source->ioDevice());
    if(!factory && source->path().contains(u"://"_s))
        factory = Decoder::findByProtocol(source->path().section(u"://"_s, 0, 0));
    if(!factory || source->path().startsWith(u"cdda://"_s))
    {
        qCWarning(plugin, "unsupported file format");
        source->deleteLater();
        return false;
    }
    qCDebug(plugin, "selected decoder: %s",qPrintable(factory->properties().shortName));
    if(factory->properties().noInput && source->ioDevice())
        source->ioDevice()->close();
    Decoder *decoder = factory->create(source->path(), source->ioDevice());
    if(!decoder->initialize())
    {
        qCWarning(plugin, "invalid file format");
        source->deleteLater();
        delete decoder;
        return false;
    }
    if(decoder->totalTime() <= 0)
    {
        source->deleteLater();
        delete decoder;
        return false;
    }
    m_decoder = decoder;
    m_input = source;
    m_user_stop = false;
    start();
    return true;
}

void QSUiWaveformScanner::stop()
{
    if(isRunning())
    {
        m_mutex.lock();
        m_user_stop = true;
        m_mutex.unlock();
        wait();
    }

    if(m_decoder)
    {
        delete m_decoder;
        m_decoder = nullptr;
    }

    if(m_input)
    {
        delete m_input;
        m_input = nullptr;
    }
}

QList<int> QSUiWaveformScanner::data() const
{
    QMutexLocker locker(&m_mutex);
    return m_data;
}

const AudioParameters &QSUiWaveformScanner::audioParameters() const
{
    return m_ap;
}

void QSUiWaveformScanner::run()
{
    m_ap = m_decoder->audioParameters();
    int channels = m_ap.channels();
    int frame_size = m_ap.frameSize();
    int sample_size = m_ap.sampleSize();
    qint64 frames = m_decoder->totalTime() * m_ap.sampleRate() / 1000;
    qint64 frames_per_value = frames / NUMBER_OF_VALUES;

    int tmp_size = QMMP_BLOCK_FRAMES * frame_size * 4;
    unsigned char* tmp = new unsigned char[tmp_size];
    float* out = new float[QMMP_BLOCK_FRAMES * channels * 4];

    AudioConverter converter;
    converter.configure(m_ap.format());

    // Number of elements in each stat buffer
    //  channels = 1, items = 4,  layout = |1111|
    //  channels = 2, items = 4,  layout = |1212|
    //  channels = 3, items = 12, layout = |1231|2312|3123|
    //  channels = 4, items = 4,  layout = |1234|
    //  channels = 5, items = 20, layout = |1234|5123|4512|3451|2345|
    //  channels = 6, items = 12, layout = |1234|5612|3456|
    //  channels = 7, items = 28, layout = |1234|5671|2345|6712|3456|7123|4567|
    //  channels = 8, items = 8,  layout = |1234|5678|
    //
    // For layouts containing less than 16 items, GCC or Clang do not produce
    // vectorized code at -O2. However, multiplying 'samples_per_stat' by a constant
    // results in the desired output.
    int samples_per_stat = std::lcm(FLOATS_PER_LANE, channels) * VECTORIZE_HINT;
    int frames_per_stat = samples_per_stat / channels;

    float *vmax = new float[samples_per_stat]{ -1.f };
    float *vmin = new float[samples_per_stat]{  1.f };
    float *vrms = new float[samples_per_stat]{  0.f };

    qint64 frame_counter = 0;
    qint64 frames_left = frames_per_value;

    m_data.clear();
    m_mutex.lock();

    while (!m_user_stop)
    {
        m_mutex.unlock();
        qint64 len = m_decoder->read(tmp, tmp_size);
        if(len > 0)
        {
            qint64 samples = len / sample_size;
            converter.toFloat(tmp, out, samples);

            for(qint64 sample = 0, samples_left = samples; sample < samples;)
            {
                // Vectorized path which should be taken most of the time
                if(frames_left >= frames_per_stat && samples_left >= samples_per_stat)
                {
                    for(int i = 0; i < samples_per_stat; ++i)
                    {
                        float value = out[sample + i];
                        vmin[i] = qMin(vmin[i], value);
                        vmax[i] = qMax(vmax[i], value);
                        vrms[i] += (value * value);
                    }

                    sample += samples_per_stat;
                    samples_left -= samples_per_stat;
                    frame_counter += frames_per_stat;
                    frames_left -= frames_per_stat;

                    // Collect stats into the first frame when transitioning to the scalar path
                    if(frames_left < frames_per_stat || samples_left < samples_per_stat)
                    {
                        for(int i = 1; i < frames_per_stat; ++i)
                        {
                            for(int ch = 0; ch < channels; ++ch)
                            {
                                int offset = channels * i + ch;
                                vmin[ch] = qMin(vmin[ch], vmin[offset]);
                                vmax[ch] = qMax(vmax[ch], vmax[offset]);
                                vrms[ch] += vrms[offset];
                                vmin[offset] =  1.f;
                                vmax[offset] = -1.f;
                                vrms[offset] =  0.f;
                            }
                        }
                    }
                }
                // Scalar path which should be taken when either
                // - the sample buffer is almost exhasted
                // - or, there are almost enough frames to emit a value
                else
                {
                    for(int ch = 0; ch < channels; ++ch)
                    {
                        float value = out[sample + ch];
                        vmin[ch] = qMin(vmin[ch], value);
                        vmax[ch] = qMax(vmax[ch], value);
                        vrms[ch] += (value * value);
                    }
                    sample += channels;
                    samples_left -= channels;
                    ++frame_counter;
                    --frames_left;
                }

                if(frame_counter >= frames_per_value)
                {
                    m_mutex.lock();
                    for(int ch = 0; ch < channels; ++ch)
                    {
                        m_data << vmax[ch] * 1000;
                        m_data << vmin[ch] * 1000;
                        m_data << std::sqrt(vrms[ch] / frame_counter) * 1000;
                        vmax[ch] = -1.f;
                        vmin[ch] =  1.f;
                        vrms[ch] =  0.f;
                    }
                    if(m_data.size() / 3 / channels % (NUMBER_OF_VALUES / 64) == 0)
                        emit dataChanged();
                    m_mutex.unlock();
                    frame_counter = 0;
                    frames_left = frames_per_value;
                }
            }
        }
        else
        {
            m_mutex.lock();
            break;
        }

        m_mutex.lock();
    }
    delete [] tmp;
    delete [] out;
    delete [] vmin;
    delete [] vmax;
    delete [] vrms;
    m_mutex.unlock();
}
