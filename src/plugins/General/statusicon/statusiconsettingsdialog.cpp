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
#include <qmmpui/templateeditor.h>
#include <qmmp/qmmp.h>
#include "statusiconpopupwidget.h"
#include "statusicon.h"
#include "ui_statusiconsettingsdialog.h"
#include "statusiconsettingsdialog.h"

StatusIconSettingsDialog::StatusIconSettingsDialog(QWidget *parent)
        : QDialog(parent), m_ui(new Ui::StatusIconSettingsDialog)
{
    m_ui->setupUi(this);
    connect(m_ui->transparencySlider, &QSlider::valueChanged, m_ui->niceTooltipOpacityValueLabel, qOverload<int>(&QLabel::setNum));
    connect(m_ui->coverSizeSlider, &QSlider::valueChanged, m_ui->niceTooltipOpacityValueLabel_2, qOverload<int>(&QLabel::setNum));
    QSettings settings;
    settings.beginGroup(u"Tray"_s);
    m_ui->messageGroupBox->setChecked(settings.value(u"show_message"_s, false).toBool());
    m_ui->messageDelaySpinBox->setValue(settings.value(u"message_delay"_s, 2000).toInt());
    m_ui->niceTooltipGroupBox->setChecked(settings.value(u"show_tooltip"_s, true).toBool());
#ifdef QMMP_WS_X11
    m_ui->niceTooltipDelaySpinBox->setValue(settings.value(u"tooltip_delay"_s, 2000).toInt());
    m_ui->transparencySlider->setValue(settings.value(u"tooltip_transparency"_s, 0).toInt());
    m_ui->coverSizeSlider->setValue(settings.value(u"tooltip_cover_size"_s, 100).toInt());
    m_ui->progressCheckBox->setChecked(settings.value(u"tooltip_progress"_s, true).toBool());
    m_ui->niceTooltipSplitCheckBox->setChecked(settings.value(u"split_file_name"_s, true).toBool());
#else
    m_ui->niceTooltipDelaySpinBox->setEnabled(false);
    m_ui->transparencySlider->setEnabled(false);
    m_ui->coverSizeSlider->setEnabled(false);
    m_ui->progressCheckBox->setEnabled(false);
    m_ui->templateButton->setEnabled(false);
#endif
    m_ui->standardIconsCheckBox->setChecked(settings.value(u"use_standard_icons"_s, false).toBool());
    m_template = settings.value(u"tooltip_template"_s, DEFAULT_TEMPLATE).toString();
    settings.endGroup();
}

StatusIconSettingsDialog::~StatusIconSettingsDialog()
{
    delete m_ui;
}

void StatusIconSettingsDialog::accept()
{
    QSettings settings;
    settings.beginGroup(u"Tray"_s);
    settings.setValue(u"show_message"_s, m_ui->messageGroupBox->isChecked());
    settings.setValue(u"message_delay"_s, m_ui->messageDelaySpinBox->value());
    settings.setValue(u"use_standard_icons"_s, m_ui->standardIconsCheckBox->isChecked());
    settings.setValue(u"show_tooltip"_s, m_ui->niceTooltipGroupBox->isChecked());
    settings.setValue(u"split_file_name"_s, m_ui->niceTooltipSplitCheckBox->isChecked());
#ifdef QMMP_WS_X11
    settings.setValue(u"tooltip_delay"_s, m_ui->niceTooltipDelaySpinBox->value());
    settings.setValue(u"tooltip_transparency"_s,  m_ui->transparencySlider->value());
    settings.setValue(u"tooltip_cover_size"_s,  m_ui->coverSizeSlider->value());
    settings.setValue(u"tooltip_progress"_s, m_ui->progressCheckBox->isChecked());
#endif
    settings.setValue(u"tooltip_template"_s, m_template);
    settings.endGroup();
    QDialog::accept();
}

void StatusIconSettingsDialog::on_templateButton_clicked()
{
    QString t = TemplateEditor::getTemplate(this, tr("Tooltip Template"), m_template, DEFAULT_TEMPLATE);
    if(!t.isEmpty())
        m_template = t;
}
