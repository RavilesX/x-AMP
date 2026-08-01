/***************************************************************************
 *   Copyright (C) 2009-2026 by Ilya Kotov                                 *
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

#ifndef SKINNEDPLAYLISTBROWSER_H
#define SKINNEDPLAYLISTBROWSER_H

#include <QDialog>

class PlayListManager;
class QStandardItemModel;
class QSortFilterProxyModel;
class QStandardItem;

namespace Ui {
class SkinnedPlayListBrowser;
}

/**
    @author Ilya Kotov <forkotov02@ya.ru>
*/
class SkinnedPlayListBrowser : public QDialog
{
Q_OBJECT
public:
    explicit SkinnedPlayListBrowser(PlayListManager *manager, QWidget *parent = nullptr);
    ~SkinnedPlayListBrowser();

private slots:
    void updateList();
    void on_filterLineEdit_textChanged(const QString &str);
    void on_listView_activated(const QModelIndex &index);
    void updatePlayListName(QStandardItem *item);
    void updateCurrentRow(const QModelIndex &index, const QModelIndex &);
    void rename();
    void on_deleteButton_clicked();
    void on_downButton_clicked();
    void on_upButton_clicked();

private:
    bool eventFilter(QObject *o, QEvent *e) override;
    Ui::SkinnedPlayListBrowser *m_ui;
    PlayListManager *m_pl_manager;
    QStandardItemModel* m_listModel;
    QSortFilterProxyModel* m_proxyModel;
};

#endif // SKINNEDPLAYLISTBROWSER_H
