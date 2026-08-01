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
#include <QDir>
#include <QMessageBox>
#include <QClipboard>
#include <QPushButton>
#include <qmmp/metadatamanager.h>
#include <qmmp/qmmp.h>
#include "playlistmodel.h"
#include "playlistdownloader.h"
#include "qmmpuisettings.h"
#include "ui_addurldialog.h"
#include "addurldialog_p.h"

#define HISTORY_SIZE 10

AddUrlDialog::AddUrlDialog(QWidget *parent) : QDialog(parent), m_ui(new Ui::AddUrlDialog)
{
    m_ui->setupUi(this);
    setAttribute(Qt::WA_DeleteOnClose);
    setAttribute(Qt::WA_QuitOnClose, false);

    m_addButton = m_ui->buttonBox->addButton(tr("&Add"), QDialogButtonBox::AcceptRole);
    m_addButton->setIcon(QIcon::fromTheme(u"list-add"_s));

    QSettings settings;
    m_history = settings.value(u"URLDialog/history"_s).toStringList();
    m_ui->urlComboBox->addItems(m_history);
    m_downloader = new PlayListDownloader(this);
    connect(m_downloader, &PlayListDownloader::finished, this, &AddUrlDialog::onFinished);

    if(QmmpUiSettings::instance()->useClipboard())
    {
        QUrl url(QApplication::clipboard()->text().trimmed());
        if(url.isValid() && (MetaDataManager::instance()->protocols().contains(url.scheme()) ||
                             MetaDataManager::hasMatch(MetaDataManager::instance()->regExps(), url.toString())))
        {
            m_ui->urlComboBox->setEditText(QApplication::clipboard()->text().trimmed());
        }
    }
}

AddUrlDialog::~AddUrlDialog()
{
    while (m_history.size() > HISTORY_SIZE)
        m_history.removeLast();
    QSettings settings;
    settings.setValue(u"URLDialog/history"_s, m_history);
    delete m_ui;
}

QPointer<AddUrlDialog> AddUrlDialog::m_instance = nullptr;

void AddUrlDialog::popup(QWidget *parent, PlayListModel *m)
{
    if (!m_instance)
    {
        m_instance = new AddUrlDialog(parent);
        m_instance->setModel(m);
    }
    m_instance->show();
    m_instance->raise();
}

void AddUrlDialog::onFinished(bool ok, const QString &message)
{
    if(ok)
    {
        QDialog::accept();
    }
    else
    {
        QMessageBox::warning(this, tr("Error"), message);
        m_addButton->setEnabled(true);
    }
}

void AddUrlDialog::accept()
{
    m_addButton->setEnabled(false);
    if(m_ui->urlComboBox->currentText().isEmpty())
    {
         QDialog::accept();
         return;
    }

    QString path = m_ui->urlComboBox->currentText().trimmed();

    if(QFile::exists(path)) //is local file
    {
        m_model->addPath(path);
        addToHistory(path);
        QDialog::accept();
        return;
    }

    if(!path.startsWith(u"http://"_s) && !path.contains(u"://"_s))
        path.prepend(u"http://"_s);

    if(MetaDataManager::hasMatch(MetaDataManager::instance()->regExps(), path))
    {
        addToHistory(path);
        m_model->addPath(path);
        QDialog::accept();
        return;
    }

    if (path.startsWith(u"http://"_s) || path.startsWith(u"https://"_s)) //try to download playlist
    {
        m_downloader->start(QUrl(path), m_model);
        addToHistory(path);
        return;
    }

    if(!MetaDataManager::instance()->protocols().contains(QUrl(path).scheme()))
    {
        qCWarning(core, "unsupported protocol");
        QDialog::reject();
        return;
    }

    addToHistory(path);
    m_model->addPath(path);
    QDialog::accept();
}

void AddUrlDialog::setModel(PlayListModel *m)
{
    m_model = m;
}

void AddUrlDialog::addToHistory(const QString &path)
{
    m_history.removeAll(path);
    m_history.prepend(path);
}
