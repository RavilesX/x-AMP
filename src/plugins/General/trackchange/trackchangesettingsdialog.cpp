/***************************************************************************
 *   Copyright (C) 2013-2026 by Ilya Kotov                                 *
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
#include <QMenu>
#include <qmmp/qmmp.h>
#include <qmmpui/metadataformattermenu.h>
#include "ui_trackchangesettingsdialog.h"
#include "trackchangesettingsdialog.h"

TrackChangeSettingsDialog::TrackChangeSettingsDialog(QWidget *parent) : QDialog(parent),
    m_ui(new Ui::TrackChangeSettingsDialog)
{
    m_ui->setupUi(this);
    addMenu(m_ui->newTrackButton);
    addMenu(m_ui->endOfTrackButton);
    addMenu(m_ui->endOfPlayListButton);
    addMenu(m_ui->titleChangeButton);

    QSettings settings;
    settings.beginGroup(u"TrackChange"_s);
    m_ui->newTrackLineEdit->setText(settings.value(u"new_track_command"_s).toString());
    m_ui->endOfTrackLineEdit->setText(settings.value(u"end_of_track_command"_s).toString());
    m_ui->endOfPlayListLineEdit->setText(settings.value(u"end_of_pl_command"_s).toString());
    m_ui->titleChangeLineEdit->setText(settings.value(u"title_change_command"_s).toString());
    m_ui->appStartupLineEdit->setText(settings.value(u"application_startup_command"_s).toString());
    m_ui->appExitLineEdit->setText(settings.value(u"application_exit_command"_s).toString());
    settings.endGroup();
}

TrackChangeSettingsDialog::~TrackChangeSettingsDialog()
{
    delete m_ui;
}

void TrackChangeSettingsDialog::accept()
{
    QSettings settings;
    settings.beginGroup(u"TrackChange"_s);
    settings.setValue(u"new_track_command"_s, m_ui->newTrackLineEdit->text());
    settings.setValue(u"end_of_track_command"_s,  m_ui->endOfTrackLineEdit->text());
    settings.setValue(u"end_of_pl_command"_s,  m_ui->endOfPlayListLineEdit->text());
    settings.setValue(u"title_change_command"_s, m_ui->titleChangeLineEdit->text());
    settings.setValue(u"application_startup_command"_s, m_ui->appStartupLineEdit->text());
    settings.setValue(u"application_exit_command"_s, m_ui->appExitLineEdit->text());
    settings.endGroup();
    QDialog::accept();
}

void TrackChangeSettingsDialog::addMenu(QToolButton *button)
{
    MetaDataFormatterMenu *menu = new MetaDataFormatterMenu(MetaDataFormatterMenu::TITLE_MENU, this);
    button->setMenu(menu);
    button->setPopupMode(QToolButton::InstantPopup);
    connect(menu, &MetaDataFormatterMenu::patternSelected, this, &TrackChangeSettingsDialog::addTemplateString);
}

void TrackChangeSettingsDialog::addTemplateString(const QString &str)
{
    QMenu *menu = qobject_cast<QMenu*>(sender());
    if(!menu)
        return;

    if(m_ui->newTrackButton->menu() == menu)
    {
        m_ui->newTrackLineEdit->insert(str);
    }
    else if(m_ui->endOfTrackButton->menu() == menu)
    {
        m_ui->endOfTrackLineEdit->insert(str);
    }
    else if(m_ui->endOfPlayListButton->menu() == menu)
    {
        m_ui->endOfPlayListLineEdit->insert(str);
    }
    else if(m_ui->titleChangeButton->menu() == menu)
    {
        m_ui->titleChangeLineEdit->insert(str);
    }
}
