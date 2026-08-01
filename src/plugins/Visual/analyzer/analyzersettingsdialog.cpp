/***************************************************************************
 *   Copyright (C) 2007-2026 by Ilya Kotov                                 *
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
#include <QSize>
#include <qmmp/qmmp.h>
#include "ui_analyzersettingsdialog.h"
#include "analyzersettingsdialog.h"

AnalyzerSettingsDialog::AnalyzerSettingsDialog(QWidget *parent) : QDialog(parent), m_ui(new Ui::AnalyzerSettingsDialog)
{
    m_ui->setupUi(this);
    QSettings settings;
    settings.beginGroup("Analyzer"_L1);
    m_ui->colorWidget1->setColor(settings.value("color1"_L1, u"Green"_s).toString());
    m_ui->colorWidget2->setColor(settings.value("color2"_L1, u"Yellow"_s).toString());
    m_ui->colorWidget3->setColor(settings.value("color3"_L1, u"Red"_s).toString());
    m_ui->bgColorWidget->setColor(settings.value("bg_color"_L1, u"Black"_s).toString());
    m_ui->peakColorWidget->setColor(settings.value("peak_color"_L1, u"Cyan"_s).toString());
    QSize cells_size = settings.value("cells_size"_L1, QSize(15, 6)).toSize();
    m_ui->cellWidthSpinBox->setValue(cells_size.width());
    m_ui->cellHeightSpinBox->setValue(cells_size.height());
    settings.endGroup();
}

AnalyzerSettingsDialog::~AnalyzerSettingsDialog()
{
    delete m_ui;
}

void AnalyzerSettingsDialog::accept()
{
    QSettings settings;
    settings.beginGroup("Analyzer"_L1);
    settings.setValue("color1"_L1, m_ui->colorWidget1->colorName());
    settings.setValue("color2"_L1, m_ui->colorWidget2->colorName());
    settings.setValue("color3"_L1, m_ui->colorWidget3->colorName());
    settings.setValue("bg_color"_L1, m_ui->bgColorWidget->colorName());
    settings.setValue("peak_color"_L1, m_ui->peakColorWidget->colorName());
    settings.setValue("cells_size"_L1, QSize(m_ui->cellWidthSpinBox->value(),
                                             m_ui->cellHeightSpinBox->value()));
    settings.endGroup();
    QDialog::accept();
}
