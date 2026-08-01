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
#include <QFontDialog>
#include <QMenu>
#include <qmmp/qmmp.h>
#include <qmmpui/templateeditor.h>
#include "popupwidget.h"
#include "ui_notifiersettingsdialog.h"
#include "notifiersettingsdialog.h"

NotifierSettingsDialog::NotifierSettingsDialog(QWidget *parent)
        : QDialog(parent), m_ui(new Ui::NotifierSettingsDialog)
{
    m_ui->setupUi(this);

    connect(m_ui->transparencySlider, &QSlider::valueChanged, m_ui->transparencyLabel, qOverload<int>(&QLabel::setNum));
    connect(m_ui->coverSizeSlider, &QSlider::valueChanged, m_ui->coverSizeLabel, qOverload<int>(&QLabel::setNum));

    m_buttons.insert(PopupWidget::TOPLEFT, m_ui->topLeftButton);
    m_buttons.insert(PopupWidget::TOP, m_ui->topButton);
    m_buttons.insert(PopupWidget::TOPRIGHT, m_ui->topRightButton);
    m_buttons.insert(PopupWidget::RIGHT, m_ui->rightButton);
    m_buttons.insert(PopupWidget::BOTTOMRIGHT, m_ui->bottomRightButton);
    m_buttons.insert(PopupWidget::BOTTOM, m_ui->bottomButton);
    m_buttons.insert(PopupWidget::BOTTOMLEFT, m_ui->bottomLeftButton);
    m_buttons.insert(PopupWidget::LEFT, m_ui->leftButton);
    m_buttons.insert(PopupWidget::CENTER, m_ui->centerButton);

    QSettings settings;
    settings.beginGroup(u"Notifier"_s);
    m_ui->messageDelaySpinBox->setValue(settings.value(u"message_delay"_s, 2000).toInt());
    uint pos = settings.value(u"message_pos"_s, PopupWidget::BOTTOMLEFT).toUInt();
    m_buttons.value(pos)->setChecked(true);
    m_ui->psiCheckBox->setChecked(settings.value(u"psi_notification"_s, false).toBool());
    m_ui->resumeCheckBox->setChecked(settings.value(u"resume_notification"_s, false).toBool());
    m_ui->songCheckBox->setChecked(settings.value(u"song_notification"_s, true).toBool());
    m_ui->volumeCheckBox->setChecked(settings.value(u"volume_notification"_s, true).toBool());
    m_ui->disableForFScheckBox->setChecked(settings.value(u"disable_fullscreen"_s, false).toBool());
    m_ui->transparencySlider->setValue(100 - settings.value(u"opacity"_s, 1.0).toDouble() * 100);
    QString fontname = settings.value(u"font"_s).toString();
    m_ui->coverSizeSlider->setValue(settings.value(u"cover_size"_s, 64).toInt());
    m_template = settings.value(u"template"_s, DEFAULT_TEMPLATE).toString();
    settings.endGroup();
    QFont font;
    if(!fontname.isEmpty())
        font.fromString(fontname);
    m_ui->fontLabel->setText(font.family() + QChar::Space + QString::number(font.pointSize ()));
    m_ui->fontLabel->setFont(font);
}


NotifierSettingsDialog::~NotifierSettingsDialog()
{
    delete m_ui;
}

void NotifierSettingsDialog::accept()
{
    QSettings settings;
    settings.beginGroup(u"Notifier"_s);
    settings.setValue(u"message_delay"_s, m_ui->messageDelaySpinBox->value());
    uint pos = PopupWidget::BOTTOMLEFT;
    for(QPushButton *button : std::as_const(m_buttons))
    {
        if(button->isChecked())
            pos = m_buttons.key(button);
    }
    settings.setValue(u"message_pos"_s, pos);
    settings.setValue(u"psi_notification"_s, m_ui->psiCheckBox->isChecked());
    settings.setValue(u"resume_notification"_s, m_ui->resumeCheckBox->isChecked());
    settings.setValue(u"song_notification"_s, m_ui->songCheckBox->isChecked());
    settings.setValue(u"volume_notification"_s, m_ui->volumeCheckBox->isChecked());
    settings.setValue(u"disable_fullscreen"_s, m_ui->disableForFScheckBox->isChecked());
    settings.setValue(u"opacity"_s, 1.0 -  (double)m_ui->transparencySlider->value()/100);
    settings.setValue(u"font"_s, m_ui->fontLabel->font().toString());
    settings.setValue(u"cover_size"_s, m_ui->coverSizeSlider->value());
    settings.setValue(u"template"_s, m_template);
    settings.endGroup();
    QDialog::accept();
}

void NotifierSettingsDialog::on_fontButton_pressed()
{
    bool ok;
    QFont font = m_ui->fontLabel->font();
    font = QFontDialog::getFont(&ok, font, this);
    if (ok)
    {
        m_ui->fontLabel->setText (font.family () + QChar::Space + QString::number(font.pointSize ()));
        m_ui->fontLabel->setFont(font);
    }
}

void NotifierSettingsDialog::on_templateButton_pressed()
{
    QString t = TemplateEditor::getTemplate(this, tr("Notification Template"), m_template,
                                            DEFAULT_TEMPLATE);
    if(!t.isEmpty())
        m_template = t;
}
