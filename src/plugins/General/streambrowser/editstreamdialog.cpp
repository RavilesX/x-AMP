/***************************************************************************
 *   Copyright (C) 2018-2026 by Ilya Kotov                                 *
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

#include <QPushButton>
#include <QUrl>
#include <qmmp/qmmp.h>
#include "editstreamdialog.h"
#include "ui_editstreamdialog.h"

EditStreamDialog::EditStreamDialog(QWidget *parent) :
    QDialog(parent),
    m_ui(new Ui::EditStreamDialog)
{
    m_ui->setupUi(this);
    connect(m_ui->nameLineEdit, &QLineEdit::textChanged, this, &EditStreamDialog::validate);
    connect(m_ui->urlLineEdit, &QLineEdit::textChanged, this, &EditStreamDialog::validate);
    m_ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
    m_ui->typeComboBox->addItems({ u"application/ogg"_s, u"audio/aac"_s, u"audio/aacp"_s, u"audio/mpeg"_s });
}

EditStreamDialog::~EditStreamDialog()
{
    delete m_ui;
}

void EditStreamDialog::setValues(const QHash<EditStreamDialog::Key, QString> &values)
{
    m_values = values;
    m_ui->urlLineEdit->setText(values.value(URL));
    m_ui->nameLineEdit->setText(values.value(NAME));
    m_ui->genreLineEdit->setText(values.value(GENRE));
    m_ui->bitrateLineEdit->setText(values.value(BITRATE));
    m_ui->typeComboBox->setEditText(values.value(TYPE));
}

QHash<EditStreamDialog::Key, QString> EditStreamDialog::values() const
{
    return m_values;
}

void EditStreamDialog::accept()
{
    m_values[URL] = m_ui->urlLineEdit->text();
    m_values[NAME] = m_ui->nameLineEdit->text();
    m_values[GENRE] = m_ui->genreLineEdit->text();
    m_values[BITRATE] = m_ui->bitrateLineEdit->text();
    m_values[TYPE] = m_ui->typeComboBox->currentText();
    QDialog::accept();
}

void EditStreamDialog::validate()
{
    bool ok = !m_ui->nameLineEdit->text().isEmpty();
    ok &= QUrl(m_ui->urlLineEdit->text()).isValid();
    m_ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(ok);
}
