/***************************************************************************
 *   Copyright (C) 2006-2026 by Ilya Kotov                                 *
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

#include <QApplication>
#include <QSettings>
#include <QMenu>
#include <QTimer>
#include <qmmp/soundcore.h>
#include <qmmp/qmmpsettings.h>
#include <qmmpui/mediaplayer.h>
#include <qmmpui/playlistmanager.h>
#include <qmmpui/qmmpuisettings.h>
#include "skin.h"
#include "skinnedvisualization.h"
#include "skinnedbutton.h"
#include "skinnedtitlebar.h"
#include "skinnedpositionbar.h"
#include "skinnedtogglebutton.h"
#include "symboldisplay.h"
#include "skinnedtextscroller.h"
#include "monostereo.h"
#include "skinnedplaystatus.h"
#include "skinnedvolumebar.h"
#include "skinnedbalancebar.h"
#include "skinnedmainwindow.h"
#include "skinnedtimeindicator.h"
#include "skinnedactionmanager.h"
#include "skinnedeqwidget.h"
#include "skinnedplaylist.h"
#include "skinneddisplay.h"

SkinnedDisplay::SkinnedDisplay(SkinnedMainWindow *parent) : PixmapWidget (parent)
{
    setPixmap(skin()->getMain());
    setCursor(skin()->getCursor(Skin::CUR_NORMAL));
    m_mw = parent;
    m_core = SoundCore::instance();
    MediaPlayer *player = MediaPlayer::instance();

    m_timeIndicatorModel = new SkinnedTimeIndicatorModel(this);
    m_titlebar = new SkinnedTitleBar(m_timeIndicatorModel, this);
    m_titlebar->move(0, 0);
    m_titlebar->setActive(true);
    m_previous = new SkinnedButton (this, Skin::BT_PREVIOUS_N, Skin::BT_PREVIOUS_P, Skin::CUR_NORMAL);
    m_previous->setToolTip(tr("Previous"));
    connect(m_previous, &SkinnedButton::clicked, player, &MediaPlayer::previous);

    m_play = new SkinnedButton(this, Skin::BT_PLAY_N, Skin::BT_PLAY_P, Skin::CUR_NORMAL);
    m_play->setToolTip(tr("Play"));
    connect(m_play, &SkinnedButton::clicked, player, &MediaPlayer::play);
    m_pause = new SkinnedButton (this, Skin::BT_PAUSE_N,Skin::BT_PAUSE_P, Skin::CUR_NORMAL);
    m_pause->setToolTip(tr("Pause"));
    connect(m_pause, &SkinnedButton::clicked, player, &MediaPlayer::pause);
    m_stop = new SkinnedButton(this, Skin::BT_STOP_N, Skin::BT_STOP_P, Skin::CUR_NORMAL);
    m_stop->setToolTip(tr("Stop"));
    connect(m_stop, &SkinnedButton::clicked, player, &MediaPlayer::stop);
    m_next = new SkinnedButton(this, Skin::BT_NEXT_N, Skin::BT_NEXT_P, Skin::CUR_NORMAL);
    m_next->setToolTip(tr("Next"));
    connect(m_next, &SkinnedButton::clicked, player, &MediaPlayer::next);
    m_eject = new SkinnedButton(this, Skin::BT_EJECT_N, Skin::BT_EJECT_P, Skin::CUR_NORMAL);
    m_eject->setToolTip(tr("Play files"));
    connect(m_eject, &SkinnedButton::clicked, parent, &SkinnedMainWindow::playFiles);
    m_vis = new SkinnedVisualization (this);

    m_eqButton = new SkinnedToggleButton(this, Skin::BT_EQ_ON_N,Skin::BT_EQ_ON_P, Skin::BT_EQ_OFF_N,Skin::BT_EQ_OFF_P);
    m_eqButton->setToolTip(tr("Equalizer"));
    m_plButton = new SkinnedToggleButton(this, Skin::BT_PL_ON_N,Skin::BT_PL_ON_P, Skin::BT_PL_OFF_N,Skin::BT_PL_OFF_P);
    m_plButton->setToolTip(tr("Playlist"));
    m_repeatButton = new SkinnedToggleButton(this,Skin::REPEAT_ON_N,Skin::REPEAT_ON_P, Skin::REPEAT_OFF_N, Skin::REPEAT_OFF_P);
    connect(m_repeatButton, &SkinnedToggleButton::clicked, this, &SkinnedDisplay::repeatableToggled);
    m_repeatButton->setToolTip(tr("Repeat playlist"));
    m_shuffleButton = new SkinnedToggleButton(this,Skin::SHUFFLE_ON_N, Skin::SHUFFLE_ON_P, Skin::SHUFFLE_OFF_N,Skin::SHUFFLE_OFF_P);
    m_shuffleButton->setToolTip(tr("Shuffle"));
    connect(m_shuffleButton, &SkinnedToggleButton::clicked, this, &SkinnedDisplay::shuffleToggled);

    m_kbps = new SymbolDisplay(this, 3);
    m_freq = new SymbolDisplay(this, 2);
    m_text = new SkinnedTextScroller(this);
    m_monoster = new MonoStereo(this);
    m_playstatus = new SkinnedPlayStatus(this);

    m_volumeBar = new SkinnedVolumeBar(this);
    m_volumeBar->setToolTip(tr("Volume"));
    m_volumeDisplayTimer = new QTimer(this);
    m_volumeDisplayTimer->setSingleShot(true);
    m_volumeDisplayTimer->setInterval(1000);
    connect(m_volumeBar, &SkinnedVolumeBar::sliderMoved, this, &SkinnedDisplay::displayVolume);
    connect(m_volumeBar, &SkinnedVolumeBar::sliderPressed, this, &SkinnedDisplay::displayVolume);
    connect(m_volumeBar, &SkinnedVolumeBar::sliderReleased, m_text, &SkinnedTextScroller::clear);
    connect(m_volumeDisplayTimer, &QTimer::timeout, m_text, &SkinnedTextScroller::clear);

    m_balanceBar = new SkinnedBalanceBar(this);
    m_balanceBar->setToolTip(tr("Balance"));
    connect(m_balanceBar, &SkinnedBalanceBar::sliderMoved, this, &SkinnedDisplay::displayVolume);
    connect(m_balanceBar, &SkinnedBalanceBar::sliderPressed, this, &SkinnedDisplay::displayVolume);
    connect(m_balanceBar, &SkinnedBalanceBar::sliderReleased, m_text, &SkinnedTextScroller::clear);

    m_posbar = new SkinnedPositionBar(this);
    connect(m_posbar, &SkinnedPositionBar::sliderPressed, this, &SkinnedDisplay::showPosition);
    connect(m_posbar, &SkinnedPositionBar::sliderMoved, this, &SkinnedDisplay::showPosition);
    connect(m_posbar, &SkinnedPositionBar::sliderReleased, this, &SkinnedDisplay::updatePosition);

    m_timeIndicator = new SkinnedTimeIndicator(m_timeIndicatorModel, this);
    m_aboutWidget = new QWidget(this);

    connect(m_core, &SoundCore::elapsedChanged, this, &SkinnedDisplay::setTime);
    connect(m_core, &SoundCore::bitrateChanged, m_kbps, &SymbolDisplay::displayNum);
    connect(m_core, &SoundCore::audioParametersChanged, this, &SkinnedDisplay::onAudioPatametersChanged);
    connect(m_core, &SoundCore::stateChanged, this, &SkinnedDisplay::setState);
    connect(m_core, &SoundCore::volumeChanged, m_volumeBar, &SkinnedVolumeBar::setValue);
    connect(m_core, &SoundCore::balanceChanged, m_balanceBar, &SkinnedBalanceBar::setValue);
    connect(m_balanceBar, &SkinnedBalanceBar::sliderMoved, m_core, &SoundCore::setBalance);
    connect(m_volumeBar, &SkinnedVolumeBar::sliderMoved, m_core, &SoundCore::setVolume);
    m_volumeBar->setValue(m_core->volume());
    m_balanceBar->setValue(m_core->balance());

    QmmpUiSettings *ui_settings = QmmpUiSettings::instance();
    connect(ui_settings, &QmmpUiSettings::repeatableListChanged, m_repeatButton, &SkinnedToggleButton::setChecked);
    connect(ui_settings, &QmmpUiSettings::shuffleChanged, m_shuffleButton, &SkinnedToggleButton::setChecked);
    updatePositions();
    updateMask();
}

SkinnedDisplay::~SkinnedDisplay()
{
    QSettings settings;
    settings.setValue("Skinned/pl_visible"_L1, m_plButton->isChecked());
    settings.setValue("Skinned/eq_visible"_L1, m_eqButton->isChecked());
}

void SkinnedDisplay::updatePositions()
{
    int r = skin()->ratio();
    m_previous->move(r * 16, r * 88);
    m_play->move(r * 39, r * 88);
    m_pause->move(r * 62,  r * 88);
    m_vis->move(r * 24, r * 43);
    m_stop->move(r * 85,  r * 88);
    m_next->move(r * 108, r * 88);
    m_eject->move(r*136, r * 89);
    m_posbar->move(r * 16, r * 72);
    m_eqButton->move(r * 219, r * 58);
    m_plButton->move(r * 241, r * 58);
    m_repeatButton->move(r * 210, r * 89);
    m_shuffleButton->move(r * 164, r * 89);
    m_kbps->move(r * 111, r * 43);
    m_freq->move(r * 156, r * 43);
    m_text->resize(r * 154, r * 14);
    m_text->move(r * 110, r * 23);
    m_monoster->move(r * 212, r * 41);
    m_playstatus->move(r * 24, r * 28);
    m_volumeBar->move(r * 107, r * 57);
    m_balanceBar->move(r * 177, r * 57);
    m_timeIndicator->move(r * 34, r * 26);
    m_aboutWidget->setGeometry(r * 247, r * 83, r * 20, r * 25);
}

void SkinnedDisplay::setTime (qint64 t)
{
    m_posbar->setValue(t);
    m_timeIndicatorModel->setPosition(t / 1000);
}
void SkinnedDisplay::setDuration(qint64 t)
{
    m_posbar->setMaximum(t);
    m_timeIndicatorModel->setDuration(t / 1000);
}

void SkinnedDisplay::setState(Qmmp::State state)
{
    switch(state)
    {
    case Qmmp::Playing:
        m_playstatus->setState(Qmmp::Playing);
        m_timeIndicatorModel->setVisible(true);
        setDuration(m_core->duration());
        break;
    case Qmmp::Paused:
        m_playstatus->setState(Qmmp::Paused);
        break;
    case Qmmp::Stopped:
        m_playstatus->setState(Qmmp::Stopped);
        m_monoster->setChannels (0);
        m_timeIndicatorModel->setVisible(false);
        m_posbar->setValue(0);
        m_posbar->setMaximum(0);
    default:
        ;
    }
}

void SkinnedDisplay::onAudioPatametersChanged(const AudioParameters &p)
{
    m_monoster->setChannels(p.channels());
    m_freq->displayNum(int(p.sampleRate()) / 1000);
}

void SkinnedDisplay::updateSkin()
{
    setPixmap(skin()->getMain());
    m_mw->resize(size());
    setCursor(skin()->getCursor(Skin::CUR_NORMAL));
    setMinimalMode(m_shaded);
    updatePositions();
}

void SkinnedDisplay::updateMask()
{
    m_mw->clearMask();
    m_mw->setMask(QRegion(0, 0, m_mw->width(), m_mw->height()));
    QRegion region = skin()->getRegion(m_shaded? Skin::WINDOW_SHADE : Skin::NORMAL);
    if (!region.isEmpty())
        m_mw->setMask(region);
}

void SkinnedDisplay::setMinimalMode(bool b)
{
    m_shaded = b;
    int r = skin()->ratio();

    if(m_shaded)
         m_mw->setFixedSize(r * 275,r*14);
    else
         m_mw->setFixedSize(r * 275, r * 116);
    updateMask();
}

void SkinnedDisplay::setActive(bool b)
{
    m_titlebar->setActive(b);
}

void SkinnedDisplay::setEQ(SkinnedEqWidget* w)
{
    m_equlizer = w;
    m_eqButton->setChecked(m_equlizer->isVisible());
    ACTION(SkinnedActionManager::SHOW_EQUALIZER)->setChecked(m_equlizer->isVisible());

    connect(ACTION(SkinnedActionManager::SHOW_EQUALIZER), &QAction::triggered, m_equlizer, &QWidget::setVisible);
    connect(ACTION(SkinnedActionManager::SHOW_EQUALIZER), &QAction::triggered, m_eqButton, &SkinnedToggleButton::setChecked);

    connect(m_eqButton, &SkinnedToggleButton::clicked, ACTION(SkinnedActionManager::SHOW_EQUALIZER), &QAction::setChecked);
    connect(m_eqButton, &SkinnedToggleButton::clicked, m_equlizer, &QWidget::setVisible);
    connect(m_equlizer, &SkinnedEqWidget::closed, m_eqButton, [this] {
        m_eqButton->setChecked(false);
        m_equlizer->setVisible(false);
    });
}

void SkinnedDisplay::setPL(SkinnedPlayList *w)
{
    m_playlist = w;
    m_plButton->setChecked(m_playlist->isVisible());
    ACTION(SkinnedActionManager::SHOW_PLAYLIST)->setChecked(m_playlist->isVisible());

    connect(ACTION(SkinnedActionManager::SHOW_PLAYLIST), &QAction::triggered, m_playlist, &QWidget::setVisible);
    connect(ACTION(SkinnedActionManager::SHOW_PLAYLIST), &QAction::triggered, m_plButton, &SkinnedToggleButton::setChecked);

    connect(m_plButton, &SkinnedToggleButton::clicked, ACTION(SkinnedActionManager::SHOW_PLAYLIST), &QAction::setChecked);
    connect(m_plButton, &SkinnedToggleButton::clicked, m_playlist, &QWidget::setVisible);
    connect(m_playlist, &SkinnedPlayList::closed, m_plButton, [this] {
        m_plButton->setChecked(false);
        m_playlist->setVisible(false);
    });
}

bool SkinnedDisplay::isPlaylistVisible() const
{
    return m_plButton->isChecked();
}

bool SkinnedDisplay::isEqualizerVisible() const
{
    return m_eqButton->isChecked();
}

void SkinnedDisplay::displayVolume()
{
    if(sender() == m_volumeBar)
        m_text->setText(tr("Volume: %1%").arg(m_volumeBar->value()));
    if(sender() == m_balanceBar)
    {
        if(m_balanceBar->value() > 0)
            m_text->setText(tr("Balance: %1% right").arg(m_balanceBar->value()));
        else if(m_balanceBar->value() < 0)
            m_text->setText(tr("Balance: %1% left").arg(-m_balanceBar->value()));
        else
            m_text->setText(tr("Balance: center"));
    }
}

void SkinnedDisplay::showPosition()
{
    m_text->setText(tr("Seek to: %1").arg(MetaDataFormatter::formatDuration(m_posbar->value(), false)));
}

void SkinnedDisplay::updatePosition()
{
    m_text->clear();
    m_core->seek(m_posbar->value());
}

void SkinnedDisplay::wheelEvent(QWheelEvent *e)
{
    m_core->changeVolume(e->angleDelta().y() * QmmpSettings::instance()->volumeStep() / 120);
    m_text->setText(tr("Volume: %1%").arg(m_core->volume()));
    m_volumeDisplayTimer->start();
}

bool SkinnedDisplay::isRepeatable() const
{
    return m_repeatButton->isChecked();
}

bool SkinnedDisplay::isShuffle() const
{
    return m_shuffleButton->isChecked();
}

void SkinnedDisplay::setIsRepeatable(bool yes)
{
    m_repeatButton->setChecked(yes);
}

void SkinnedDisplay::setIsShuffle(bool yes)
{
    m_shuffleButton->setChecked(yes);
}

void SkinnedDisplay::mousePressEvent(QMouseEvent *e)
{
    if(e->button() == Qt::RightButton)
        m_mw->menu()->exec(e->globalPosition().toPoint());
    else if(e->button() == Qt::LeftButton && m_aboutWidget->underMouse())
        m_mw->about();
    PixmapWidget::mousePressEvent(e);
}
