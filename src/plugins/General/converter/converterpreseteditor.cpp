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

#include <QMenu>
#include <qmmp/qmmp.h>
#include "converterpreseteditor.h"
#include "ui_converterpreseteditor.h"

using namespace Qt::Literals::StringLiterals;

PresetEditor::PresetEditor(const QVariantHash &data, QWidget *parent) :
    QDialog(parent), m_ui(new Ui::ConverterPresetEditor)
{
    m_ui->setupUi(this);
    m_ui->nameLineEdit->setText(data.value(u"name"_s).toString());
    m_ui->extensionLineEdit->setText(data.value(u"ext"_s).toString());
    m_ui->commandLineEdit->setText(data.value(u"command"_s).toString());
    m_ui->us16bitCheckBox->setChecked(data.value(u"use_16bit"_s).toBool());
    m_ui->tagsCheckBox->setChecked(data.value(u"tags"_s).toBool());

    if(data[u"read_only"_s].toBool())
    {
        setWindowTitle(tr("%1 (Read Only)").arg(windowTitle()));
        m_ui->buttonBox->setStandardButtons(QDialogButtonBox::Close);
        m_ui->nameLineEdit->setReadOnly(true);
        m_ui->extensionLineEdit->setReadOnly(true);
        m_ui->commandLineEdit->setReadOnly(true);
        m_ui->us16bitCheckBox->setDisabled(true);
        m_ui->tagsCheckBox->setDisabled(true);
        m_ui->commandToolButton->setDisabled(true);
    }
    else
        createMenus();
}

PresetEditor::~PresetEditor()
{
    delete m_ui;
}

QVariantHash PresetEditor::data() const
{
    QVariantHash data;
    data.insert(u"name"_s, m_ui->nameLineEdit->text());
    data.insert(u"ext"_s, m_ui->extensionLineEdit->text());
    data.insert(u"command"_s, m_ui->commandLineEdit->text());
    data.insert(u"use_16bit"_s,  m_ui->us16bitCheckBox->isChecked());
    data.insert(u"tags"_s, m_ui->tagsCheckBox->isChecked());
    data.insert(u"read_only"_s, false);
    return data;
}

void PresetEditor::createMenus()
{
    QMenu *commandMenu = new QMenu(this);
    commandMenu->addAction(tr("Output file"))->setData(u"%o"_s);
    commandMenu->addAction(tr("Input file"))->setData(u"%i"_s);

    m_ui->commandToolButton->setMenu(commandMenu);
    m_ui->commandToolButton->setPopupMode(QToolButton::InstantPopup);
    connect(commandMenu, &QMenu::triggered, this, &PresetEditor::addCommandString);
}

void PresetEditor::addCommandString(QAction *a)
{
    m_ui->commandLineEdit->insert(a->data().toString());
}
