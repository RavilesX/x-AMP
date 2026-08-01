/***************************************************************************
 *   Copyright (C) 2020-2026 by Ilya Kotov                                 *
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

#include <QDir>
#include <QSettings>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <qmmp/qmmp.h>
#include <qmmpui/filedialog.h>
#include "librarysettingsdialog.h"
#include "ui_librarysettingsdialog.h"

LibrarySettingsDialog::LibrarySettingsDialog(QWidget *parent) :
    QDialog(parent),
    m_ui(new Ui::LibrarySettingsDialog)
{
    m_ui->setupUi(this);
    QSettings settings;
    m_lastPath = settings.value(u"Library/last_path"_s, QDir::homePath()).toString();
    QStringList dirs = settings.value(u"Library/dirs"_s).toStringList();
    m_ui->dirsListWidget->addItems(dirs);
    m_ui->showYearCheckBox->setChecked(settings.value(u"Library/show_year"_s, false).toBool());
    m_ui->recreateDatabaseCheckBox->setChecked(settings.value(u"Library/recreate_db"_s, false).toBool());
}

LibrarySettingsDialog::~LibrarySettingsDialog()
{
    delete m_ui;
}

void LibrarySettingsDialog::accept()
{
    QSettings settings;
    settings.setValue(u"Library/last_path"_s, m_lastPath);

    QStringList dirs;
    for(int i = 0; i < m_ui->dirsListWidget->count(); ++i)
        dirs << m_ui->dirsListWidget->item(i)->text();

    settings.setValue(u"Library/dirs"_s, dirs);
    settings.setValue(u"Library/show_year"_s, m_ui->showYearCheckBox->isChecked());
    settings.setValue(u"Library/recreate_db"_s, m_ui->recreateDatabaseCheckBox->isChecked());
    QDialog::accept();
}

void LibrarySettingsDialog::on_addDirButton_clicked()
{
    QString path = FileDialog::getExistingDirectory(this, tr("Select Directories for Scanning"), m_lastPath);
    if(!path.isEmpty())
    {
        m_ui->dirsListWidget->addItem(path);
        m_lastPath = QFileInfo(path).absolutePath();
    }
}

void LibrarySettingsDialog::on_removeDirButton_clicked()
{
    QList<QListWidgetItem *> items = m_ui->dirsListWidget->selectedItems();
    qDeleteAll(items);
}
