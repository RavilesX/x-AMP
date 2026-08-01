/***************************************************************************
 *   Copyright (C) 2009-2012 by Artur Guzik                                *
 *   a.guzik88@gmail.com                                                   *
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

#include "kdenotifysettingsdialog.h"
#include "ui_kdenotifysettingsdialog.h"
#include "kdenotify.h"
#include <qmmp/qmmp.h>
#include <qmmpui/templateeditor.h>
#include <QSettings>

KdeNotifySettingsDialog::KdeNotifySettingsDialog(QWidget *parent) :
    QDialog(parent),
    m_ui(new Ui::KdeNotifySettingsDialog)
{
    m_ui->setupUi(this);

    QSettings settings;
    settings.beginGroup(u"Kde_Notifier"_s);
    m_ui->notifyDelaySpinBox->setValue(settings.value(u"notify_duration"_s, 5000).toInt() / 1000);
    m_ui->showCoversCheckBox->setChecked(settings.value(u"show_covers"_s, true).toBool());
    m_ui->updateNotifyCheckBox->setChecked(settings.value(u"update_notify"_s, true).toBool());
    m_ui->volumeCheckBox->setChecked(settings.value(u"volume_notification"_s, false).toBool());
    m_template = settings.value(u"template"_s, DEFAULT_TEMPLATE).toString();
    settings.endGroup();
}

KdeNotifySettingsDialog::~KdeNotifySettingsDialog()
{
    delete m_ui;
}

void KdeNotifySettingsDialog::accept()
{
    QSettings settings;
    settings.beginGroup(u"Kde_Notifier"_s);
    settings.setValue(u"notify_duration"_s, m_ui->notifyDelaySpinBox->value() * 1000);
    settings.setValue(u"show_covers"_s, m_ui->showCoversCheckBox->isChecked());
    settings.setValue(u"template"_s, m_template);
    settings.setValue(u"update_notify"_s, m_ui->updateNotifyCheckBox->isChecked());
    settings.setValue(u"volume_notification"_s, m_ui->volumeCheckBox->isChecked());
    settings.endGroup();
    QDialog::accept();
}

void KdeNotifySettingsDialog::changeEvent(QEvent *e)
{
    QDialog::changeEvent(e);
    switch (e->type()) {
    case QEvent::LanguageChange:
        m_ui->retranslateUi(this);
        break;
    default:
        break;
    }
}

void KdeNotifySettingsDialog::on_templateButton_clicked()
{
    QString t = TemplateEditor::getTemplate(this, tr("Notification Template"), m_template, DEFAULT_TEMPLATE);
    if(!t.isEmpty())
        m_template = t;
}
