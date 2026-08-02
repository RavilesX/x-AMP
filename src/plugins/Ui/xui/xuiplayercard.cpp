/***************************************************************************
 *   Copyright (C) 2026 by x-AMP contributors                              *
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

#include <QHBoxLayout>
#include <QLabel>
#include <QPainter>
#include <QVBoxLayout>
#include <qmmp/soundcore.h>
#include <qmmp/metadatamanager.h>
#include <qmmpui/mediaplayer.h>
#include <qmmpui/qmmpuisettings.h>
#include "xuitheme.h"
#include "xuicontrols.h"
#include "xuivisualization.h"
#include "xuiplayercard.h"

namespace
{
    QString formatTime(qint64 ms)
    {
        if(ms < 0)
            return QStringLiteral("--:--");
        const qint64 total = ms / 1000;
        const qint64 hours = total / 3600;
        const qint64 minutes = (total % 3600) / 60;
        const qint64 seconds = total % 60;
        if(hours > 0)
            return QStringLiteral("%1:%2:%3").arg(hours)
                    .arg(minutes, 2, 10, QLatin1Char('0'))
                    .arg(seconds, 2, 10, QLatin1Char('0'));
        return QStringLiteral("%1:%2").arg(minutes, 2, 10, QLatin1Char('0'))
                .arg(seconds, 2, 10, QLatin1Char('0'));
    }

    QLabel *makeLabel(const QColor &color, qreal scale, bool bold = false)
    {
        QLabel *label = new QLabel();
        QFont f = label->font();
        f.setPointSizeF(f.pointSizeF() * scale);
        f.setBold(bold);
        label->setFont(f);
        QPalette pal = label->palette();
        pal.setColor(QPalette::WindowText, color);
        label->setPalette(pal);
        return label;
    }
}

XUiPlayerCard::XUiPlayerCard(QWidget *parent) : QWidget(parent)
{
    m_core = SoundCore::instance();
    m_player = MediaPlayer::instance();
    m_settings = QmmpUiSettings::instance();

    QVBoxLayout *root = new QVBoxLayout(this);
    root->setContentsMargins(XUi::CardPadding, XUi::CardPadding,
                             XUi::CardPadding, XUi::CardPadding);
    root->setSpacing(14);

    QHBoxLayout *top = new QHBoxLayout;
    top->setSpacing(18);
    m_cover = new XUiCoverArt(this);
    top->addWidget(m_cover, 0, Qt::AlignTop);
    top->addWidget(buildDetails(), 1);
    root->addLayout(top);

    m_seek = new XUiSlider(this);
    m_seek->setGripAlwaysVisible(true);
    root->addWidget(m_seek);

    root->addWidget(buildTransport());

    connect(m_core, &SoundCore::trackInfoChanged, this, &XUiPlayerCard::updateTrackInfo);
    connect(m_core, &SoundCore::stateChanged, this, &XUiPlayerCard::updateState);
    connect(m_core, &SoundCore::elapsedChanged, this, &XUiPlayerCard::updateElapsed);
    connect(m_core, &SoundCore::volumeChanged, this, &XUiPlayerCard::updateVolume);
    connect(m_core, &SoundCore::bitrateChanged, this, &XUiPlayerCard::updateBitrate);
    connect(m_seek, &XUiSlider::released, this, &XUiPlayerCard::seek);

    updateTrackInfo();
    updateState(m_core->state());
    updateVolume(m_core->volume());
}

QWidget *XUiPlayerCard::buildDetails()
{
    QWidget *panel = new QWidget(this);
    QVBoxLayout *layout = new QVBoxLayout(panel);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(6);

    //title row: track name on the left, per-track actions on the right
    QHBoxLayout *titleRow = new QHBoxLayout;
    titleRow->setSpacing(2);
    m_title = makeLabel(XUi::Text, 1.55, true);
    titleRow->addWidget(m_title, 1);
    for(XUiIcons::Icon icon : { XUiIcons::Heart, XUiIcons::More, XUiIcons::Kebab })
    {
        XUiIconButton *button = new XUiIconButton(icon, panel);
        button->setCheckable(icon == XUiIcons::Heart);
        titleRow->addWidget(button, 0, Qt::AlignTop);
    }
    layout->addLayout(titleRow);

    m_artist = makeLabel(XUi::Accent, 1.05);
    layout->addWidget(m_artist);

    //format badges, and the channel indicator on the far right
    QHBoxLayout *chipRow = new QHBoxLayout;
    chipRow->setSpacing(8);
    m_format = new XUiChip(panel);
    m_bitrate = new XUiChip(panel);
    m_sampleRate = new XUiChip(panel);
    chipRow->addWidget(m_format);
    chipRow->addWidget(m_bitrate);
    chipRow->addWidget(m_sampleRate);
    chipRow->addStretch(1);
    m_mono = makeLabel(XUi::TextFaint, 0.9, true);
    m_mono->setText(tr("MONO"));
    m_stereo = makeLabel(XUi::TextFaint, 0.9, true);
    m_stereo->setText(tr("STEREO"));
    chipRow->addWidget(m_mono);
    chipRow->addWidget(m_stereo);
    layout->addLayout(chipRow);

    layout->addStretch(1);

    //spectrum, elapsed/total, level meters
    QHBoxLayout *visRow = new QHBoxLayout;
    visRow->setSpacing(14);
    m_spectrum = new XUiSpectrum(panel);
    m_spectrum->setMinimumHeight(46);
    visRow->addWidget(m_spectrum, 1);
    m_time = makeLabel(XUi::TextDim, 1.0, true);
    visRow->addWidget(m_time, 0, Qt::AlignBottom);
    m_vu = new XUiVuMeter(panel);
    m_vu->setFixedSize(84, 52);
    visRow->addWidget(m_vu, 0, Qt::AlignBottom);
    layout->addLayout(visRow);

    Visual::add(m_spectrum);
    Visual::add(m_vu);
    return panel;
}

QWidget *XUiPlayerCard::buildTransport()
{
    QWidget *panel = new QWidget(this);
    QHBoxLayout *layout = new QHBoxLayout(panel);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(10);

    m_shuffle = new XUiIconButton(XUiIcons::Shuffle, panel);
    m_shuffle->setCheckable(true);
    m_shuffle->setChecked(m_settings->isShuffle());
    connect(m_shuffle, &XUiIconButton::toggled, m_settings, &QmmpUiSettings::setShuffle);
    connect(m_settings, &QmmpUiSettings::shuffleChanged, m_shuffle, &XUiIconButton::setChecked);

    XUiIconButton *previous = new XUiIconButton(XUiIcons::Previous, panel);
    previous->setIconSize(24);
    connect(previous, &XUiIconButton::clicked, m_player, &MediaPlayer::previous);

    m_play = new XUiPlayButton(panel);
    connect(m_play, &XUiPlayButton::clicked, this, [this] {
        if(m_core->state() == Qmmp::Playing || m_core->state() == Qmmp::Paused)
            m_player->pause();
        else
            m_player->play();
    });

    XUiIconButton *next = new XUiIconButton(XUiIcons::Next, panel);
    next->setIconSize(24);
    connect(next, &XUiIconButton::clicked, m_player, &MediaPlayer::next);

    m_repeat = new XUiIconButton(XUiIcons::Repeat, panel);
    m_repeat->setCheckable(true);
    m_repeat->setChecked(m_settings->isRepeatableList());
    connect(m_repeat, &XUiIconButton::toggled, m_settings, &QmmpUiSettings::setRepeatableList);
    connect(m_settings, &QmmpUiSettings::repeatableListChanged, m_repeat, &XUiIconButton::setChecked);

    m_volumeIcon = new XUiIconButton(XUiIcons::Volume, panel);
    connect(m_volumeIcon, &XUiIconButton::clicked, this, [this] {
        m_core->setMuted(!m_core->isMuted());
    });

    m_volume = new XUiSlider(panel);
    m_volume->setMaximum(100);
    m_volume->setFixedWidth(120);
    connect(m_volume, &XUiSlider::moved, m_core, &SoundCore::setVolume);

    layout->addStretch(1);
    layout->addWidget(m_shuffle);
    layout->addSpacing(6);
    layout->addWidget(previous);
    layout->addSpacing(6);
    layout->addWidget(m_play);
    layout->addSpacing(6);
    layout->addWidget(next);
    layout->addSpacing(6);
    layout->addWidget(m_repeat);
    layout->addStretch(1);
    layout->addWidget(m_volumeIcon);
    layout->addWidget(m_volume);
    return panel;
}

void XUiPlayerCard::updateTrackInfo()
{
    const TrackInfo info = m_core->trackInfo();

    const QString title = info.value(Qmmp::TITLE);
    m_title->setText(title.isEmpty() ? tr("x-AMP") : title);
    const QString artist = info.value(Qmmp::ARTIST);
    m_artist->setText(artist.isEmpty() ? tr("Not playing") : artist);

    const AudioParameters params = m_core->audioParameters();
    m_format->setText(info.value(Qmmp::FORMAT_NAME));
    updateBitrate(m_core->bitrate());
    const int rate = params.sampleRate();
    m_sampleRate->setText(rate > 0 ? tr("%1 kHz").arg(rate / 1000.0, 0, 'g', 3) : QString());
    //an empty badge would still reserve its padding
    m_format->setVisible(!m_format->text().isEmpty());
    m_sampleRate->setVisible(rate > 0);

    //the active channel mode is highlighted, the other stays dim
    const bool stereo = params.channels() > 1;
    QPalette monoPal = m_mono->palette();
    monoPal.setColor(QPalette::WindowText, stereo ? XUi::TextFaint : XUi::Accent);
    m_mono->setPalette(monoPal);
    QPalette stereoPal = m_stereo->palette();
    stereoPal.setColor(QPalette::WindowText, stereo ? XUi::Accent : XUi::TextFaint);
    m_stereo->setPalette(stereoPal);

    m_seek->setMaximum(m_core->duration());
    updateTimeLabel(m_core->elapsed());

    const QPixmap cover = QPixmap::fromImage(
        MetaDataManager::instance()->getCover(info.path()));
    if(cover.isNull())
        m_cover->clear();
    else
        m_cover->setCover(cover);
}

void XUiPlayerCard::updateBitrate(int bitrate)
{
    //arrives after the metadata, and again whenever a VBR stream shifts
    m_bitrate->setText(bitrate > 0 ? tr("%1 kbps").arg(bitrate) : QString());
    m_bitrate->setVisible(bitrate > 0);
}

void XUiPlayerCard::updateState(Qmmp::State state)
{
    const bool playing = state == Qmmp::Playing;
    m_play->setPlaying(playing || state == Qmmp::Paused);
    if(state == Qmmp::Stopped)
    {
        m_seek->setValue(0);
        m_play->setProgress(0.0);
        m_spectrum->clear();
        m_vu->clear();
        updateTimeLabel(-1);
    }
}

void XUiPlayerCard::updateElapsed(qint64 elapsed)
{
    m_seek->setMaximum(m_core->duration());
    m_seek->setValue(elapsed);
    const qint64 duration = m_core->duration();
    m_play->setProgress(duration > 0 ? qreal(elapsed) / qreal(duration) : 0.0);
    updateTimeLabel(elapsed);
}

void XUiPlayerCard::updateTimeLabel(qint64 elapsed)
{
    const qint64 duration = m_core->duration();
    m_time->setText(QStringLiteral("%1 / %2")
                    .arg(formatTime(elapsed), formatTime(duration > 0 ? duration : -1)));
}

void XUiPlayerCard::updateVolume(int volume)
{
    m_volume->setValue(volume);
    m_volumeIcon->setIcon(m_core->isMuted() || volume == 0 ? XUiIcons::VolumeMuted
                                                           : XUiIcons::Volume);
}

void XUiPlayerCard::seek(qint64 position)
{
    m_core->seek(position);
}

void XUiPlayerCard::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);
    QLinearGradient g(rect().topLeft(), rect().bottomLeft());
    g.setColorAt(0.0, XUi::CardTop);
    g.setColorAt(1.0, XUi::Card);
    p.setPen(QPen(XUi::Border, 1));
    p.setBrush(g);
    p.drawRoundedRect(QRectF(rect()).adjusted(0.5, 0.5, -0.5, -0.5),
                      XUi::CardRadius, XUi::CardRadius);
}
