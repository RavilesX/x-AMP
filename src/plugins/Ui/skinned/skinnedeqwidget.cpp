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
#include <QSettings>
#include <QEvent>
#include <QMenu>
#include <QInputDialog>
#include <QCloseEvent>
#include <QFile>
#include <QScreen>
#include <QApplication>
#include <algorithm>
#include <qmmpui/filedialog.h>
#include <qmmpui/playlistmanager.h>
#include <qmmp/soundcore.h>
#include "skin.h"
#include "skinnedeqslider.h"
#include "skinnedeqtitlebar.h"
#include "skinnedtogglebutton.h"
#include "skinnedeqgraph.h"
#include "skinnedbutton.h"
#include "skinnedpreseteditor.h"
#include "skinnedmainwindow.h"
#include "skinnedplaylist.h"
#include "windowsystem.h"
#include "skinnedeqwidget.h"

SkinnedEqWidget::SkinnedEqWidget (QWidget *parent)
        : PixmapWidget (parent)
{
    setWindowTitle(tr("Equalizer"));
    setPixmap(skin()->getEqPart(Skin::EQ_MAIN), true);
    setCursor(skin()->getCursor(Skin::CUR_EQNORMAL));
    m_titleBar = new SkinnedEqTitleBar (this);
    m_titleBar->move (0,0);

    m_preamp = new SkinnedEqSlider(this);
    connect(m_preamp, &SkinnedEqSlider::sliderMoved, this, &SkinnedEqWidget::writeEq);

    m_on = new SkinnedToggleButton (this,Skin::EQ_BT_ON_N,Skin::EQ_BT_ON_P, Skin::EQ_BT_OFF_N, Skin::EQ_BT_OFF_P);
    connect (m_on, &SkinnedToggleButton::clicked, this,  &SkinnedEqWidget::writeEq);

    m_autoButton = new SkinnedToggleButton(this, Skin::EQ_BT_AUTO_1_N, Skin::EQ_BT_AUTO_1_P,
                                    Skin::EQ_BT_AUTO_0_N, Skin::EQ_BT_AUTO_0_P);
    m_eqg = new SkinnedEqGraph(this);
    m_presetsMenu = new QMenu(this);
    m_presetButton = new SkinnedButton (this, Skin::EQ_BT_PRESETS_N, Skin::EQ_BT_PRESETS_P, Skin::CUR_EQNORMAL);
    connect(m_presetButton, &SkinnedButton::clicked, this, &SkinnedEqWidget::showPresetsMenu);
    connect(SoundCore::instance(), &SoundCore::eqSettingsChanged, this, &SkinnedEqWidget::readEq);

    for (int i = 0; i < 10; ++i)
    {
        m_sliders << new SkinnedEqSlider (this);
        connect(m_sliders.at(i), &SkinnedEqSlider::sliderMoved, this, &SkinnedEqWidget::writeEq);
    }
    readSettings();
    createActions();
    updatePositions();
    updateMask();
#ifdef QMMP_WS_X11
    QString wm_name = WindowSystem::netWindowManagerName();
    if(wm_name.contains(u"openbox"_s, Qt::CaseInsensitive) || wm_name.contains(u"xfwm4"_s, Qt::CaseInsensitive))
        setWindowFlags (Qt::Drawer | Qt::FramelessWindowHint);
    else if(wm_name.contains(u"metacity"_s, Qt::CaseInsensitive))
        setWindowFlags (Qt::Tool | Qt::FramelessWindowHint);
    else
#endif
        setWindowFlags (Qt::Dialog | Qt::FramelessWindowHint);
}

void SkinnedEqWidget::updatePositions()
{
    m_preamp->move(skin()->scaled(21), skin()->scaled(38));
    m_on->move(skin()->scaled(14), skin()->scaled(18));
    m_autoButton->move(skin()->scaled(39), skin()->scaled(18));
    m_eqg->move(skin()->scaled(87), skin()->scaled(17));
    m_presetButton->move(skin()->scaled(217), skin()->scaled(18));
     for (int i = 0; i < 10; ++i)
         m_sliders.at (i)->move(skin()->scaled(78 + i * 18), skin()->scaled(38));
}

void SkinnedEqWidget::changeEvent (QEvent * event)
{
    if(event->type() == QEvent::ActivationChange)
    {
        m_titleBar->setActive(isActiveWindow());
    }
}

void SkinnedEqWidget::closeEvent (QCloseEvent* e)
{
    if(e->spontaneous ())
        parentWidget()->close();
}

void SkinnedEqWidget::updateSkin()
{
    m_titleBar->setActive (false);
    setPixmap(skin()->getEqPart(Skin::EQ_MAIN), true);
    setCursor(skin()->getCursor(Skin::CUR_EQNORMAL));
    setMimimalMode(m_shaded);
    updatePositions();
}

void SkinnedEqWidget::setMimimalMode(bool b)
{
    m_shaded = b;

    if(m_shaded)
        setFixedSize(skin()->scaled(275), skin()->scaled(14));
    else
        setFixedSize(skin()->scaled(275), skin()->scaled(116));

    updateMask();
}

void SkinnedEqWidget::readSettings()
{
    QSettings settings;
    QScreen *primaryScreen = QGuiApplication::primaryScreen();
    QRect availableGeometry = primaryScreen->availableGeometry();
    QPoint pos = settings.value("Skinned/eq_pos"_L1, QPoint(100, 216)).toPoint();
    //TODO QGuiApplication::screenAt
    const QList<QScreen *> screens = QGuiApplication::screens();
    auto it = std::find_if(screens.cbegin(), screens.cend(), [pos](QScreen *screen){ return screen->availableGeometry().contains(pos); });
    if(it != screens.cend())
        availableGeometry = (*it)->availableGeometry();
    pos.setX(qBound(availableGeometry.left(), pos.x(), availableGeometry.right() - skin()->scaled(275)));
    pos.setY(qBound(availableGeometry.top(), pos.y(), availableGeometry.bottom() - skin()->scaled(116)));
    move(pos); //geometry
    readEq();
    m_autoButton->setChecked(settings.value("Skinned/eq_auto"_L1, false).toBool());
    //equalizer presets
    QString preset_path = Qmmp::configDir() + u"/eq.preset"_s;
    if(!QFile::exists(preset_path))
        preset_path = u":/skinned/eq.preset"_s;
    QSettings eq_preset (preset_path, QSettings::IniFormat);
    int i = 0;
    while(eq_preset.contains(QStringLiteral("Presets/Preset%1").arg(++i)))
    {
        QString name = eq_preset.value(QStringLiteral("Presets/Preset%1").arg(i), tr("preset")).toString();
        EqSettings preset(EqSettings::EQ_BANDS_10);
        eq_preset.beginGroup(name);
        for(int j = 0; j < 10; ++j)
        {
            preset.setGain(j,eq_preset.value(QStringLiteral("Band%1").arg(j), 0).toDouble());
        }
        preset.setPreamp(eq_preset.value("Preamp"_L1, 0).toDouble());
        m_presets.append(preset);
        m_presetNames.append(name);
        eq_preset.endGroup();
    }
    //equalizer auto-load presets
    QSettings eq_auto (Qmmp::configDir() + u"/eq.auto_preset"_s, QSettings::IniFormat);
    i = 0;
    while(eq_auto.contains(QStringLiteral("Presets/Preset%1").arg(++i)))
    {
        QString name = eq_auto.value(QStringLiteral("Presets/Preset%1").arg(i), tr("preset")).toString();
        EqSettings preset(EqSettings::EQ_BANDS_10);
        eq_auto.beginGroup(name);

        for(int j = 0; j < 10; ++j)
        {
            preset.setGain(j, eq_auto.value(QStringLiteral("Band%1").arg(j), 0).toDouble());
        }
        preset.setPreamp(eq_auto.value("Preamp"_L1, 0).toDouble());
        m_autoPresets.append(preset);
        m_autoPresetNames.append(name);
        eq_auto.endGroup();
    }
}

void SkinnedEqWidget::writeSettings()
{
    QSettings settings;
    settings.setValue("Skinned/eq_pos"_L1, this->pos()); //geometry
    settings.setValue("Skinned/eq_auto"_L1, m_autoButton->isChecked());
    //equalizer presets
    QSettings eq_preset(Qmmp::configDir() + u"/eq.preset"_s, QSettings::IniFormat);
    eq_preset.clear ();
    for (int i = 0; i < m_presets.size(); ++i)
    {
        eq_preset.setValue(QStringLiteral("Presets/Preset%1").arg(i + 1), m_presetNames.at(i));
        eq_preset.beginGroup(m_presetNames.at(i));
        for (int j = 0; j < 10; ++j)
        {
            eq_preset.setValue(QStringLiteral("Band%1").arg(j), m_presets.at(i).gain(j));
        }
        eq_preset.setValue("Preamp"_L1, m_presets.at(i).preamp());
        eq_preset.endGroup();
    }
    //equalizer auto-load presets
    QSettings eq_auto(Qmmp::configDir() + u"/eq.auto_preset"_s, QSettings::IniFormat);
    eq_auto.clear();
    for (int i = 0; i < m_autoPresets.size(); ++i)
    {
        eq_auto.setValue(QStringLiteral("Presets/Preset%1").arg(i + 1), m_autoPresetNames.at(i));
        eq_auto.beginGroup(m_autoPresetNames.at(i));
        for (int j = 0; j < 10; ++j)
        {
            eq_auto.setValue(QStringLiteral("Band%1").arg(j), m_autoPresets.at(i).gain(j));
        }
        eq_auto.setValue("Preamp"_L1, m_autoPresets.at(i).preamp());
        eq_auto.endGroup();
    }
}

void SkinnedEqWidget::readEq()
{
    m_eqg->clear();
    EqSettings eqSettings = SoundCore::instance()->eqSettings();
    if(eqSettings.bands() != 10)
    {
        m_on->setChecked(false);
        return;
    }
    m_preamp->setValue(eqSettings.preamp());
    for (int i=0; i<10; ++i)
    {
        m_sliders.at(i)->setValue(eqSettings.gain(i));
        m_eqg->addValue(m_sliders.at(i)->value());
    }
    m_on->setChecked(eqSettings.isEnabled());
}

void SkinnedEqWidget::writeEq()
{
    m_eqg->clear();
    EqSettings eqSettings = SoundCore::instance()->eqSettings();
    eqSettings.setPreamp(m_preamp->value());
    for (int i = 0; i < 10; ++i)
    {
        eqSettings.setGain(i, m_sliders.at(i)->value());
        m_eqg->addValue(m_sliders.at(i)->value());
    }
    eqSettings.setEnabled(m_on->isChecked());
    SoundCore::instance()->setEqSettings(eqSettings);
}

void SkinnedEqWidget::createActions()
{
    m_presetsMenu->addAction(tr("&Load/Delete"), this, &SkinnedEqWidget::showEditor);
    m_presetsMenu->addSeparator();
    m_presetsMenu->addAction(QIcon::fromTheme(u"document-save"_s), tr("&Save Preset"), this, &SkinnedEqWidget::savePreset);
    m_presetsMenu->addAction(QIcon::fromTheme(u"document-save"_s), tr("&Save Auto-load Preset"), this, &SkinnedEqWidget::saveAutoPreset);
    m_presetsMenu->addAction(QIcon::fromTheme(u"document-open"_s), tr("&Import"), this, &SkinnedEqWidget::importWinampEQF);
    m_presetsMenu->addSeparator();
    m_presetsMenu->addAction(QIcon::fromTheme(u"edit-clear"_s), tr("&Clear"), this, &SkinnedEqWidget::reset);
}

void SkinnedEqWidget::showPresetsMenu()
{
    m_presetsMenu->exec(m_presetButton->mapToGlobal(QPoint(0, 0)));
}

void SkinnedEqWidget::reset()
{
    for (int i = 0; i < m_sliders.size(); ++i)
        m_sliders.at(i)->setValue(0);
    m_preamp->setValue(0);
    writeEq();
}

void SkinnedEqWidget::showEditor()
{
    SkinnedPresetEditor *editor = new SkinnedPresetEditor(this);
    editor->addPresets(m_presetNames);
    editor->addAutoPresets(m_autoPresetNames);
    connect(editor, &SkinnedPresetEditor::presetLoaded, this, &SkinnedEqWidget::setPresetByName);
    connect(editor, &SkinnedPresetEditor::presetRemoved, this, &SkinnedEqWidget::removePresetByName);
    editor->show();
}

void SkinnedEqWidget::savePreset()
{
    bool ok = false;
    QString name = QInputDialog::getText(this, tr("Saving Preset"),
                                         tr("Preset name:"), QLineEdit::Normal,
                                         tr("preset #") + QString::number(m_presets.size() + 1), &ok);
    if(ok)
    {
        //remove presets with same name
        for(int i = m_presetNames.count() - 1; i >= 0; --i)
        {
            if(m_presetNames.at(i) == name)
            {
                m_presetNames.remove(i);
                m_presets.remove(i);
            }
        }

        //create new preset
        EqSettings preset(EqSettings::EQ_BANDS_10);
        preset.setPreamp(m_preamp->value());
        for (int i = 0; i < 10; ++i)
            preset.setGain(i, m_sliders.at (i)->value());

        m_presetNames.append(name);
        m_presets.append(preset);
    }
}

void SkinnedEqWidget::saveAutoPreset()
{
    PlayListTrack *track = PlayListManager::instance()->currentPlayList()->currentTrack();
    if (!track)
        return;

    QString name = track->path().section(QLatin1Char('/'), -1);
    //remove presets with same name
    for(int i = m_autoPresetNames.count() - 1; i >= 0; --i)
    {
        if(m_autoPresetNames.at(i) == name)
        {
            m_autoPresetNames.remove(i);
            m_autoPresets.remove(i);
        }
    }

    EqSettings preset(EqSettings::EQ_BANDS_10);
    preset.setPreamp(m_preamp->value());
    for (int i = 0; i < 10; ++i)
        preset.setGain(i, m_sliders.at (i)->value());

    m_autoPresetNames.append(name);
    m_autoPresets.append(preset);
}

void SkinnedEqWidget::setPresetByName(const QString &name, bool autoPreset)
{
    int index = autoPreset ? m_autoPresetNames.indexOf(name) : m_presetNames.indexOf(name);
    if(index >= 0)
        setPreset(index, autoPreset);
}

void SkinnedEqWidget::removePresetByName(const QString &name, bool autoPreset)
{
    if(autoPreset)
    {
        int index = m_autoPresetNames.indexOf(name);
        if(index >= 0)
        {
            m_autoPresets.remove(index);
            m_autoPresetNames.remove(index);
        }
    }
    else
    {
        int index = m_presetNames.indexOf(name);
        if(index >= 0)
        {
            m_presets.remove(index);
            m_presetNames.remove(index);
        }
    }
}

void SkinnedEqWidget::loadPreset(const QString &name)
{
    if(m_autoButton->isChecked())
    {
        int index = m_autoPresetNames.indexOf(name);

        if(index >= 0)
            setPreset(m_autoPresets.at(index));
        else
            reset();
    }
}

void SkinnedEqWidget::importWinampEQF()
{
    QString path = FileDialog::getOpenFileName(this, tr("Import Preset"), QDir::homePath(), u"Winamp EQF (*.q1)"_s);
    if(path.isEmpty())
        return;

    QFile file(path);
    if(!file.open(QIODevice::ReadOnly))
    {
        qCWarning(plugin) << "unable to open preset file; error:" << file.errorString();
        return;
    }

    char header[31];
    file.read(header, 31);
    if (QString::fromLatin1(header).contains(u"Winamp EQ library file v1.1"_s))
    {
        char name[257];
        char bands[11];
        while(file.read(name, 257))
        {
            EqSettings preset(EqSettings::EQ_BANDS_10);

            file.read(bands,11);

            for (int i = 0; i < 10; ++i)
            {
                preset.setGain(i, 20 - bands[i] * 40 / 64);
            }
            preset.setPreamp(20 - bands[10] * 40 / 64);
            m_presets.append(preset);
            m_presetNames.append(QString::fromLatin1(name));
        }
    }
    file.close();

}

void SkinnedEqWidget::keyPressEvent (QKeyEvent *ke)
{
    QKeyEvent event = QKeyEvent(ke->type(), ke->key(), ke->modifiers(), ke->text(), ke->isAutoRepeat(), ke->count());
    QApplication::sendEvent(qobject_cast<SkinnedMainWindow*>(parent())->playlist(), &event);
}

#ifdef QMMP_WS_X11
bool SkinnedEqWidget::event (QEvent *event)
{
    if(event->type() == QEvent::WinIdChange ||
            event->type() == QEvent::Show)
    {
        WindowSystem::ghostWindow(winId());
        WindowSystem::setWinHint(winId(), "equalizer", "xamp");
    }
    return QWidget::event(event);
}
#endif

void SkinnedEqWidget::updateMask()
{
    clearMask();
    setMask(QRegion(0,0,width(),height()));
    QRegion region = skin()->getRegion(m_shaded ? Skin::EQUALIZER_WS : Skin::EQUALIZER);
    if (!region.isEmpty())
        setMask(region);
}

void SkinnedEqWidget::setPreset(int index, bool autoPreset)
{
    EqSettings preset = autoPreset ? m_autoPresets.at(index) : m_presets.at(index);
    setPreset(preset);
}

void SkinnedEqWidget::setPreset(const EqSettings &preset)
{
    for (int i = 0; i < 10; ++i)
        m_sliders.at(i)->setValue(preset.gain(i));
    m_preamp->setValue(preset.preamp());
    writeEq();
}

