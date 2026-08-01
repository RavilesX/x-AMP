/***************************************************************************
 *   Copyright (C) 2011-2026 by Ilya Kotov                                 *
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

#include <QGridLayout>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSlider>
#include <QCheckBox>
#include <QPushButton>
#include <QComboBox>
#include <QLabel>
#include <QPainter>
#include <QDialogButtonBox>
#include <QSettings>
#include <QMessageBox>
#include <QDir>
#include <qmmp/qmmpsettings.h>
#include "qsuiequalizer.h"

QSUiEqualizer::QSUiEqualizer(QWidget *parent) : QDialog(parent)
{
    setWindowTitle(tr("Equalizer"));

    m_layout = new QVBoxLayout(this);
    setLayout(m_layout);
    m_layout->setSpacing(5);
    m_layout->setContentsMargins(5,5,5,5);

    QHBoxLayout *buttonsLayout = new QHBoxLayout;
    buttonsLayout->setSpacing(5);
    m_enableCheckBox = new QCheckBox(tr("Enable equalizer"), this);
    buttonsLayout->addWidget(m_enableCheckBox);

    buttonsLayout->addSpacerItem(new QSpacerItem(30, 0, QSizePolicy::Fixed, QSizePolicy::Fixed));

    QLabel *label = new QLabel(this);
    label->setText(tr("Preset:"));
    buttonsLayout->addWidget(label);

    m_presetComboBox = new QComboBox(this);
    m_presetComboBox->setEditable(true);
    connect(m_presetComboBox, &QComboBox::activated, this, &QSUiEqualizer::loadPreset);
    buttonsLayout->addWidget(m_presetComboBox);

    QPushButton *saveButton = new QPushButton(tr("Save"), this);
    connect(saveButton, &QPushButton::pressed, this, &QSUiEqualizer::savePreset);
    buttonsLayout->addWidget(saveButton);

    QPushButton *deleteButton = new QPushButton(tr("Delete"), this);
    connect(deleteButton, &QPushButton::pressed, this, &QSUiEqualizer::deletePreset);
    buttonsLayout->addWidget(deleteButton);

    QPushButton *reset = new QPushButton(tr("Reset"), this);
    connect(reset, &QPushButton::pressed, this, &QSUiEqualizer::resetSettings);
    buttonsLayout->addWidget(reset);

    QDialogButtonBox *dialogButtons = new QDialogButtonBox(QDialogButtonBox::Close, Qt::Horizontal, this);
    dialogButtons->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    connect(dialogButtons, &QDialogButtonBox::rejected, this, &QSUiEqualizer::reject);
    buttonsLayout->addWidget(dialogButtons);

    QGridLayout *slidersLayout = new QGridLayout;
    slidersLayout->setSpacing(5);

    const QStringList names = {
        tr("Preamp"), u"25"_s, u"40"_s, u"63"_s, u"100"_s, u"160"_s, u"250"_s, u"400"_s,
        u"630"_s, u"1k"_s, u"1,6k"_s, u"2,5k"_s, u"4k"_s, u"6,3k"_s, u"10k"_s, u"16k"_s
    };

    for(int i = 0; i < 16; ++i)
    {
        QSlider *slider = new QSlider(this);
        slider->setRange(-20, 20);
        slider->setTickPosition(QSlider::TicksLeft);
        slider->setTickInterval(10);
        QLabel *eqLabel = new QLabel(this);
        eqLabel->setFrameShape(QFrame::Box);
        eqLabel->setText(names.at(i));
        slidersLayout->addWidget(slider, 1, i, Qt::AlignHCenter);
        slidersLayout->addWidget(eqLabel, 2, i, Qt::AlignHCenter);

        QLabel *label2 = new QLabel(this);
        label2->setText(tr("%1dB").arg(0));
        label2->setFrameShape(QFrame::Box);
        label2->setMinimumWidth(fontMetrics().horizontalAdvance(tr("+%1dB").arg(20)) + 5);
        label2->setAlignment(Qt::AlignCenter);
        slidersLayout->addWidget(label2, 0, i, Qt::AlignHCenter);
        connect(slider, &QSlider::valueChanged, this, &QSUiEqualizer::updateLabel);
        m_sliders << slider;
        m_labels << label2;
    }
    m_layout->addItem(slidersLayout);
    m_layout->addItem(buttonsLayout);
    setMinimumHeight(300);
    for(const QSlider *slider : std::as_const(m_sliders))
    {
        connect(slider, &QSlider::valueChanged, this, &QSUiEqualizer::applySettings);
    }
    connect(m_enableCheckBox, &QCheckBox::clicked, this, &QSUiEqualizer::applySettings);
    readSettigs();
    loadPresets();
}

QSUiEqualizer::~QSUiEqualizer()
{
    savePresets();
}

void QSUiEqualizer::readSettigs()
{
    EqSettings settings = QmmpSettings::instance()->eqSettings();
    m_enableCheckBox->setChecked(settings.isEnabled());
    m_sliders.at(0)->setValue(settings.preamp());
    for(int i = 0; i < EqSettings::EQ_BANDS_15; ++i)
    {
        m_sliders.at(i + 1)->setValue(settings.gain(i));
    }
}

void QSUiEqualizer::loadPresets()
{
    m_presetComboBox->clear();
    //equalizer presets
    QString preset_path = Qmmp::configDir() + u"/eq15.preset"_s;
    if(!QFile::exists(preset_path))
        preset_path = u":/qsui/eq15.preset"_s;
    QSettings eq_preset (preset_path, QSettings::IniFormat);
    int i = 0;
    while(eq_preset.contains(u"Presets/Preset"_s + QString::number(++i)))
    {
        QString name = eq_preset.value(QStringLiteral("Presets/Preset%1").arg(i), tr("preset")).toString();
        EqSettings preset(EqSettings::EQ_BANDS_15);
        //preset->setText(name);
        eq_preset.beginGroup(name);
        for (int j = 0; j < EqSettings::EQ_BANDS_15; ++j)
        {
            preset.setGain(j,eq_preset.value(QStringLiteral("Band%1").arg(j), 0).toDouble());
        }
        preset.setPreamp(eq_preset.value(u"Preamp"_s, 0).toDouble());
        m_presets.append(preset);
        m_presetComboBox->addItem(name);
        eq_preset.endGroup();
    }
    m_presetComboBox->clearEditText();
}

void QSUiEqualizer::applySettings()
{
    EqSettings settings = QmmpSettings::instance()->eqSettings();
    settings.setPreamp(m_sliders.constFirst()->value());
    settings.setEnabled(m_enableCheckBox->isChecked());
    for(int i = 0; i < EqSettings::EQ_BANDS_15; ++i)
    {
        settings.setGain(i, m_sliders.at(i + 1)->value());
    }
    QmmpSettings::instance()->setEqSettings(settings);
}

void QSUiEqualizer::resetSettings()
{
    for(QSlider *slider : std::as_const(m_sliders))
    {
        slider->setValue(0);
    }
    applySettings();
    m_presetComboBox->setCurrentIndex(-1);
}

void QSUiEqualizer::updateLabel()
{
    QSlider *slider = qobject_cast<QSlider *>(sender());
    int index = m_sliders.indexOf(slider);
    if(index < 0)
        return;
    if(slider->value() > 0)
        m_labels[index]->setText(tr("+%1dB").arg(slider->value()));
    else
        m_labels[index]->setText(tr("%1dB").arg(slider->value()));
}

void QSUiEqualizer::loadPreset(int index)
{
    EqSettings preset = m_presets.at(index);
    m_sliders.at(0)->setValue(preset.preamp());
    for(int i = 0; i < EqSettings::EQ_BANDS_15; ++i)
    {
        m_sliders.at(i+1)->setValue(preset.gain(i));
    }
    applySettings();
}

void QSUiEqualizer::savePreset()
{
    QString name = m_presetComboBox->currentText();
    if(name.isEmpty())
        return;

    int index = m_presetComboBox->findText(name);
    if(index != -1)
    {
        if(QMessageBox::question(this, tr("Overwrite Request"),
                                 tr("Preset '%1' already exists. Overwrite?").arg(name),
                                 QMessageBox::Yes | QMessageBox::No) == QMessageBox::No)
            return;
        m_presets[index].setPreamp(m_sliders.at(0)->value());
        for(int i = 0; i < EqSettings::EQ_BANDS_15; ++i)
        {
            m_presets[index].setGain(i, m_sliders.at(i+1)->value());
        }
    }
    else
    {
        m_presetComboBox->addItem(name);
        EqSettings preset(EqSettings::EQ_BANDS_15);
        preset.setPreamp(m_sliders.at(0)->value());
        for(int i = 0; i < EqSettings::EQ_BANDS_15; ++i)
        {
            preset.setGain(i, m_sliders.at(i + 1)->value());
        }
        m_presets.append(preset);
    }
    m_presetComboBox->clearEditText();
}

void QSUiEqualizer::savePresets()
{
    QSettings eq_preset(Qmmp::configDir() + u"/eq15.preset"_s, QSettings::IniFormat);
    eq_preset.clear ();
    for (int i = 0; i < m_presets.size(); ++i)
    {
        eq_preset.setValue(QStringLiteral("Presets/Preset%1").arg(i + 1), m_presetComboBox->itemText(i));
        eq_preset.beginGroup(m_presetComboBox->itemText(i));
        for (int j = 0; j < EqSettings::EQ_BANDS_15; ++j)
        {
            eq_preset.setValue(QStringLiteral("Band%1").arg(j),m_presets.at(i).gain(j));
        }
        eq_preset.setValue(u"Preamp"_s, m_presets.at(i).preamp());
        eq_preset.endGroup();
    }
}

void QSUiEqualizer::deletePreset()
{
    QString name = m_presetComboBox->currentText();
    if(name.isEmpty())
        return;

    int index = m_presetComboBox->findText(name);
    if(index != -1)
    {
        m_presetComboBox->removeItem(index);
    }
    m_presetComboBox->clearEditText();
}
