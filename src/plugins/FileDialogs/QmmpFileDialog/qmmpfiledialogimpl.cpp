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

#include <QApplication>
#include <QFileInfo>
#include <QStyle>
#include <QSettings>
#include <QMessageBox>
#include <QHeaderView>
#include <QStorageInfo>
#include <QRegularExpression>
#include <qmmp/qmmp.h>
#include "ui_qmmpfiledialog.h"
#include "qmmpfiledialogimpl.h"

#define HISTORY_SIZE 8

// Makes a list of filters from a normal filter string "Image Files (*.png *.jpg)"
static QStringList qt_clean_filter_list(const QString &filter)
{
    //This regular expression has been copied from Qt library
    static const QRegularExpression regexp(u"([a-zA-Z0-9 -]*)\\(([a-zA-Z0-9_.*? +;#\\-\\[\\]@\\{\\}/!<>\\$%&=^~:\\|]*)\\)$"_s);
    QString f = filter;
    QRegularExpressionMatch match = regexp.match(f);
    if(match.hasMatch())
        f = match.captured(2);
    return f.split(QChar::Space, Qt::SkipEmptyParts);
}

QmmpFileDialogImpl::QmmpFileDialogImpl(QWidget *parent, Qt::WindowFlags f) : QDialog(parent, f),
    m_ui(new Ui::QmmpFileDialog)
{
    m_ui->setupUi(this);
    setAttribute(Qt::WA_QuitOnClose, false);
    m_model = new QFileSystemModel(this);
    m_model->setNameFilterDisables(false);
    m_model->setReadOnly(true);

    m_ui->fileListView->setModel(m_model);
    m_ui->treeView->setModel(m_model);
    m_ui->treeView->setSortingEnabled(true);
    m_ui->treeView->setItemsExpandable(false);
    m_ui->treeView->header()->setSortIndicator(0, Qt::AscendingOrder);
    m_ui->treeView->header()->setStretchLastSection (false);
    m_ui->listToolButton->setChecked(true);
    m_ui->upToolButton->setIcon(qApp->style()->standardIcon(QStyle::SP_ArrowUp));
    m_ui->listToolButton->setIcon(qApp->style()->standardIcon(QStyle::SP_FileDialogListView));
    m_ui->closeOnAddToolButton->setIcon(qApp->style()->standardIcon(QStyle::SP_DialogOkButton));
    m_ui->detailsToolButton->setIcon(qApp->style()->standardIcon(QStyle::SP_FileDialogDetailedView));
    connect(m_ui->fileListView->selectionModel(), &QItemSelectionModel::selectionChanged, this, &QmmpFileDialogImpl::updateSelection);
    connect(m_ui->treeView->selectionModel(), &QItemSelectionModel::selectionChanged, this, &QmmpFileDialogImpl::updateSelection);
    PathCompleter* completer = new PathCompleter(m_model, m_ui->fileListView, this);
    m_ui->fileNameLineEdit->setCompleter(completer);

    QSettings settings;
    m_ui->closeOnAddToolButton->setChecked(settings.value(u"QMMPFileDialog/close_on_add"_s, false).toBool());
    restoreGeometry(settings.value(u"QMMPFileDialog/geometry"_s).toByteArray());
    m_history = settings.value(u"QMMPFileDialog/history"_s).toStringList();
    m_ui->lookInComboBox->addItems(m_history);
    m_ui->lookInComboBox->setMaxCount(HISTORY_SIZE);
    QCompleter* dir_completer = new QCompleter (m_model, this);
    m_ui->lookInComboBox->setCompleter(dir_completer);

    if(qApp->style()->styleHint(QStyle::SH_DialogButtonBox_ButtonsHaveIcons, nullptr, this))
    {
        m_ui->addPushButton->setIcon(qApp->style()->standardIcon(QStyle::SP_DialogOpenButton));
        m_ui->closePushButton->setIcon(qApp->style()->standardIcon(QStyle::SP_DialogCloseButton));
    }

    m_ui->splitter->setStretchFactor(0, 0);
    m_ui->splitter->setStretchFactor(1, 10);
    m_ui->splitter->setSizes(QList<int>{ 150, width() - 150 });
    m_ui->splitter->restoreState(settings.value(u"QMMPFileDialog/splitter_state"_s).toByteArray());
}

QmmpFileDialogImpl::~QmmpFileDialogImpl()
{
    delete m_ui;
}

QStringList QmmpFileDialogImpl::selectedFiles() const
{
    QStringList l;
    if(m_mode == FileDialog::SaveFile)
    {
        l << m_model->filePath(m_ui->fileListView->rootIndex()) + QLatin1Char('/') + m_ui->fileNameLineEdit->text();
        qCDebug(plugin) << l.constFirst();
    }
    else
    {
        const QModelIndexList ml = m_ui->fileListView->selectionModel()->selectedIndexes();
        for(const QModelIndex &i : std::as_const(ml))
            l << m_model->filePath(i);
    }
    return l;
}

void QmmpFileDialogImpl::on_mountPointsListWidget_itemClicked(QListWidgetItem *item)
{
    m_ui->lookInComboBox->setEditText(item->data(Qt::UserRole).toString());
    on_lookInComboBox_textActivated(item->data(Qt::UserRole).toString());
}

void QmmpFileDialogImpl::on_lookInComboBox_textActivated(const QString &path)
{
    if(QFileInfo::exists(path))
    {
        m_ui->fileListView->setRootIndex(m_model->index(path));
        m_ui->treeView->setRootIndex(m_model->index(path));
        m_model->setRootPath(path);
    }
}

void QmmpFileDialogImpl::on_upToolButton_clicked()
{
#ifndef Q_OS_WIN
    if(!m_model->parent(m_ui->fileListView->rootIndex()).isValid())
        return;
#endif
    m_ui->fileListView->setRootIndex(m_model->parent(m_ui->fileListView->rootIndex()));
    m_ui->treeView->setRootIndex(m_ui->fileListView->rootIndex());
    m_ui->lookInComboBox->setEditText(m_model->filePath(m_ui->fileListView->rootIndex()));
    m_ui->fileListView->selectionModel()->clear ();
    m_model->setRootPath(m_model->filePath(m_ui->treeView->rootIndex()));
}

void QmmpFileDialogImpl::on_treeView_doubleClicked(const QModelIndex &index)
{
    if(index.isValid())
    {
        QFileInfo info = m_model->fileInfo(index);
        if(info.isDir())
        {
            m_ui->treeView->setRootIndex(index);
            m_ui->lookInComboBox->setEditText(m_model->filePath(index));
            m_ui->treeView->selectionModel()->clear();
            m_ui->fileListView->setRootIndex(index);
            m_ui->fileListView->selectionModel()->clear();
            m_model->setRootPath(m_model->filePath(index));
        }
        else
        {
            const QString path = m_model->filePath(index);
            addToHistory(path);
            addFiles({ path });
        }
    }
}

void QmmpFileDialogImpl::on_fileListView_doubleClicked(const QModelIndex& index)
{
    if(index.isValid())
    {
        QFileInfo info = m_model->fileInfo(index);
        if(info.isDir())
        {
            m_ui->fileListView->setRootIndex(index);
            m_ui->lookInComboBox->setEditText(m_model->filePath(index));
            m_ui->fileListView->selectionModel()->clear();
            m_ui->treeView->setRootIndex(index);
            m_ui->treeView->selectionModel()->clear();
            m_model->setRootPath(m_model->filePath(index));
        }
        else
        {
            const QString path = m_model->filePath(index);
            addToHistory(path);
            addFiles({ path });
        }
    }
}

void QmmpFileDialogImpl::on_fileNameLineEdit_returnPressed()
{
    qCWarning(plugin, "TODO: %s    %d", __FILE__, __LINE__);
}

void QmmpFileDialogImpl::on_fileNameLineEdit_textChanged (const QString &text)
{
    if(m_mode == FileDialog::SaveFile)
    {
        m_ui->addPushButton->setEnabled(!text.isEmpty());
        return;
    }
    QModelIndex index;

    if(text.startsWith(QLatin1Char('/')))
        index = m_model->index(text);
    else
        index = m_model->index(m_model->filePath(m_ui->fileListView->rootIndex()) + QLatin1Char('/') + text);
    if(!index.isValid() || !m_ui->fileNameLineEdit->hasFocus())
        return;

    m_ui->fileListView->selectionModel()->clear();
    m_ui->fileListView->selectionModel()->select(index, QItemSelectionModel::Select);
}

void QmmpFileDialogImpl::on_addPushButton_clicked()
{
    QStringList l;
    if(m_mode != FileDialog::SaveFile)
    {
        QModelIndexList ml;
        if(m_ui->stackedWidget->currentIndex() == 0)
            ml = m_ui->fileListView->selectionModel()->selectedIndexes();
        else
            ml = m_ui->treeView->selectionModel()->selectedIndexes();
       for(const QModelIndex &i : std::as_const(ml))
        {
            if(!l.contains(m_model->filePath(i)))
                l << m_model->filePath(i);
        }
        if(!l.isEmpty())
        {
            addToHistory(l.constFirst());
            addFiles(l);
            return;
        }
    }
    else
    {
        l << m_model->filePath(m_ui->fileListView->rootIndex()) +  QLatin1Char('/') + m_ui->fileNameLineEdit->text();
        addFiles(l);
    }
}

void QmmpFileDialogImpl::setModeAndMask(const QString &d, FileDialog::Mode m, const QStringList &mask)
{
    m_mode = m;
    m_ui->fileListView->clearSelection();
    m_ui->treeView->clearSelection();
    m_ui->fileTypeComboBox->clear();
    m_ui->addPushButton->setEnabled(false);
    m_ui->addPushButton->setText(tr("Add"));

    QString fileName;
    QString path = d;

    if(m == FileDialog::SaveFile)
    {
        if(path.endsWith(QLatin1Char('/')))
            path.remove(path.size() - 1, 1);
        path = path.left(path.lastIndexOf(QLatin1Char('/')));
        fileName = d.section(QLatin1Char('/'), -1);
        m_ui->fileNameLineEdit->setText(fileName);
        m_ui->addPushButton->setEnabled(!fileName.isEmpty());
        m_ui->addPushButton->setText(tr("Save"));
    }
    if(!QFile::exists(path))
        path = QDir::home ().path ();
    if(m_model->filePath(m_ui->fileListView->rootIndex()) != path)
    {
        m_ui->fileListView->setRootIndex(m_model->index(path));
        m_ui->treeView->setRootIndex(m_model->index(path));
        m_model->setRootPath(path);
    }

    if(m == FileDialog::AddDirs || m == FileDialog::AddDir)
    {
        m_model->setFilter(QDir::AllDirs | QDir::NoDotAndDotDot); //dirs only
        m_ui->fileTypeComboBox->addItem(tr("Directories"));
        m_ui->fileTypeComboBox->setEnabled(false);
    }
    else
    {
        m_model->setFilter(QDir::AllDirs | QDir::Files | QDir::NoDotAndDotDot);
        m_ui->fileTypeComboBox->setEnabled(true);
        m_ui->fileTypeComboBox->addItems(mask);
        on_fileTypeComboBox_activated(0);
    }

    //set selection mode
    if(m == FileDialog::AddDir ||  m == FileDialog::AddFile || m == FileDialog::SaveFile)
    {
        m_ui->fileListView->setSelectionMode(QAbstractItemView::SingleSelection);
        m_ui->treeView->setSelectionMode(QAbstractItemView::SingleSelection);
    }
    else
    {
        m_ui->fileListView->setSelectionMode (QAbstractItemView::ExtendedSelection);
        m_ui->treeView->setSelectionMode (QAbstractItemView::ExtendedSelection);
    }

    m_ui->lookInComboBox->setEditText(QDir::cleanPath(path));
}

void QmmpFileDialogImpl::loadMountedVolumes()
{
    m_ui->mountPointsListWidget->clear();
    for(const QStorageInfo &i : QStorageInfo::mountedVolumes())
    {
        if(i.fileSystemType() == "tmpfs"_ba)
            continue;
        QString name = i.displayName();
        name.replace(u"\\x20"_s, u" "_s);
        QListWidgetItem *item = new QListWidgetItem(name);
        item->setData(Qt::UserRole, i.rootPath());
        item->setToolTip(i.rootPath());
        item->setIcon(style()->standardIcon(QStyle::SP_DirIcon));
        m_ui->mountPointsListWidget->addItem(item);
    }
}

void QmmpFileDialogImpl::on_listToolButton_toggled(bool yes)
{
    if(yes)
    {
        m_ui->stackedWidget->setCurrentIndex(0);
    }
}

void QmmpFileDialogImpl::on_detailsToolButton_toggled(bool yes)
{
    if(yes)
    {
        m_ui->stackedWidget->setCurrentIndex(1);
    }
}

void QmmpFileDialogImpl::on_fileTypeComboBox_activated(int index)
{
    m_model->setNameFilters(qt_clean_filter_list(m_ui->fileTypeComboBox->itemText(index)));
}

void QmmpFileDialogImpl::hideEvent (QHideEvent *event)
{
    QSettings settings;
    settings.setValue(u"QMMPFileDialog/close_on_add"_s, m_ui->closeOnAddToolButton->isChecked());
    settings.setValue(u"QMMPFileDialog/geometry"_s, saveGeometry());
    settings.setValue(u"QMMPFileDialog/history"_s, m_history);
    settings.setValue(u"QMMPFileDialog/splitter_state"_s, m_ui->splitter->saveState());
    QWidget::hideEvent(event);
}

void QmmpFileDialogImpl::updateSelection ()
{
    QModelIndexList ml;
    if(m_ui->stackedWidget->currentIndex() == 0)
        ml = m_ui->fileListView->selectionModel()->selectedIndexes();
    else
        ml = m_ui->treeView->selectionModel()->selectedIndexes();
    QStringList l;
    QStringList files;
    for(const QModelIndex &i : std::as_const(ml))
    {
        if(!l.contains(m_model->filePath(i).section(QLatin1Char('/'), -1)))
        {
            files << m_model->filePath(i);
            l << m_model->filePath(i).section(QLatin1Char('/'), -1);
        }
    }

    if(!l.isEmpty())
    {
        QString str;
        if(l.size() == 1)
            str = l.constFirst();
        else
        {
            str = l.join (u"\" \""_s);
            str.append(QLatin1Char('"'));
            str.prepend(QLatin1Char('"'));
        }
        if(!m_ui->fileNameLineEdit->hasFocus())
            m_ui->fileNameLineEdit->setText(str);
        if(m_mode == FileDialog::AddFiles || m_mode == FileDialog::AddFile/* || FileDialog::SaveFile*/)
        {
            m_ui->addPushButton->setEnabled(true);
            for(const QString &file : std::as_const(files))
            {
                if(QFileInfo(file).isDir())
                {
                    m_ui->addPushButton->setEnabled(false);
                    break;
                }
            }
        }
        else
        {
            m_ui->addPushButton->setEnabled(true);
        }
    }
    else
    {
        m_ui->fileNameLineEdit->clear();
        m_ui->addPushButton->setEnabled(false);
    }
}

void QmmpFileDialogImpl::addToHistory(const QString &path)
{
    QString path_copy = path;
    if(path_copy.endsWith(QLatin1Char('/')))
        path_copy.remove(path.size() - 1, 1);
    QString dir_path = path.left(path_copy.lastIndexOf(QLatin1Char('/')));

    m_history.removeAll(dir_path);
    m_history.prepend(dir_path);

    while(m_history.size() > HISTORY_SIZE)
        m_history.removeLast();

    m_ui->lookInComboBox->clear();
    m_ui->lookInComboBox->addItems(m_history);
}

void QmmpFileDialogImpl::addFiles(const QStringList &list)
{
    if(list.isEmpty())
        return;
    if(!isModal())
    {
        emit filesSelected(list);
        if(m_ui->closeOnAddToolButton->isChecked())
            reject();
    }
    else if(m_mode == FileDialog::SaveFile)
    {
        //check file extension
        QString f_name = m_ui->fileNameLineEdit->text();
        bool contains = false;
        for(const QString &str : qt_clean_filter_list(m_ui->fileTypeComboBox->currentText()))
        {
            QRegularExpression regExp(QRegularExpression::wildcardToRegularExpression(str));
            if(f_name.contains(regExp))
            {
                contains = true;
                break;
            }
        }
        //add extensio to file name
        if(!contains)
        {
            QString ext = qt_clean_filter_list(m_ui->fileTypeComboBox->currentText()).constFirst();
            ext.remove(QLatin1Char('*'));
            if(!ext.isEmpty() && ext != "."_L1)
            {
                f_name.append(ext);
                qCDebug(plugin) << "added file extension";
                m_ui->fileNameLineEdit->setText(f_name);
                return;
            }
        }
        QFileInfo info(list[0]);

        if(info.exists())
        {
            if(QMessageBox::question (this, windowTitle(), tr("%1 already exists.\nDo you want to replace it?")
                                      .arg(m_ui->fileNameLineEdit->text()),
                                      QMessageBox::Ok | QMessageBox::Cancel) == QMessageBox::Ok)
                accept();
            else
                return;

        }
        accept();
    }
    else
        accept();
}
