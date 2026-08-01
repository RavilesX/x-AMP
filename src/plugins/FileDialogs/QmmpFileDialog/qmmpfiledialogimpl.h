/**************************************************************************
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

#ifndef QMMPFILEDIALOGIMPL_H
#define QMMPFILEDIALOGIMPL_H

#include <QDialog>
#include <QCompleter>
#include <QAbstractItemView>
#include <qmmpui/filedialog.h>
#include <QFileSystemModel>

class QListWidgetItem;

namespace Ui {
class QmmpFileDialog;
}

class QmmpFileDialogImpl : public QDialog
{
    Q_OBJECT
public:
    QmmpFileDialogImpl(QWidget *parent = nullptr, Qt::WindowFlags f = Qt::WindowFlags());

    ~QmmpFileDialogImpl();

    void setModeAndMask(const QString &d, FileDialog::Mode m, const QStringList &mask = QStringList());
    void loadMountedVolumes();
    QStringList selectedFiles() const;

signals:
    void filesSelected(const QStringList&, bool play = false);

private slots:
    void on_mountPointsListWidget_itemClicked(QListWidgetItem *item);
    void on_lookInComboBox_textActivated(const QString &path);
    void on_upToolButton_clicked();
    void on_fileListView_doubleClicked(const QModelIndex &index);
    void on_treeView_doubleClicked(const QModelIndex &index);
    void on_fileNameLineEdit_returnPressed();
    void on_fileNameLineEdit_textChanged(const QString &text);
    void on_addPushButton_clicked();
    void on_listToolButton_toggled(bool);
    void on_detailsToolButton_toggled(bool);
    void on_fileTypeComboBox_activated(int);
    void updateSelection();

private:
    void hideEvent(QHideEvent *event) override;
    void addToHistory(const QString &path);
    void addFiles(const QStringList &list);

    Ui::QmmpFileDialog *m_ui;
    FileDialog::Mode m_mode = FileDialog::AddFiles;
    QFileSystemModel *m_model;
    QStringList m_history;
};

class PathCompleter : public QCompleter
{
    Q_OBJECT
public:
    PathCompleter(QAbstractItemModel *model, QAbstractItemView *itemView, QObject *parent = nullptr) : QCompleter(model, parent)
    {
        m_itemView = itemView;
    }


    inline QString pathFromIndex(const QModelIndex &index) const override
    {
        const QFileSystemModel *dirModel = static_cast<const QFileSystemModel *>(model());
        QString currentLocation = dirModel->filePath(m_itemView->rootIndex());
        QString path = dirModel->filePath(index);
        if (path.startsWith(currentLocation))
        {
            path = path.mid(currentLocation.length() + 1);
        }
        return path;
    }


    inline QStringList splitPath(const QString &path) const override
    {
        if (path.isEmpty())
            return QStringList(completionPrefix());
        QStringList parts;
        if (!path.startsWith(QDir::separator()))
        {
            const QFileSystemModel *dirModel = static_cast<const QFileSystemModel *>(model());
            QString currentLocation = QDir::toNativeSeparators(dirModel->filePath(m_itemView->rootIndex()));
            parts = QCompleter::splitPath(currentLocation);
        }
        parts << QCompleter::splitPath(path);
        return parts;
    }
private:
    QAbstractItemView *m_itemView;
};


#endif //QMMPFILEDIALOGIMPL_H
