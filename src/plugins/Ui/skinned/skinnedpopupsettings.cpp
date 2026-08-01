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

#include <QSettings>
#include <qmmp/qmmp.h>
#include <qmmpui/metadataformattermenu.h>
#include "skinnedpopupwidget.h"
#include "ui_skinnedpopupsettings.h"
#include "skinnedpopupsettings.h"

SkinnedPopupSettings::SkinnedPopupSettings(QWidget *parent)
        : QDialog(parent), m_ui(new Ui::SkinnedPopupSettings)
{
    m_ui->setupUi(this);

    connect(m_ui->transparencySlider, &QSlider::valueChanged, m_ui->transparencyLabel, qOverload<int>(&QLabel::setNum));
    connect(m_ui->coverSizeSlider, &QSlider::valueChanged, m_ui->coverSizeLabel, qOverload<int>(&QLabel::setNum));

    QSettings settings;
    settings.beginGroup("Skinned"_L1);
    m_ui->transparencySlider->setValue(100 - settings.value("popup_opacity"_L1, 1.0).toDouble() * 100);
    m_ui->coverSizeSlider->setValue(settings.value ("popup_cover_size"_L1, 48).toInt());
    m_ui->textEdit->setPlainText(settings.value ("popup_template"_L1, DEFAULT_TEMPLATE).toString());
    m_ui->delaySpinBox->setValue(settings.value("popup_delay"_L1, 2500).toInt());
    m_ui->coverCheckBox->setChecked(settings.value("popup_show_cover"_L1, true).toBool());
    settings.endGroup();
    createMenu();
}


SkinnedPopupSettings::~SkinnedPopupSettings()
{
    delete m_ui;
}

void SkinnedPopupSettings::accept()
{
    QSettings settings;
    settings.beginGroup("Skinned"_L1);
    settings.setValue("popup_opacity"_L1, 1.0 -  (double)m_ui->transparencySlider->value()/100);
    settings.setValue("popup_cover_size"_L1, m_ui->coverSizeSlider->value());
    settings.setValue("popup_template"_L1, m_ui->textEdit->toPlainText());
    settings.setValue("popup_delay"_L1,  m_ui->delaySpinBox->value());
    settings.setValue("popup_show_cover"_L1, m_ui->coverCheckBox->isChecked());
    settings.endGroup();
    QDialog::accept();
}

void SkinnedPopupSettings::createMenu()
{
    MetaDataFormatterMenu *menu = new MetaDataFormatterMenu(MetaDataFormatterMenu::TITLE_MENU, this);
    m_ui->insertButton->setMenu(menu);
    connect(menu, &MetaDataFormatterMenu::patternSelected, m_ui->textEdit, &QPlainTextEdit::insertPlainText);
}

void SkinnedPopupSettings::on_resetButton_clicked()
{
    m_ui->textEdit->setPlainText(DEFAULT_TEMPLATE);
}
