/***************************************************************************
 *   Copyright (C) 2015-2026 by Ilya Kotov                                 *
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
#include "columneditor_p.h"
#include "metadataformattermenu.h"
#include "ui_columneditor.h"

ColumnEditor::ColumnEditor(const QString &name, const QString &patt, QWidget *parent) :
    QDialog(parent),
    m_ui(new Ui::ColumnEditor)
{
    m_ui->setupUi(this);
    createMenu();
    fillTypes();

    //load inital values
    m_ui->nameLineEdit->setText(name);
    m_ui->formatLineEdit->setText(patt);
}

ColumnEditor::~ColumnEditor()
{
    delete m_ui;
}

QString ColumnEditor::name() const
{
    return m_ui->nameLineEdit->text();
}

QString ColumnEditor::pattern() const
{
    return m_ui->formatLineEdit->text();
}

void ColumnEditor::insertExpression(const QString &str)
{
    if (m_ui->formatLineEdit->cursorPosition () < 1)
        m_ui->formatLineEdit->insert(str);
    else
        m_ui->formatLineEdit->insert(u" - "_s + str);
}

void ColumnEditor::on_comboBox_activated(int index)
{
    m_ui->formatLineEdit->setText(m_ui->comboBox->itemData(index).toString());
    m_ui->nameLineEdit->setText(m_ui->comboBox->itemText(index));
}

void ColumnEditor::on_formatLineEdit_textChanged(const QString &text)
{
    int index = m_ui->comboBox->findData(text);
    if(index < 0)
        index = m_ui->comboBox->findData(u"custom"_s);
    m_ui->comboBox->setCurrentIndex(index);
}

void ColumnEditor::createMenu()
{
    MetaDataFormatterMenu *menu = new MetaDataFormatterMenu(MetaDataFormatterMenu::COLUMN_MENU, this);
    m_ui->formatButton->setMenu(menu);
    connect(menu, &MetaDataFormatterMenu::patternSelected, this, &ColumnEditor::insertExpression);
}

void ColumnEditor::fillTypes()
{
    m_ui->comboBox->addItem(tr("Artist"), u"%p"_s);
    m_ui->comboBox->addItem(tr("Album"), u"%a"_s);
    m_ui->comboBox->addItem(tr("Artist - Album"), u"%if(%p&%a,%p - %a,)"_s);
    m_ui->comboBox->addItem(tr("Artist - Title"), u"%if(%p,%p - %t,%t)"_s);
    m_ui->comboBox->addItem(tr("Album Artist"), u"%aa"_s);
    m_ui->comboBox->addItem(tr("Title"), u"%t"_s);
    m_ui->comboBox->addItem(tr("Track Number"), u"%n"_s);
    m_ui->comboBox->addItem(tr("Two-digit Track Number"), u"%NN"_s);
    m_ui->comboBox->addItem(tr("Genre"), u"%g"_s);
    m_ui->comboBox->addItem(tr("Comment"), u"%c"_s);
    m_ui->comboBox->addItem(tr("Composer"), u"%C"_s);
    m_ui->comboBox->addItem(tr("Duration"), u"%l"_s);
    m_ui->comboBox->addItem(tr("Disc Number"), u"%D"_s);
    m_ui->comboBox->addItem(tr("File Name"), u"%f"_s);
    m_ui->comboBox->addItem(tr("File Path"), u"%F"_s);
    m_ui->comboBox->addItem(tr("Track Index"), u"%I"_s);
    m_ui->comboBox->addItem(tr("Year"), u"%y"_s);
    m_ui->comboBox->addItem(tr("Parent Directory Name"), u"%dir(0)"_s);
    m_ui->comboBox->addItem(tr("Parent Directory Path"), u"%dir"_s);
    m_ui->comboBox->addItem(tr("Custom"), u"custom"_s);
}
