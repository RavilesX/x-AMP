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

#ifndef ADDURLDIALOG_P_H
#define ADDURLDIALOG_P_H

#include <QDialog>
#include <QPointer>
#include <QUrl>

class QNetworkAccessManager;
class QNetworkReply;
class QPushButton;
class PlayListModel;
class PlayListDownloader;

namespace Ui {
class AddUrlDialog;
}

/*! @internal
   @author Vladimir Kuznetsov <vovanec@gmail.com>
 */
class AddUrlDialog : public QDialog
{
    Q_OBJECT
public:
    static void popup(QWidget* parent, PlayListModel *m);

private slots:
    void onFinished(bool ok, const QString &message);

private:
    explicit AddUrlDialog(QWidget *parent);
    ~AddUrlDialog();
    void accept() override;
    void setModel(PlayListModel *m);
    void addToHistory(const QString &path);
    static QPointer<AddUrlDialog> m_instance;
    Ui::AddUrlDialog *m_ui;
    PlayListModel *m_model;
    PlayListDownloader *m_downloader;
    QStringList m_history;
    QPushButton *m_addButton;
};
#endif //ADDURLDIALOG_P_H
