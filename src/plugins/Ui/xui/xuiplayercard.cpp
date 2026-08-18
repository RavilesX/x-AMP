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
#include <QLocale>
#include <QPainter>
#include <QVBoxLayout>
#include <qmmp/soundcore.h>
#include <qmmp/metadatamanager.h>
#include <qmmp/effect.h>
#include <qmmp/effectfactory.h>
#include <qmmpui/mediaplayer.h>
#include <qmmpui/qmmpuisettings.h>
#include <qmmpui/playlistmanager.h>
#include <qmmpui/playlistmodel.h>
#include <qmmpui/scheduler.h>
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
    root->setSpacing(10);

    QHBoxLayout *top = new QHBoxLayout;
    top->setSpacing(14);
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
    //muting leaves the volume untouched, so the glyph would otherwise keep
    //showing sound coming out of a silent player
    connect(m_core, &SoundCore::mutedChanged, this, [this] {
        updateVolume(m_core->volume());
    });
    connect(m_core, &SoundCore::bitrateChanged, this, &XUiPlayerCard::updateBitrate);
    connect(m_seek, &XUiSlider::released, this, &XUiPlayerCard::seek);

    if(Scheduler *scheduler = Scheduler::instance())
        connect(scheduler, &Scheduler::settingsChanged,
                this, &XUiPlayerCard::updateScheduler);

    updateTrackInfo();
    updateState(m_core->state());
    updateVolume(m_core->volume());
    updateScheduler();
}

void XUiPlayerCard::applyAccent()
{
    QPalette pal = m_artist->palette();
    pal.setColor(QPalette::WindowText, XUi::Accent);
    m_artist->setPalette(pal);
    updateTrackInfo(); //repaints the MONO/STEREO pair with the new accent
}

QWidget *XUiPlayerCard::buildDetails()
{
    QWidget *panel = new QWidget(this);
    QVBoxLayout *layout = new QVBoxLayout(panel);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(4);

    //Title row: track name, and the details button. The design also shows a
    //heart and an overflow menu; neither maps onto anything the engine can
    //do -- there are no favourites, and every per-track action already lives
    //in the playlist's context menu -- so they are left out rather than
    //shipped dead.
    QHBoxLayout *titleRow = new QHBoxLayout;
    titleRow->setSpacing(2);
    m_title = makeLabel(XUi::Text, 1.3, true);
    titleRow->addWidget(m_title, 1);

    //Sits ahead of the details button and always on screen: it is the way
    //into the scheduler either way, and lit or unlit it answers at a glance
    //whether something is going to close the player, or the machine, on its
    //own. Lit through setChecked() on a button that is not setCheckable(),
    //as the repeat button does -- the state is the scheduler's, not a
    //toggle the click owns.
    m_scheduler = new XUiIconButton(XUiIcons::Clock, panel);
    m_scheduler->setIconSize(16);
    //unlit it stands for a switched-off scheduler, so it takes the grey the
    //MUTE and 100% words use for the same meaning
    m_scheduler->setRestColor(XUi::TextFaint);
    connect(m_scheduler, &XUiIconButton::clicked,
            this, &XUiPlayerCard::schedulerRequested);
    titleRow->addWidget(m_scheduler, 0, Qt::AlignTop);

    XUiIconButton *details = new XUiIconButton(XUiIcons::Kebab, panel);
    details->setToolTip(tr("Track details"));
    connect(details, &XUiIconButton::clicked, this, [this] {
        PlayListManager::instance()->currentPlayList()->showDetailsForCurrent(this);
    });
    titleRow->addWidget(details, 0, Qt::AlignTop);
    layout->addLayout(titleRow);

    m_artist = makeLabel(XUi::Accent, 1.0);
    layout->addWidget(m_artist);

    //format badges
    QHBoxLayout *chipRow = new QHBoxLayout;
    chipRow->setSpacing(8);
    m_format = new XUiChip(panel);
    m_bitrate = new XUiChip(panel);
    m_sampleRate = new XUiChip(panel);
    chipRow->addWidget(m_format);
    chipRow->addWidget(m_bitrate);
    chipRow->addWidget(m_sampleRate);
    chipRow->addStretch(1);
    layout->addLayout(chipRow);

    layout->addStretch(1);

    //MONO/STEREO sits directly over the meters, in one column with them, so
    //the two share a width and each word lands over its own channel
    QWidget *channels = new QWidget(panel);
    QVBoxLayout *channelColumn = new QVBoxLayout(channels);
    channelColumn->setContentsMargins(0, 0, 0, 0);
    channelColumn->setSpacing(4);

    QHBoxLayout *channelLabels = new QHBoxLayout;
    channelLabels->setSpacing(6);
    m_mono = makeLabel(XUi::TextFaint, 0.85, true);
    m_mono->setText(tr("MONO"));
    m_stereo = makeLabel(XUi::TextFaint, 0.85, true);
    m_stereo->setText(tr("STEREO"));
    channelLabels->addWidget(m_mono, 1, Qt::AlignCenter);
    channelLabels->addWidget(m_stereo, 1, Qt::AlignCenter);
    channelColumn->addLayout(channelLabels);

    m_vu = new XUiVuMeter(panel);
    m_vu->setFixedHeight(34); //width follows the labels above
    channelColumn->addWidget(m_vu);

    //spectrum, elapsed/total, then the channel column
    QHBoxLayout *visRow = new QHBoxLayout;
    visRow->setSpacing(10);
    m_spectrum = new XUiSpectrum(panel);
    m_spectrum->setMinimumHeight(34);
    visRow->addWidget(m_spectrum, 1);
    m_time = makeLabel(XUi::TextDim, 1.0, true);
    visRow->addWidget(m_time, 0, Qt::AlignBottom);
    visRow->addWidget(channels, 0, Qt::AlignBottom);
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

    XUiIconButton *stop = new XUiIconButton(XUiIcons::Stop, panel);
    stop->setIconSize(21); //a filled square reads smaller than the triangles
    stop->setToolTip(tr("Stop"));
    connect(stop, &XUiIconButton::clicked, m_player, &MediaPlayer::stop);

    XUiIconButton *next = new XUiIconButton(XUiIcons::Next, panel);
    next->setIconSize(24);
    next->setToolTip(tr("Next"));
    connect(next, &XUiIconButton::clicked, this, &XUiPlayerCard::nextTrack);

    //Three states rather than a checkbox: repeating only the playlist is
    //barely observable, and repeat-one is what the second press is expected
    //to do. The engine keeps the two as separate flags.
    m_repeat = new XUiIconButton(XUiIcons::Repeat, panel);
    connect(m_repeat, &XUiIconButton::clicked, this, &XUiPlayerCard::cycleRepeat);
    connect(m_settings, &QmmpUiSettings::repeatableListChanged,
            this, &XUiPlayerCard::updateRepeat);
    connect(m_settings, &QmmpUiSettings::repeatableTrackChanged,
            this, &XUiPlayerCard::updateRepeat);
    updateRepeat();

    //Crossfade is an effect plugin rather than a player setting, so the glyph
    //drives Effect::setEnabled() -- which adds or drops it in the running
    //engine and writes the choice out, the same way the preferences page does.
    m_crossfade = new XUiIconButton(XUiIcons::Crossfade, panel);
    m_crossfade->setCheckable(true);
    connect(m_crossfade, &XUiIconButton::toggled, this, [](bool on) {
        if(EffectFactory *factory = Effect::findFactory(QStringLiteral("crossfade")))
            Effect::setEnabled(factory, on);
    });
    updateCrossfade();

    m_volumeIcon = new XUiIconButton(XUiIcons::Volume, panel);
    connect(m_volumeIcon, &XUiIconButton::clicked, this, [this] {
        m_core->setMuted(!m_core->isMuted());
    });


    layout->addStretch(1);
    layout->addWidget(m_shuffle);
    layout->addSpacing(6);
    layout->addWidget(previous);
    layout->addSpacing(6);
    layout->addWidget(m_play);
    layout->addSpacing(6);
    layout->addWidget(stop);
    layout->addSpacing(6);
    layout->addWidget(next);
    layout->addSpacing(6);
    layout->addWidget(m_repeat);
    layout->addSpacing(6);
    layout->addWidget(m_crossfade);
    layout->addStretch(1);
    layout->addWidget(m_volumeIcon);
    layout->addWidget(buildVolume(panel));
    return panel;
}

QWidget *XUiPlayerCard::buildVolume(QWidget *panel)
{
    QWidget *column = new QWidget(panel);
    QVBoxLayout *layout = new QVBoxLayout(column);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(3);

    //Two shortcuts over the slider, in the manner of MONO/STEREO over the
    //meters: one word at each end of the travel they stand for. Each one is a
    //toggle -- switching it off puts the volume back where it was found.
    m_mute = new XUiTextToggle(tr("MUTE"), column);
    m_mute->setToolTip(tr("Silence, and back to the previous level"));
    m_full = new XUiTextToggle(tr("100%"), column);
    m_full->setAlignment(Qt::AlignRight); //its own end of the rail is the right one
    m_full->setToolTip(tr("Full volume, and back to the previous level"));

    QHBoxLayout *shortcuts = new QHBoxLayout;
    //inset to the ends of the rail rather than to the edges of the widget:
    //the slider keeps a grip's radius clear at both ends, and a word set
    //flush with the widget sits that much wide of the level it names
    shortcuts->setContentsMargins(XUiSlider::RailInset, 0, XUiSlider::RailInset, 0);
    shortcuts->addWidget(m_mute);
    shortcuts->addStretch(1);
    shortcuts->addWidget(m_full);
    layout->addLayout(shortcuts);

    m_volume = new XUiSlider(column);
    m_volume->setMaximum(100);
    m_volume->setWheelStep(4); //percent per notch, hovering is enough
    m_volume->setFixedWidth(120);
    connect(m_volume, &XUiSlider::moved, m_core, &SoundCore::setVolume);
    //A hand on the slider is the newer instruction: whatever a shortcut had
    //put aside is dropped, so switching the shortcut off later leaves the
    //level the user has just chosen alone. The words themselves are cleared by
    //updateVolume(), which sees every change wherever it comes from.
    connect(m_volume, &XUiSlider::moved, this, [this] { m_savedVolume = -1; });
    layout->addWidget(m_volume);

    //Both shortcuts work on the volume itself rather than on the engine's
    //mute flag: the point of the pair is that the slider goes where the word
    //says, and comes back when the word is switched off. The flag is only
    //cleared, never set, so that neither shortcut can end in silence the
    //slider does not account for.
    auto shortcut = [this](XUiTextToggle *other, int level) {
        return [this, other, level](bool on) {
            if(!on)
            {
                restoreVolume();
                return;
            }
            m_savedVolume = m_core->volume();
            other->setChecked(false);
            m_core->setMuted(false);
            m_core->setVolume(level);
        };
    };
    connect(m_mute, &XUiTextToggle::toggled, this, shortcut(m_full, 0));
    connect(m_full, &XUiTextToggle::toggled, this, shortcut(m_mute, 100));
    return column;
}

void XUiPlayerCard::restoreVolume()
{
    if(m_savedVolume < 0)
        return;
    m_core->setVolume(m_savedVolume);
    m_savedVolume = -1;
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
    //the glyph offers the next action: pause while playing, play otherwise --
    //so a paused player shows a play triangle, not a second pause icon
    m_play->setPlaying(state == Qmmp::Playing);
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
    //A shortcut only holds while the level it set is the level in force -- a
    //global hotkey or an MPRIS client can move it too -- so the word above the
    //slider stops claiming otherwise. Nothing is put back: whatever moved the
    //volume is the newer instruction.
    if((m_full->isChecked() && volume != 100) || (m_mute->isChecked() && volume != 0))
    {
        m_full->setChecked(false);
        m_mute->setChecked(false);
        m_savedVolume = -1;
    }
    m_volumeIcon->setIcon(m_core->isMuted() || volume == 0 ? XUiIcons::VolumeMuted
                                                           : XUiIcons::Volume);
}

void XUiPlayerCard::seek(qint64 position)
{
    m_core->seek(position);
}

void XUiPlayerCard::nextTrack()
{
    //At the end of the playlist the engine's next() simply stops, since
    //wrapping is what repeat-list is for. Pressing Next explicitly is a
    //different intent, so wrap here -- unless shuffle or repeat already
    //decide the order themselves.
    PlayListModel *playList = PlayListManager::instance()->currentPlayList();
    const bool wrap = playList->trackCount() > 0
                      && playList->currentIndex() >= playList->trackCount() - 1
                      && !m_settings->isShuffle()
                      && !m_settings->isRepeatableList();
    if(wrap)
    {
        playList->setCurrent(0);
        //stop first: play() would only resume a paused track, ignoring the
        //one just made current
        m_player->stop();
        m_player->play();
        return;
    }
    m_player->next();
}

void XUiPlayerCard::cycleRepeat()
{
    if(m_settings->isRepeatableTrack())
    {
        m_settings->setRepeatableTrack(false);
        m_settings->setRepeatableList(false);
    }
    else if(m_settings->isRepeatableList())
    {
        m_settings->setRepeatableList(false);
        m_settings->setRepeatableTrack(true);
    }
    else
    {
        m_settings->setRepeatableList(true);
    }
    updateRepeat();
}

void XUiPlayerCard::updateRepeat()
{
    const bool track = m_settings->isRepeatableTrack();
    const bool list = m_settings->isRepeatableList();
    m_repeat->setIcon(track ? XUiIcons::RepeatOne : XUiIcons::Repeat);
    m_repeat->setChecked(track || list);
    m_repeat->setToolTip(track ? tr("Repeat track")
                               : (list ? tr("Repeat playlist") : tr("No repeat")));
}

void XUiPlayerCard::updateCrossfade()
{
    //A build without the plugin would otherwise carry a switch that does
    //nothing, so the glyph is left out entirely rather than shown dead.
    EffectFactory *factory = Effect::findFactory(QStringLiteral("crossfade"));
    m_crossfade->setVisible(factory != nullptr);
    if(!factory)
        return;

    m_crossfade->setChecked(Effect::isEnabled(factory));
    m_crossfade->setToolTip(tr("Crossfade between tracks"));
}

void XUiPlayerCard::updateScheduler()
{
    Scheduler *scheduler = Scheduler::instance();
    const QString openIt = QLatin1Char('\n') + tr("Click to open the scheduler");
    if(!scheduler || !scheduler->isEnabled())
    {
        m_scheduler->setChecked(false);
        m_scheduler->setToolTip(tr("Scheduler off") + openIt);
        return;
    }

    QString action;
    switch(scheduler->action())
    {
    case Scheduler::PLAY_FILE:
        action = tr("play a file");
        break;
    case Scheduler::PLAY_PLAYLIST:
        action = tr("play a playlist");
        break;
    case Scheduler::QUIT:
        action = tr("close the player");
        break;
    case Scheduler::SUSPEND:
        action = tr("suspend the computer");
        break;
    case Scheduler::SHUTDOWN:
        action = tr("shut the computer down");
        break;
    }

    QString when;
    if(scheduler->isArmedForPlayListEnd())
        when = tr("at the end of the playlist");
    else if(scheduler->deadline().isValid())
        when = tr("at %1").arg(QLocale().toString(scheduler->deadline(),
                                                  QStringLiteral("dddd HH:mm")));

    //two lines: what is going to happen, then how to get at it
    m_scheduler->setChecked(true);
    m_scheduler->setToolTip(tr("Scheduled: %1 %2").arg(action, when).trimmed() + openIt);
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
