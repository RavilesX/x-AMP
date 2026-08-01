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
#include <QStringList>
#include <qmmp/qmmp.h>

extern "C"{
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/dict.h>
}
#include "ui_ffmpegsettingsdialog.h"
#include "ffmpegsettingsdialog.h"

FFmpegSettingsDialog::FFmpegSettingsDialog(QWidget *parent)
        : QDialog(parent), m_ui(new Ui::FFmpegSettingsDialog)
{
    m_ui->setupUi(this);
    QSettings settings;
    QStringList disabledFilters = { u"*.mp3"_s };
    disabledFilters = settings.value(u"FFMPEG/disabled_filters"_s, disabledFilters).toStringList();

    m_ui->wmaCheckBox->setEnabled(avcodec_find_decoder(AV_CODEC_ID_WMAV1));
    m_ui->wmaCheckBox->setChecked(!disabledFilters.contains(u"*.wma"_s) && avcodec_find_decoder(AV_CODEC_ID_WMAV1));
    m_ui->apeCheckBox->setEnabled(avcodec_find_decoder(AV_CODEC_ID_APE));
    m_ui->apeCheckBox->setChecked(!disabledFilters.contains(u"*.ape"_s) && avcodec_find_decoder(AV_CODEC_ID_APE));
    m_ui->ttaCheckBox->setEnabled(avcodec_find_decoder(AV_CODEC_ID_TTA));
    m_ui->ttaCheckBox->setChecked(!disabledFilters.contains(u"*.tta"_s) && avcodec_find_decoder(AV_CODEC_ID_TTA));
    m_ui->aacCheckBox->setEnabled(avcodec_find_decoder(AV_CODEC_ID_AAC));
    m_ui->aacCheckBox->setChecked(!disabledFilters.contains(u"*.aac"_s) && avcodec_find_decoder(AV_CODEC_ID_AAC));
    m_ui->mp3CheckBox->setEnabled(avcodec_find_decoder(AV_CODEC_ID_MP3));
    m_ui->mp3CheckBox->setChecked(!disabledFilters.contains(u"*.mp3"_s) && avcodec_find_decoder(AV_CODEC_ID_MP3));
    m_ui->mp4CheckBox->setEnabled(avcodec_find_decoder(AV_CODEC_ID_AAC));
    m_ui->mp4CheckBox->setChecked(!disabledFilters.contains(u"*.m4a"_s) && (avcodec_find_decoder(AV_CODEC_ID_AAC)
                                                               || avcodec_find_decoder(AV_CODEC_ID_ALAC)));
    m_ui->raCheckBox->setEnabled(avcodec_find_decoder(AV_CODEC_ID_RA_288));
    m_ui->raCheckBox->setChecked(!disabledFilters.contains(u"*.ra"_s) && avcodec_find_decoder(AV_CODEC_ID_RA_288));
    m_ui->shCheckBox->setChecked(!disabledFilters.contains(u"*.shn"_s) && avcodec_find_decoder(AV_CODEC_ID_SHORTEN));
    m_ui->ac3CheckBox->setEnabled(avcodec_find_decoder(AV_CODEC_ID_AC3) || avcodec_find_decoder(AV_CODEC_ID_EAC3));
    m_ui->ac3CheckBox->setChecked(!disabledFilters.contains(u"*.ac3"_s) && (avcodec_find_decoder(AV_CODEC_ID_AC3) ||
                                  avcodec_find_decoder(AV_CODEC_ID_EAC3)));
    m_ui->dtsCheckBox->setEnabled(avcodec_find_decoder(AV_CODEC_ID_DTS));
    m_ui->dtsCheckBox->setChecked(!disabledFilters.contains(u"*.dts"_s) && avcodec_find_decoder(AV_CODEC_ID_DTS));
    m_ui->mkaCheckBox->setEnabled(av_find_input_format("matroska") && avcodec_find_decoder(AV_CODEC_ID_TRUEHD));
    m_ui->mkaCheckBox->setChecked(m_ui->mkaCheckBox->isEnabled() && !disabledFilters.contains(u"*.mka"_s));
    m_ui->webmCheckBox->setEnabled(av_find_input_format("webm") && (avcodec_find_decoder(AV_CODEC_ID_OPUS) || avcodec_find_decoder(AV_CODEC_ID_VORBIS)));
    m_ui->webmCheckBox->setChecked(m_ui->webmCheckBox->isEnabled() && !disabledFilters.contains(u"*.webm"_s));
    m_ui->vqfCheckBox->setEnabled(avcodec_find_decoder(AV_CODEC_ID_TWINVQ));
    m_ui->vqfCheckBox->setChecked(!disabledFilters.contains(u"*.vqf"_s) && avcodec_find_decoder(AV_CODEC_ID_TWINVQ));
    m_ui->takCheckBox->setEnabled(avcodec_find_decoder(AV_CODEC_ID_TAK));
    m_ui->takCheckBox->setChecked(!disabledFilters.contains(u"*.tak"_s) && avcodec_find_decoder(AV_CODEC_ID_TAK));
    m_ui->dsdCheckBox->setEnabled(avcodec_find_decoder(AV_CODEC_ID_DSD_LSBF));
    m_ui->dsdCheckBox->setChecked(!disabledFilters.contains(u"*.dsdiff"_s) && avcodec_find_decoder(AV_CODEC_ID_DSD_LSBF));
}

FFmpegSettingsDialog::~FFmpegSettingsDialog()
{
    delete m_ui;
}

void FFmpegSettingsDialog::accept()
{
    QStringList disabledFilters;
    if(!m_ui->mp3CheckBox->isChecked())
        disabledFilters << u"*.mp3"_s;
    if(!m_ui->wmaCheckBox->isChecked())
        disabledFilters << u"*.wma"_s;
    if(!m_ui->apeCheckBox->isChecked())
        disabledFilters << u"*.ape"_s;
    if(!m_ui->ttaCheckBox->isChecked())
        disabledFilters << u"*.tta"_s;
    if(!m_ui->aacCheckBox->isChecked())
        disabledFilters << u"*.aac"_s;
    if(!m_ui->mp4CheckBox->isChecked())
        disabledFilters << u"*.m4a"_s << u"*.m4b"_s;
    if(!m_ui->raCheckBox->isChecked())
        disabledFilters << u"*.ra"_s;
    if(!m_ui->shCheckBox->isChecked())
        disabledFilters << u"*.shn"_s;
    if(!m_ui->ac3CheckBox->isChecked())
        disabledFilters << u"*.ac3"_s;
    if(!m_ui->dtsCheckBox->isChecked())
        disabledFilters << u"*.dts"_s;
    if(!m_ui->mkaCheckBox->isChecked())
        disabledFilters << u"*.mka"_s;
    if(!m_ui->webmCheckBox->isChecked())
        disabledFilters << u"*.webm"_s;
    if(!m_ui->vqfCheckBox->isChecked())
        disabledFilters << u"*.vqf"_s;
    if(!m_ui->takCheckBox->isChecked())
        disabledFilters << u"*.tak"_s;
    if(!m_ui->dsdCheckBox->isChecked())
        disabledFilters << u"*.dsf"_s << u"*.dsdiff"_s;
    QSettings settings;
    settings.setValue(u"FFMPEG/disabled_filters"_s, disabledFilters);
    QDialog::accept();
}
