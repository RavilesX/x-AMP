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

#include <QFile>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QLabel>
#include <QMenu>
#include <QPainter>
#include <QSettings>
#include <QVBoxLayout>
#include <qmmp/qmmp.h>
#include <qmmp/qmmpsettings.h>
#include <qmmp/soundcore.h>
#include "xuitheme.h"
#include "xuicontrols.h"
#include "xuiequalizercard.h"

namespace
{
    //The frequencies this interface labels; EqSettings::EQ_BANDS_10 is
    //requested in the factory to match.
    const QStringList BAND_LABELS = {
        QStringLiteral("60"),  QStringLiteral("170"), QStringLiteral("310"),
        QStringLiteral("600"), QStringLiteral("1k"),  QStringLiteral("3k"),
        QStringLiteral("6k"),  QStringLiteral("12k"), QStringLiteral("14k"),
        QStringLiteral("16k")
    };

    constexpr double RANGE_DB = 12.0;

    QLabel *makeLabel(const QString &text, const QColor &color, qreal scale)
    {
        QLabel *label = new QLabel(text);
        QFont f = label->font();
        f.setPointSizeF(f.pointSizeF() * scale);
        label->setFont(f);
        QPalette pal = label->palette();
        pal.setColor(QPalette::WindowText, color);
        label->setPalette(pal);
        return label;
    }

    //Shared with the skinned interface on purpose, so presets carry across.
    QString presetPath()
    {
        return Qmmp::configDir() + QStringLiteral("/eq.preset");
    }

    //Auto presets are keyed by file name and live in their own file, again
    //the same one the skinned interface uses.
    QString autoPresetPath()
    {
        return Qmmp::configDir() + QStringLiteral("/eq.auto_preset");
    }

    //Both files use the same layout: a Presets group indexing named groups.
    void readPresets(const QString &path, QList<EqSettings> *presets, QStringList *names)
    {
        presets->clear();
        names->clear();
        if(!QFile::exists(path))
            return;
        QSettings file(path, QSettings::IniFormat);
        int i = 0;
        while(file.contains(QStringLiteral("Presets/Preset%1").arg(++i)))
        {
            const QString name = file.value(QStringLiteral("Presets/Preset%1").arg(i)).toString();
            EqSettings preset(EqSettings::EQ_BANDS_10);
            file.beginGroup(name);
            for(int band = 0; band < EqSettings::EQ_BANDS_10; ++band)
                preset.setGain(band, file.value(QStringLiteral("Band%1").arg(band), 0).toDouble());
            preset.setPreamp(file.value(QStringLiteral("Preamp"), 0).toDouble());
            file.endGroup();
            presets->append(preset);
            names->append(name);
        }
    }

    void writePreset(const QString &path, const QString &name, int slot,
                     const EqSettings &preset)
    {
        QSettings file(path, QSettings::IniFormat);
        file.setValue(QStringLiteral("Presets/Preset%1").arg(slot + 1), name);
        file.beginGroup(name);
        for(int i = 0; i < EqSettings::EQ_BANDS_10; ++i)
            file.setValue(QStringLiteral("Band%1").arg(i), preset.gain(i));
        file.setValue(QStringLiteral("Preamp"), preset.preamp());
        file.endGroup();
        file.sync();
    }
}

XUiEqualizerCard::XUiEqualizerCard(QWidget *parent) : QWidget(parent)
{
    m_settings = QmmpSettings::instance();
    m_core = SoundCore::instance();

    QVBoxLayout *root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);
    root->addWidget(buildHeader());
    root->addWidget(buildBands(), 1);

    loadPresets();
    reloadFromSettings();
    connect(m_settings, &QmmpSettings::eqSettingsChanged,
            this, &XUiEqualizerCard::reloadFromSettings);
    connect(m_core, &SoundCore::trackInfoChanged,
            this, &XUiEqualizerCard::applyAutoPreset);
}

QWidget *XUiEqualizerCard::buildHeader()
{
    QWidget *header = new QWidget(this);
    header->setFixedHeight(XUi::CardHeaderHeight);
    QHBoxLayout *layout = new QHBoxLayout(header);
    layout->setContentsMargins(XUi::CardPadding, 0, XUi::CardPadding, 0);
    layout->setSpacing(12);

    QLabel *title = makeLabel(tr("EQUALIZER"), XUi::Text, 1.05);
    QFont f = title->font();
    f.setBold(true);
    f.setLetterSpacing(QFont::AbsoluteSpacing, 1.4);
    title->setFont(f);

    m_enabled = new XUiToggle(header);
    connect(m_enabled, &XUiToggle::toggled, this, &XUiEqualizerCard::applySettings);

    QLabel *onLabel = makeLabel(tr("ON"), XUi::TextDim, 0.9);

    m_auto = new XUiToggle(header);
    m_auto->setToolTip(tr("Load a preset saved for each track"));
    connect(m_auto, &XUiToggle::toggled, this, [this](bool on) {
        QSettings().setValue(QStringLiteral("XUi/eq_auto"), on);
        if(on)
            applyAutoPreset();
    });
    QLabel *autoLabel = makeLabel(tr("AUTO"), XUi::TextDim, 0.9);

    m_presetButton = new XUiMenuButton(tr("Presets"), header);
    connect(m_presetButton, &XUiMenuButton::clicked, this, &XUiEqualizerCard::showPresetMenu);

    XUiIconButton *reset = new XUiIconButton(XUiIcons::Settings, header);
    reset->setToolTip(tr("Reset all bands"));
    connect(reset, &XUiIconButton::clicked, this, &XUiEqualizerCard::setFlat);

    layout->addWidget(title);
    layout->addSpacing(8);
    layout->addWidget(m_enabled);
    layout->addWidget(onLabel);
    layout->addSpacing(10);
    layout->addWidget(m_auto);
    layout->addWidget(autoLabel);
    layout->addStretch(1);
    layout->addWidget(m_presetButton);
    layout->addWidget(reset);
    return header;
}

QWidget *XUiEqualizerCard::buildBands()
{
    QWidget *panel = new QWidget(this);
    QHBoxLayout *layout = new QHBoxLayout(panel);
    layout->setContentsMargins(XUi::CardPadding, 4, XUi::CardPadding, XUi::CardPadding);
    layout->setSpacing(6);

    //dB scale down the left edge
    QVBoxLayout *scale = new QVBoxLayout;
    scale->setContentsMargins(0, 0, 6, 22);
    scale->setSpacing(0);
    scale->addWidget(makeLabel(tr("+%1 dB").arg(int(RANGE_DB)), XUi::TextFaint, 0.85),
                     0, Qt::AlignRight | Qt::AlignTop);
    scale->addStretch(1);
    scale->addWidget(makeLabel(tr("0 dB"), XUi::TextFaint, 0.85), 0, Qt::AlignRight);
    scale->addStretch(1);
    scale->addWidget(makeLabel(tr("-%1 dB").arg(int(RANGE_DB)), XUi::TextFaint, 0.85),
                     0, Qt::AlignRight | Qt::AlignBottom);
    layout->addLayout(scale);

    auto addSlider = [&](XUiEqSlider *slider, const QString &caption, bool dim) {
        QVBoxLayout *column = new QVBoxLayout;
        column->setSpacing(6);
        slider->setRange(-RANGE_DB, RANGE_DB);
        connect(slider, &XUiEqSlider::moved, this, &XUiEqualizerCard::applySettings);
        column->addWidget(slider, 1, Qt::AlignHCenter);
        column->addWidget(makeLabel(caption, dim ? XUi::TextDim : XUi::TextFaint, 0.85),
                          0, Qt::AlignHCenter);
        layout->addLayout(column);
    };

    m_preamp = new XUiEqSlider(panel);
    addSlider(m_preamp, tr("Preamp"), true);
    layout->addSpacing(10);

    for(const QString &label : BAND_LABELS)
    {
        XUiEqSlider *slider = new XUiEqSlider(panel);
        m_bands.append(slider);
        addSlider(slider, label, false);
    }
    return panel;
}

void XUiEqualizerCard::reloadFromSettings()
{
    const EqSettings eq = m_settings->eqSettings();
    m_enabled->setChecked(eq.isEnabled());
    m_preamp->setValue(eq.preamp());
    for(int i = 0; i < m_bands.size() && i < eq.bands(); ++i)
        m_bands.at(i)->setValue(eq.gain(i));
}

void XUiEqualizerCard::applySettings()
{
    EqSettings eq = m_settings->eqSettings();
    eq.setEnabled(m_enabled->isChecked());
    eq.setPreamp(m_preamp->value());
    for(int i = 0; i < m_bands.size() && i < eq.bands(); ++i)
        eq.setGain(i, m_bands.at(i)->value());
    m_settings->setEqSettings(eq);
}

void XUiEqualizerCard::loadPresets()
{
    readPresets(presetPath(), &m_presets, &m_presetNames);
    readPresets(autoPresetPath(), &m_autoPresets, &m_autoPresetNames);
    m_auto->setChecked(QSettings().value(QStringLiteral("XUi/eq_auto"), false).toBool());
}

QString XUiEqualizerCard::currentTrackKey() const
{
    //the file name, matching what the skinned interface keys auto presets on
    const QString path = m_core->trackInfo().path();
    return path.section(QLatin1Char('/'), -1);
}

void XUiEqualizerCard::applyAutoPreset()
{
    if(!m_auto->isChecked())
        return;
    const int index = m_autoPresetNames.indexOf(currentTrackKey());
    if(index >= 0)
        applyPreset(m_autoPresets.at(index), m_autoPresetNames.at(index));
    else
        setFlat(); //no preset for this track: back to flat, not the last one
}

void XUiEqualizerCard::setFlat()
{
    m_preamp->setValue(0.0);
    for(XUiEqSlider *slider : std::as_const(m_bands))
        slider->setValue(0.0);
    m_presetButton->setText(tr("Presets"));
    applySettings();
}

void XUiEqualizerCard::saveAutoPreset()
{
    const QString key = currentTrackKey();
    if(key.isEmpty())
        return;

    EqSettings preset(EqSettings::EQ_BANDS_10);
    preset.setPreamp(m_preamp->value());
    for(int i = 0; i < m_bands.size(); ++i)
        preset.setGain(i, m_bands.at(i)->value());

    int slot = m_autoPresetNames.indexOf(key);
    if(slot < 0)
        slot = m_autoPresetNames.size();
    writePreset(autoPresetPath(), key, slot, preset);
    loadPresets();
    m_auto->setChecked(true); //saving one implies wanting it applied
}

void XUiEqualizerCard::applyPreset(const EqSettings &preset, const QString &name)
{
    m_preamp->setValue(preset.preamp());
    for(int i = 0; i < m_bands.size(); ++i)
        m_bands.at(i)->setValue(preset.gain(i));
    m_presetButton->setText(name);
    applySettings();
}

void XUiEqualizerCard::savePreset()
{
    bool accepted = false;
    const QString name = QInputDialog::getText(this, tr("Save Preset"), tr("Preset name:"),
                                               QLineEdit::Normal, QString(), &accepted);
    if(!accepted || name.isEmpty())
        return;

    EqSettings preset(EqSettings::EQ_BANDS_10);
    preset.setPreamp(m_preamp->value());
    for(int i = 0; i < m_bands.size(); ++i)
        preset.setGain(i, m_bands.at(i)->value());

    //replacing an existing name keeps its slot rather than adding a duplicate
    int slot = m_presetNames.indexOf(name);
    if(slot < 0)
        slot = m_presetNames.size();
    writePreset(presetPath(), name, slot, preset);

    loadPresets();
    m_presetButton->setText(name);
}

void XUiEqualizerCard::showPresetMenu()
{
    QMenu menu(this);
    for(int i = 0; i < m_presetNames.size(); ++i)
    {
        QAction *action = menu.addAction(m_presetNames.at(i));
        connect(action, &QAction::triggered, this, [this, i] {
            applyPreset(m_presets.at(i), m_presetNames.at(i));
        });
    }
    if(!m_presetNames.isEmpty())
        menu.addSeparator();
    connect(menu.addAction(tr("Save as...")), &QAction::triggered,
            this, &XUiEqualizerCard::savePreset);
    QAction *forTrack = menu.addAction(tr("Save for This Track"));
    forTrack->setEnabled(!currentTrackKey().isEmpty());
    connect(forTrack, &QAction::triggered, this, &XUiEqualizerCard::saveAutoPreset);
    menu.exec(m_presetButton->mapToGlobal(QPoint(0, m_presetButton->height() + 2)));
}

void XUiEqualizerCard::paintEvent(QPaintEvent *)
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

    //separator under the header, as in the design
    p.setPen(QPen(XUi::Border, 1));
    p.drawLine(QPointF(1, XUi::CardHeaderHeight), QPointF(width() - 1, XUi::CardHeaderHeight));
}
