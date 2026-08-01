/***************************************************************************
 *   Copyright (C) 2006-2026 by Ilya Kotov                                 *
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
#include <qmmp/qmmptextcodec.h>
#include "ui_mpegsettingsdialog.h"
#include "mpegsettingsdialog.h"

MpegSettingsDialog::MpegSettingsDialog(bool using_rusxmms, QWidget *parent)
        : QDialog(parent), m_ui(new Ui::MpegSettingsDialog)
{
    m_ui->setupUi(this);

    m_ui->id3v1EncComboBox->addItem(tr("Detect by Locale"), u"locale"_s);
    const QStringList charsets = QmmpTextCodec::availableCharsets();
    for(const QString &charset : std::as_const(charsets))
    {
        m_ui->id3v1EncComboBox->addItem(charset, charset);
        m_ui->id3v2EncComboBox->addItem(charset, charset);
    }

    QSettings settings;
    settings.beginGroup(u"MPEG"_s);

#if defined(WITH_MAD) && defined(WITH_MPG123)
    QString decoderName = settings.value(u"decoder"_s, u"mpg123"_s).toString();
    m_ui->madRadioButton->setChecked(true);
    m_ui->mpg123RadioButton->setChecked(decoderName == "mpg123"_L1);
    m_ui->enableCrcCheckBox->setChecked(settings.value(u"enable_crc"_s, false).toBool());
#elif defined(WITH_MAD)
    m_ui->madRadioButton->setChecked(true);
    m_ui->decoderGroupBox->setEnabled(false);
#elif defined(WITH_MPG123)
    m_ui->mpg123RadioButton->setChecked(true);
    m_ui->decoderGroupBox->setEnabled(false);
#endif

    int pos = m_ui->id3v1EncComboBox->findData(settings.value(u"ID3v1_encoding"_s, u"locale"_s).toString());
    m_ui->id3v1EncComboBox->setCurrentIndex(pos);
    pos = m_ui->id3v2EncComboBox->findData(settings.value(u"ID3v2_encoding"_s, u"UTF-8"_s).toString());
    m_ui->id3v2EncComboBox->setCurrentIndex(pos);

    m_ui->firstTagComboBox->setCurrentIndex(settings.value(u"tag_1"_s, ID3v2).toInt());
    m_ui->secondTagComboBox->setCurrentIndex(settings.value(u"tag_2"_s, APE).toInt());
    m_ui->thirdTagComboBox->setCurrentIndex(settings.value(u"tag_3"_s, ID3v1).toInt());
    m_ui->mergeTagsCheckBox->setChecked(settings.value(u"merge_tags"_s, false).toBool());
    m_ui->detectEncodingCheckBox->setChecked(settings.value(u"detect_encoding"_s, false).toBool());

    settings.endGroup();

    if(using_rusxmms)
    {
        m_ui->id3v1EncComboBox->setEnabled(false);
        m_ui->id3v2EncComboBox->setEnabled(false);
        m_ui->detectEncodingCheckBox->setEnabled(false);
    }
}

MpegSettingsDialog::~MpegSettingsDialog()
{
    delete m_ui;
}

void MpegSettingsDialog::accept()
{
    QSettings settings;
    settings.beginGroup(u"MPEG"_s);
    settings.setValue(u"decoder"_s, m_ui->mpg123RadioButton->isChecked() ? u"mpg123"_s : u"mad"_s);
    settings.setValue(u"enable_crc"_s, m_ui->enableCrcCheckBox->isChecked());
    settings.setValue(u"ID3v1_encoding"_s, m_ui->id3v1EncComboBox->currentData());
    settings.setValue(u"ID3v2_encoding"_s, m_ui->id3v2EncComboBox->currentData());
    settings.setValue(u"detect_encoding"_s, m_ui->detectEncodingCheckBox->isChecked());
    settings.setValue(u"tag_1"_s, m_ui->firstTagComboBox->currentIndex());
    settings.setValue(u"tag_2"_s, m_ui->secondTagComboBox->currentIndex());
    settings.setValue(u"tag_3"_s, m_ui->thirdTagComboBox->currentIndex());
    settings.setValue(u"merge_tags"_s, m_ui->mergeTagsCheckBox->isChecked());
    settings.endGroup();
    QDialog::accept();
}
