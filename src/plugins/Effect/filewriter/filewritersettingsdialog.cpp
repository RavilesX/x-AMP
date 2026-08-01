/***************************************************************************
 *   Copyright (C) 2017-2026 by Ilya Kotov                                 *
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
#include <QStandardPaths>
#include <qmmp/qmmp.h>
#include <qmmpui/filedialog.h>
#include <qmmpui/metadataformattermenu.h>
#include "ui_filewritersettingsdialog.h"
#include "filewritersettingsdialog.h"

FileWriterSettingsDialog::FileWriterSettingsDialog(QWidget *parent)
 : QDialog(parent), m_ui(new Ui::FileWriterSettingsDialog)
{
    m_ui->setupUi(this);

    MetaDataFormatterMenu *fileNameMenu = new MetaDataFormatterMenu(MetaDataFormatterMenu::TITLE_MENU, this);
    m_ui->fileNameButton->setMenu(fileNameMenu);
    m_ui->fileNameButton->setPopupMode(QToolButton::InstantPopup);
    connect(fileNameMenu, &MetaDataFormatterMenu::patternSelected, this, &FileWriterSettingsDialog::addTitleString);

    QSettings settings;
    QString outDir = QStandardPaths::writableLocation(QStandardPaths::MusicLocation);
    m_ui->outDirEdit->setText(settings.value(u"FileWriter/out_dir"_s, outDir).toString());
    m_ui->outFileEdit->setText(settings.value(u"FileWriter/file_name"_s, u"%p%if(%p&%t, - ,)%t"_s).toString());
    m_ui->qualitySpinBox->setValue(settings.value(u"FileWriter/vorbis_quality"_s, 0.8).toFloat());
    m_ui->singleFileCheckBox->setChecked(settings.value(u"FileWriter/single_file"_s, false).toBool());
}

FileWriterSettingsDialog::~FileWriterSettingsDialog()
{
    delete m_ui;
}

void FileWriterSettingsDialog::accept()
{
    QSettings settings;
    settings.setValue(u"FileWriter/out_dir"_s, m_ui->outDirEdit->text());
    settings.setValue(u"FileWriter/file_name"_s, m_ui->outFileEdit->text());
    settings.setValue(u"FileWriter/vorbis_quality"_s, m_ui->qualitySpinBox->value());
    settings.setValue(u"FileWriter/single_file"_s, m_ui->singleFileCheckBox->isChecked());
    QDialog::accept();
}

void FileWriterSettingsDialog::addTitleString(const QString &str)
{
    if (m_ui->outFileEdit->cursorPosition () < 1)
        m_ui->outFileEdit->insert(str);
    else
        m_ui->outFileEdit->insert(u" - "_s + str);
}

void FileWriterSettingsDialog::on_dirButton_clicked()
{
    QString dir = FileDialog::getExistingDirectory(this, tr("Choose a directory"), m_ui->outDirEdit->text());
    if(!dir.isEmpty())
        m_ui->outDirEdit->setText(dir);
}
