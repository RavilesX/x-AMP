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
#include <QMessageBox>
#include <qmmp/qmmp.h>
#include "scrobbler.h"
#include "defines.h"
#include "ui_scrobblersettingsdialog.h"
#include "scrobblersettingsdialog.h"

ScrobblerSettingsDialog::ScrobblerSettingsDialog(QWidget *parent) : QDialog(parent),
    m_ui(new Ui::ScrobblerSettingsDialog)
{
    m_ui->setupUi(this);
    m_lastfmAuth = new ScrobblerAuth(SCROBBLER_LASTFM_URL, LASTFM_AUTH_URL, u"lastfm"_s, this);
    m_librefmAuth = new ScrobblerAuth(SCROBBLER_LIBREFM_URL, LIBREFM_AUTH_URL, u"librefm"_s, this);
    connect(m_lastfmAuth, &ScrobblerAuth::tokenRequestFinished, this, &ScrobblerSettingsDialog::processTokenResponse);
    connect(m_lastfmAuth, &ScrobblerAuth::sessionRequestFinished, this, &ScrobblerSettingsDialog::processSessionResponse);
    connect(m_lastfmAuth, &ScrobblerAuth::checkSessionFinished, this, &ScrobblerSettingsDialog::processCheckResponse);
    connect(m_librefmAuth, &ScrobblerAuth::tokenRequestFinished, this, &ScrobblerSettingsDialog::processTokenResponse);
    connect(m_librefmAuth, &ScrobblerAuth::sessionRequestFinished, this, &ScrobblerSettingsDialog::processSessionResponse);
    connect(m_librefmAuth, &ScrobblerAuth::checkSessionFinished, this, &ScrobblerSettingsDialog::processCheckResponse);

    QSettings settings;
    settings.beginGroup(u"Scrobbler"_s);
    m_ui->lastfmGroupBox->setChecked(settings.value(u"use_lastfm"_s, false).toBool());
    m_ui->librefmGroupBox->setChecked(settings.value(u"use_librefm"_s, false).toBool());
    m_ui->sessionLineEdit_lastfm->setText(settings.value(u"lastfm_session"_s).toString());
    m_ui->sessionLineEdit_librefm->setText(settings.value(u"librefm_session"_s).toString());
    settings.endGroup();
}

ScrobblerSettingsDialog::~ScrobblerSettingsDialog()
{
    delete m_ui;
}

void ScrobblerSettingsDialog::accept()
{
    QSettings settings;
    settings.beginGroup(u"Scrobbler"_s);
    settings.setValue(u"use_lastfm"_s, m_ui->lastfmGroupBox->isChecked());
    settings.setValue(u"use_librefm"_s, m_ui->librefmGroupBox->isChecked());
    settings.setValue(u"lastfm_session"_s, m_ui->sessionLineEdit_lastfm->text());
    settings.setValue(u"librefm_session"_s, m_ui->sessionLineEdit_librefm->text());
    settings.endGroup();
    QDialog::accept();
}

void ScrobblerSettingsDialog::on_newSessionButton_lastfm_clicked()
{
    m_ui->newSessionButton_lastfm->setEnabled(false);
    m_lastfmAuth->getToken();
}

void ScrobblerSettingsDialog::on_newSessionButton_librefm_clicked()
{
    m_ui->newSessionButton_librefm->setEnabled(false);
    m_librefmAuth->getToken();
}

void ScrobblerSettingsDialog::processTokenResponse(int error)
{
    if(sender() == m_lastfmAuth)
        m_ui->newSessionButton_lastfm->setEnabled(true);
    else if(sender() == m_librefmAuth)
        m_ui->newSessionButton_librefm->setEnabled(true);

    switch(error)
    {
    case ScrobblerAuth::NO_ERROR:
    {
        ScrobblerAuth *auth = qobject_cast<ScrobblerAuth *>(sender());
        QString name;

        if(auth == m_lastfmAuth)
        {
            m_ui->newSessionButton_lastfm->setEnabled(false);
            name = u"Last.fm"_s;
        }
        else if(auth == m_librefmAuth)
        {
            m_ui->newSessionButton_librefm->setEnabled(false);
            name = u"Libre.fm"_s;
        }
        else
        {
            qCWarning(plugin, "invalid sender");
            return;
        }

        QMessageBox::information(this, tr("Message"),
                                 tr("1. Wait for browser startup") + QChar::LineFeed +
                                 tr("2. Allow x-AMP to scrobble tracks to your %1 account").arg(name) + QChar::LineFeed +
                                 tr("3. Press \"OK\""));
        auth->getSession();
    }
        break;
    case ScrobblerAuth::NETWORK_ERROR:
        QMessageBox::warning(this, tr("Error"), tr("Network error"));
        break;
    case ScrobblerAuth::LASTFM_ERROR:
    default:
        QMessageBox::warning(this, tr("Error"), tr("Unable to register new session"));
    }
}

void ScrobblerSettingsDialog::processSessionResponse(int error)
{
    if(sender() == m_lastfmAuth)
        m_ui->newSessionButton_lastfm->setEnabled(true);
    else if(sender() == m_librefmAuth)
        m_ui->newSessionButton_librefm->setEnabled(true);
    switch(error)
    {
    case ScrobblerAuth::NO_ERROR:
    {
        QMessageBox::information(this, tr("Message"), tr("New session has been received successfully"));
        QSettings settings;
        if(sender() == m_lastfmAuth)
        {
            m_ui->sessionLineEdit_lastfm->setText(m_lastfmAuth->session());
            settings.setValue(u"Scrobbler/lastfm_session"_s, m_ui->sessionLineEdit_lastfm->text());
        }
        else if(sender() == m_librefmAuth)
        {
            m_ui->sessionLineEdit_librefm->setText(m_librefmAuth->session());
            settings.setValue(u"Scrobbler/librefm_session"_s, m_ui->sessionLineEdit_librefm->text());
        }
        break;
    }
    case ScrobblerAuth::NETWORK_ERROR:
        QMessageBox::warning(this, tr("Error"), tr("Network error"));
        break;
    case ScrobblerAuth::LASTFM_ERROR:
    default:
        QMessageBox::warning(this, tr("Error"), tr("Unable to register new session"));
    }
}

void ScrobblerSettingsDialog::on_checkButton_lastfm_clicked()
{
    if(!m_ui->sessionLineEdit_lastfm->text().isEmpty())
    {
        m_ui->checkButton_lastfm->setEnabled(false);
        m_lastfmAuth->checkSession(m_ui->sessionLineEdit_lastfm->text());
    }
}

void ScrobblerSettingsDialog::on_checkButton_librefm_clicked()
{
    if(!m_ui->sessionLineEdit_librefm->text().isEmpty())
    {
        m_ui->checkButton_librefm->setEnabled(false);
        m_librefmAuth->checkSession(m_ui->sessionLineEdit_librefm->text());
    }
}

void ScrobblerSettingsDialog::processCheckResponse(int error)
{
    if(sender() == m_lastfmAuth)
        m_ui->checkButton_lastfm->setEnabled(true);
    else if(sender() == m_librefmAuth)
        m_ui->checkButton_librefm->setEnabled(true);
    switch(error)
    {
    case ScrobblerAuth::NO_ERROR:
    {
        QMessageBox::information(this, tr("Message"), tr("Permission granted"));
        if(sender() == m_lastfmAuth)
            m_ui->sessionLineEdit_lastfm->setText(m_lastfmAuth->session());
        else if(sender() == m_librefmAuth)
            m_ui->sessionLineEdit_librefm->setText(m_librefmAuth->session());
        break;
    }
    case ScrobblerAuth::NETWORK_ERROR:
        QMessageBox::warning(this, tr("Error"), tr("Network error"));
        break;
    case ScrobblerAuth::LASTFM_ERROR:
    default:
        QMessageBox::warning(this, tr("Error"), tr("Permission denied"));
    }
}
