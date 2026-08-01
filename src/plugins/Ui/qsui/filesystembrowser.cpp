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

#include <QFileSystemModel>
#include <QSortFilterProxyModel>
#include <QListView>
#include <QTreeView>
#include <QVBoxLayout>
#include <QSettings>
#include <QAction>
#include <QApplication>
#include <QLineEdit>
#include <QIcon>
#include <QMenu>
#include <qmmp/metadatamanager.h>
#include <qmmpui/playlistmanager.h>
#include <qmmpui/filedialog.h>
#include <qmmpui/uihelper.h>
#include <qmmp/soundcore.h>
#include <qmmp/qmmp.h>
#include "elidinglabel.h"
#include "filesystembrowser.h"

class FileSystemFilterProxyModel : public QSortFilterProxyModel
{
public:
    explicit FileSystemFilterProxyModel(QObject *parent) : QSortFilterProxyModel(parent) {}

protected:
    virtual bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const override
    {
        QFileSystemModel *sm = qobject_cast<QFileSystemModel*>(sourceModel());
        if (source_parent == sm->index(sm->rootPath()))
            return QSortFilterProxyModel::filterAcceptsRow(source_row, source_parent);

        return true;
    }
};

FileSystemBrowser::FileSystemBrowser(QWidget *parent) :
    QWidget(parent)
{
    m_treeView = new QTreeView(this);
    m_treeView->setFrameStyle(QFrame::NoFrame);
    m_treeView->setDragEnabled(true);
    m_treeView->setSelectionMode(QAbstractItemView::ExtendedSelection);
    connect(m_treeView, &QTreeView::activated, this, &FileSystemBrowser::onListViewActivated);

    m_label = new Utils::ElidingLabel(this);
    m_label->setContentsMargins(5, 5, 5, 0);
    m_label->setMargin(0);

    m_filterLineEdit = new QLineEdit(this);
    m_filterLineEdit->setContentsMargins(5, 5, 5, 0);
    m_filterLineEdit->setClearButtonEnabled(true);
    m_filterLineEdit->setVisible(false);

    QVBoxLayout *layout = new QVBoxLayout();
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(m_label);
    layout->addWidget(m_filterLineEdit);
    layout->addWidget(m_treeView);
    setLayout(layout);

    m_fileSystemModel = new QFileSystemModel(this);
    m_fileSystemModel->setReadOnly(true);
    m_fileSystemModel->setNameFilterDisables(false);
    m_fileSystemModel->setFilter(QDir::AllDirs | QDir::Files | QDir::NoDot);

    m_proxyModel = new FileSystemFilterProxyModel(this);
    m_proxyModel->setDynamicSortFilter(true);
    m_proxyModel->setFilterCaseSensitivity(Qt::CaseInsensitive);
    m_proxyModel->setSourceModel(m_fileSystemModel);
    m_treeView->setModel(m_proxyModel);
    m_treeView->setColumnHidden(1, true);
    m_treeView->setColumnHidden(2, true);
    m_treeView->setColumnHidden(3, true);
    m_treeView->setHeaderHidden(true);
    m_treeView->setUniformRowHeights(true);
    m_treeView->setRootIsDecorated(false);

    setContextMenuPolicy(Qt::ActionsContextMenu);
    QAction *addToPlayListAction = new QAction(QIcon::fromTheme(u"list-add"_s), tr("Add to Playlist"), this);
    QAction *replacePlayListAction = new QAction(QIcon::fromTheme(u"media-eject"_s), tr("Replace Playlist"), this);
    QAction *selectDirAction = new QAction(QIcon::fromTheme(u"folder"_s), tr("Change Directory"), this);
    QAction *separatorAction = new QAction(this);
    separatorAction->setSeparator(true);
    m_treeModeAction = new QAction(tr("Tree View Mode"), this);
    m_treeModeAction->setCheckable(true);
    m_showFilterAction = new QAction(tr("Quick Search"), this);
    m_showFilterAction->setCheckable(true);
    QMenu *sortMenu = new QMenu(this);
    sortMenu->addAction(tr("By Name"), this, [this]() { m_fileSystemModel->sort(0); } );
    sortMenu->addAction(tr("By Size"), this, [this]() { m_fileSystemModel->sort(1); } );
    sortMenu->addAction(tr("By Type"), this, [this]() { m_fileSystemModel->sort(2); } );
    sortMenu->addAction(tr("By Date"), this, [this]() { m_fileSystemModel->sort(3); } );
    m_sortAction = sortMenu->menuAction();
    m_sortAction->setIcon(QIcon::fromTheme(u"view-sort-ascending"_s));
    m_sortAction->setText(tr("Sort"));

    const QList<QAction *> contextMenuActions = {
        addToPlayListAction, replacePlayListAction, selectDirAction, separatorAction,
        m_treeModeAction, m_showFilterAction, m_sortAction
    };

    addActions(contextMenuActions);
    connect(addToPlayListAction, &QAction::triggered, this, &FileSystemBrowser::addToPlayList);
    connect(replacePlayListAction, &QAction::triggered, this, &FileSystemBrowser::replacePlayList);
    connect(selectDirAction, &QAction::triggered, this, &FileSystemBrowser::selectDirectory);
    connect(m_treeModeAction, &QAction::triggered, this, &FileSystemBrowser::setTreeViewMode);
    connect(m_showFilterAction, &QAction::toggled, m_filterLineEdit, &QLineEdit::setVisible);
    connect(m_showFilterAction, &QAction::triggered, m_filterLineEdit, &QLineEdit::clear);
    connect(m_filterLineEdit, &QLineEdit::textChanged, this, &FileSystemBrowser::onFilterLineEditTextChanged);

    readSettings();
}

FileSystemBrowser::~FileSystemBrowser()
{
    QSettings settings;
    settings.beginGroup(u"Simple"_s);
    settings.setValue(u"fsbrowser_current_dir"_s, m_fileSystemModel->rootDirectory().canonicalPath());
    settings.setValue(u"fsbrowser_quick_search"_s, m_showFilterAction->isChecked());
    settings.setValue(u"fsbrowser_tree_mode"_s, m_treeModeAction->isChecked());
    settings.endGroup();
}

void FileSystemBrowser::readSettings()
{
    QSettings settings;
    settings.beginGroup(u"Simple"_s);
    if(!m_update)
    {
        m_update = true;
        setCurrentDirectory(settings.value(u"fsbrowser_current_dir"_s, QDir::homePath()).toString());
        m_showFilterAction->setChecked(settings.value(u"fsbrowser_quick_search"_s, false).toBool());
        setTreeViewMode(settings.value(u"fsbrowser_tree_mode"_s, false).toBool());
        m_treeModeAction->setChecked(m_treeView->rootIsDecorated());
    }
    settings.endGroup();
    m_fileSystemModel->setNameFilters(MetaDataManager::instance()->nameFilters());
}

void FileSystemBrowser::onListViewActivated(const QModelIndex &index)
{
    if (!index.isValid())
        return;

    QModelIndex sourceIndex = m_proxyModel->mapToSource(index);

    QString name = m_fileSystemModel->fileName(sourceIndex);

    if(!m_treeView->rootIsDecorated())
    {
        if(name == QLatin1String(".."))
        {
            setCurrentDirectory(m_fileSystemModel->fileInfo(sourceIndex).absoluteFilePath());
        }
        else if(m_fileSystemModel->isDir(sourceIndex))
        {
            QFileInfo info = m_fileSystemModel->fileInfo(sourceIndex);
            if(info.isExecutable() && info.isReadable())
                setCurrentDirectory(m_fileSystemModel->filePath(sourceIndex));
        }
    }
}

void FileSystemBrowser::addToPlayList()
{
     PlayListManager::instance()->selectedPlayList()->addPaths(selectedPaths());
}

void FileSystemBrowser::replacePlayList()
{
    QStringList paths = selectedPaths();

    if(paths.isEmpty())
        return;

    SoundCore *core = SoundCore::instance();
    PlayListManager *manager = PlayListManager::instance();
    PlayListModel *model = PlayListManager::instance()->selectedPlayList();

    bool play = (core->state() == Qmmp::Playing || core->state() == Qmmp::Paused || core->state() == Qmmp::Buffering) &&
            model == manager->currentPlayList();


    if(play)
    {
        UiHelper::instance()->replaceAndPlay(paths);
    }
    else
    {
        manager->selectedPlayList()->clear();
        manager->selectedPlayList()->addPaths(paths);
    }
}

void FileSystemBrowser::selectDirectory()
{
    QString dir = FileDialog::getExistingDirectory(qApp->activeWindow(),
                                                   tr("Select Directory"), m_fileSystemModel->rootDirectory().canonicalPath());
    if(!dir.isEmpty())
        setCurrentDirectory(dir);
}

void FileSystemBrowser::onFilterLineEditTextChanged(const QString &str)
{
    m_proxyModel->setFilterFixedString(str);
}

void FileSystemBrowser::setTreeViewMode(bool enabled)
{
    m_treeView->setRootIsDecorated(enabled);
    if(enabled)
        m_fileSystemModel->setFilter(m_fileSystemModel->filter() | QDir::NoDotDot);
    else
        m_fileSystemModel->setFilter(m_fileSystemModel->filter() & ~QDir::NoDotDot);

    int s = style()->pixelMetric(enabled ? QStyle::PM_SmallIconSize : QStyle::PM_ListViewIconSize);
    m_treeView->setIconSize(QSize(s, s));
}

QStringList FileSystemBrowser::selectedPaths() const
{
    QStringList paths;

    for(const QModelIndex &index : m_treeView->selectionModel()->selectedIndexes())
    {
        if(!index.isValid() || index.column() != 0)
            continue;

        QModelIndex sourceIndex = m_proxyModel->mapToSource(index);
        QString name = m_fileSystemModel->fileName(sourceIndex);
        if(name == QLatin1String(".."))
            continue;

        paths << m_fileSystemModel->filePath(sourceIndex);
    }

    return paths;
}

void FileSystemBrowser::setCurrentDirectory(const QString &path)
{
    if(path.isEmpty())
        return;

    m_filterLineEdit->clear();

    QModelIndex index = m_fileSystemModel->setRootPath(QDir(path).exists() ? path : QDir::homePath());
    if(index.isValid())
    {
        m_treeView->setRootIndex(m_proxyModel->mapFromSource(index));
        m_label->setText(QDir(m_fileSystemModel->rootPath()).dirName());
    }
    else
        m_label->clear();
}
