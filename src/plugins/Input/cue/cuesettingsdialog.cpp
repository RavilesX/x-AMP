/***************************************************************************
 *   Copyright (C) 2008-2026 by Ilya Kotov                                 *
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
#include <QRegularExpression>
#include <qmmp/qmmptextcodec.h>
#include <qmmp/qmmp.h>

#ifdef WITH_ENCA
#include <enca.h>
#endif

#include "ui_cuesettingsdialog.h"
#include "cuesettingsdialog.h"

CueSettingsDialog::CueSettingsDialog(QWidget *parent)
        : QDialog(parent), m_ui(new Ui::CueSettingsDialog)
{
    m_ui->setupUi(this);
    m_ui->cueEncComboBox->addItems(QmmpTextCodec::availableCharsets());

#ifdef WITH_ENCA
    size_t n = 0;
    const char **langs = enca_get_languages(&n);
    for (size_t i = 0; i < n; ++i)
        m_ui->encaAnalyserComboBox->addItem(QString::fromLatin1(langs[i]));
#endif
    QSettings settings;
    settings.beginGroup(u"CUE"_s);
    int pos = m_ui->cueEncComboBox->findText(settings.value(u"encoding"_s, u"UTF-8"_s).toString());
    m_ui->cueEncComboBox->setCurrentIndex(pos);
#ifdef WITH_ENCA
    m_ui->autoCharsetCheckBox->setChecked(settings.value(u"use_enca"_s, false).toBool());
    pos = m_ui->encaAnalyserComboBox->findText(settings.value(u"enca_lang"_s, QString::fromLatin1(langs[n - 1])).toString());
    m_ui->encaAnalyserComboBox->setCurrentIndex(pos);
#else
    m_ui->autoCharsetCheckBox->setEnabled(false);
#endif
    m_ui->dirtyCueCheckBox->setChecked(settings.value(u"dirty_cue"_s, false).toBool());
    settings.endGroup();
}

CueSettingsDialog::~CueSettingsDialog()
{
    delete m_ui;
}

void CueSettingsDialog::accept()
{
    QSettings settings;
    settings.beginGroup(u"CUE"_s);
    settings.setValue(u"encoding"_s, m_ui->cueEncComboBox->currentText());
#ifdef WITH_ENCA
    settings.setValue(u"use_enca"_s, m_ui->autoCharsetCheckBox->isChecked());
    settings.setValue(u"enca_lang"_s, m_ui->encaAnalyserComboBox->currentText());
#endif
    settings.setValue(u"dirty_cue"_s, m_ui->dirtyCueCheckBox->isChecked());
    settings.endGroup();
    QDialog::accept();
}
