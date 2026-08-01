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

#include <QDialog>
#include <QMenu>
#include <QWidget>
#include <QAction>
#include <QSettings>
#include <QApplication>
#include <QMessageBox>
#include <QFileInfo>
#include <algorithm>
#include <qmmp/metadatamanager.h>
#include "filedialog.h"
#include "playlistparser.h"
#include "playlistmanager.h"
#include "qmmpuisettings.h"
#include "general.h"
#include "generalfactory.h"
#include "jumptotrackdialog_p.h"
#include "aboutdialog_p.h"
#include "addurldialog_p.h"
#include "mediaplayer.h"
#include "uihelper.h"

class UiHelperPrivate
{
    Q_DECLARE_PUBLIC(UiHelper)
public:
    UiHelperPrivate(UiHelper *helper) : q_ptr(helper)
    {
        if(instance)
            qCFatal(core) << "only one instance is allowed";
        instance = helper;
    };

    ~UiHelperPrivate()
    {
        instance = nullptr;
    }

private:
    UiHelper *q_ptr;

    void addSelectedFiles(const QStringList &files, bool play)
    {
        if(files.isEmpty() || !PlayListManager::instance()->playLists().contains(model))
            return;
        if(play)
        {
            PlayListManager::instance()->selectPlayList(model);
            q_ptr->replaceAndPlay(files);
        }
        else
        {
            model->addPaths(files);
        }
    }

    void playSelectedFiles(const QStringList &files)
    {
        addSelectedFiles(files, true);
    }

    void disconnectPl()
    {
        Q_Q(UiHelper);
        while(!plConnections.isEmpty())
            q->disconnect(plConnections.takeFirst());
    }

    QMap<GeneralFactory*, General*> generals;
    struct MenuData
    {
        QPointer<QMenu> menu;
        QPointer<QAction> before;
        QList<QAction*> actions;
        bool autoHide = false;
    };
    QMap<UiHelper::MenuType, MenuData> menus;
    QString lastDir;
    QPointer<JumpToTrackDialog> jumpDialog;
    PlayListModel *model = nullptr;
    QList<QMetaObject::Connection> plConnections;
    static UiHelper *instance;
};

UiHelper *UiHelperPrivate::instance = nullptr;

UiHelper::UiHelper(QObject *parent) :
    QObject(parent),
    d_ptr(new UiHelperPrivate(this))
{
    General::create(parent);
    QSettings settings;
    d_ptr->lastDir = settings.value(u"General/last_dir"_s, QDir::homePath()).toString(); //last directory
}

UiHelper::~UiHelper()
{
    QSettings settings;
    settings.setValue(u"General/last_dir"_s, d_ptr->lastDir);
    delete d_ptr;
}

bool UiHelper::visibilityControl() const
{
    const QList<GeneralFactory *> factories = General::enabledFactories();
    return std::any_of(factories.cbegin(), factories.cend(),
                       [](GeneralFactory *factory){ return factory->properties().visibilityControl; });
}

void UiHelper::addAction(QAction *action, MenuType type)
{
    Q_D(UiHelper);
    connect(action, &QAction::destroyed, this, [action, this] { removeAction(action); });

    if(!d->menus[type].actions.contains(action))
    {
        d->menus[type].actions.append(action);
        action->setShortcutVisibleInContextMenu(true);
    }
    if(d->menus[type].menu && !d->menus[type].menu->actions().contains(action))
    {
        if(d->menus[type].before)
            d->menus[type].menu->insertAction(d->menus[type].before, action);
        else
            d->menus[type].menu->addAction(action);
        d->menus[type].menu->menuAction()->setVisible(!d->menus[type].autoHide || !d->menus[type].actions.isEmpty());
    }
}

void UiHelper::removeAction(QAction *action)
{
    Q_D(UiHelper);
    for(UiHelperPrivate::MenuData &menuData : d->menus)
    {
        menuData.actions.removeAll(action);
        if(menuData.menu)
        {
            menuData.menu->removeAction(action);
            menuData.menu->menuAction()->setVisible(!menuData.autoHide || !menuData.actions.isEmpty());
        }
    }
}

QList<QAction *> UiHelper::actions(MenuType type)
{
    return d_ptr->menus[type].actions;
}

QMenu *UiHelper::createMenu(MenuType type, const QString &title, bool autoHide, QWidget *parent)
{
    Q_D(UiHelper);
    if(d->menus[type].menu)
    {
        d->menus[type].menu->setTitle(title);
    }
    else
    {
        d->menus[type].menu = new QMenu(title, parent);
        d->menus[type].menu->addActions(d->menus[type].actions);
    }
    d->menus[type].autoHide = autoHide;
    d->menus[type].menu->menuAction()->setVisible(!autoHide || !d->menus[type].actions.isEmpty());
    return d->menus[type].menu;
}

void UiHelper::registerMenu(UiHelper::MenuType type, QMenu *menu, bool autoHide, QAction *before)
{
    Q_D(UiHelper);
    d->menus[type].menu = menu;
    d->menus[type].before = before;
    d->menus[type].autoHide = autoHide;
    if(before)
        d->menus[type].menu->insertActions(before, d->menus[type].actions);
    else
        d->menus[type].menu->addActions(d->menus[type].actions);
    d->menus[type].menu->menuAction()->setVisible(!autoHide || !d->menus[type].actions.isEmpty());
}

void UiHelper::addFiles(QWidget *parent, PlayListModel *model)
{
    Q_D(UiHelper);
    QStringList filters;
    filters << tr("All Supported Bitstreams") +
               QStringLiteral(" (%1)").arg(MetaDataManager::instance()->nameFilters().join(QChar::Space));
    filters << MetaDataManager::instance()->filters();
    d->model = model;
    FileDialog::popup(parent, FileDialog::PlayDirsFiles, &d->lastDir,
                      this, [d](const QStringList &files, bool play) { d->addSelectedFiles(files, play); },
                      tr("Select one or more files to open"), filters.join(u";;"_s));
}

void UiHelper::playFiles(QWidget *parent, PlayListModel *model)
{
    Q_D(UiHelper);
    QStringList filters;
    filters << tr("All Supported Bitstreams") +
               QStringLiteral(" (%1)").arg(MetaDataManager::instance()->nameFilters().join(QChar::Space));
    filters << MetaDataManager::instance()->filters();
    d->model = model;
    FileDialog::popup(parent, FileDialog::AddDirsFiles, &d->lastDir,
                      this, [d](const QStringList &files, bool) { d->addSelectedFiles(files, true); },
                      tr("Select one or more files to play"), filters.join(u";;"_s));

}

void UiHelper::addDirectory(QWidget *parent, PlayListModel *model)
{
    FileDialog::popup(parent, FileDialog::AddDirs, &d_ptr->lastDir,
                      model, [model](const QStringList &paths, bool) { model->addPaths(paths); },
                      tr("Choose a directory"));
}

void UiHelper::addUrl(QWidget *parent, PlayListModel *model)
{
    AddUrlDialog::popup(parent, model);
}

void UiHelper::loadPlayList(QWidget *parent, PlayListModel *model)
{
    Q_D(UiHelper);
    if(PlayListParser::nameFilters().isEmpty())
    {
        qCWarning(core, "There is no registered playlist parsers");
        return;
    }

    QString mask = tr("Playlist Files") + QStringLiteral(" (%1)").arg(PlayListParser::nameFilters().join(QChar::Space));
    //TODO use nonmodal dialog and multiplier playlists
    QString f_path = FileDialog::getOpenFileName(parent, tr("Open Playlist"), d->lastDir, mask);
    if (!f_path.isEmpty())
    {
        if(QmmpUiSettings::instance()->clearPreviousPlayList())
        {
            model->clear();
            model->setName(QFileInfo(f_path).baseName());
        }
        model->loadPlaylist(f_path);
        d->lastDir = QFileInfo(f_path).absoluteDir().path();
    }
}

void UiHelper::savePlayList(QWidget *parent, PlayListModel *model)
{
    Q_D(UiHelper);
    if(PlayListParser::nameFilters().isEmpty())
    {
        qCWarning(core, "There is no registered playlist parsers");
        return;
    }

    QStringList filters;
    filters << tr("Playlist Files") + QStringLiteral(" (%1)").arg(PlayListParser::nameFilters().join(QChar::Space));
    filters << PlayListParser::filters();
    QString selectedFilter = filters.at(1);
    QString f_name = FileDialog::getSaveFileName(parent, tr("Save Playlist"), d->lastDir + QLatin1Char('/') +
                                                 model->name(), filters.join(u";;"_s), &selectedFilter);

    if(f_name.isEmpty())
        return;

    if(!PlayListParser::isPlayList(f_name)) //append selected extension
    {
        QStringList selectedFilters = selectedFilter.section(QLatin1Char('('), 1).remove(QLatin1Char(')')).split(QChar::Space);
        if(selectedFilters.isEmpty())
            return;

        QString ext = selectedFilters.first().remove(QLatin1Char('*')); //use first extension
        f_name.append(ext);

        QFileInfo info(f_name);

        if(info.exists())
        {
            if (QMessageBox::question(parent, tr("Save Playlist"), tr("%1 already exists.\nDo you want to replace it?")
                                      .arg(info.fileName()), QMessageBox::Ok | QMessageBox::Cancel) != QMessageBox::Ok)
            {
                return;
            }
        }
    }

    if (!f_name.isEmpty())
    {
        model->savePlaylist(f_name);
        d->lastDir = QFileInfo(f_name).absoluteDir().path();
    }
}

void UiHelper::jumpToTrack(QWidget *parent, PlayListModel *model)
{
    Q_D(UiHelper);
    if(!d->jumpDialog)
        d->jumpDialog = new JumpToTrackDialog(model, parent);

    if(d->jumpDialog->isHidden())
        d->jumpDialog->show();

    d->jumpDialog->raise();
}

void UiHelper::about(QWidget *parent)
{
    AboutDialog *dialog = new AboutDialog(parent);
    dialog->exec();
    dialog->deleteLater();
}

void UiHelper::toggleVisibility()
{
    emit toggleVisibilityCalled();
}

void UiHelper::showMainWindow()
{
    emit showMainWindowCalled();
}

void UiHelper::exit()
{
    qApp->closeAllWindows();
    qApp->quit();
}

void UiHelper::replaceAndPlay(const QStringList &paths)
{
    Q_D(UiHelper);
    if(paths.isEmpty())
        return;

    MediaPlayer::instance()->stop();
    PlayListModel *pl = PlayListManager::instance()->selectedPlayList();
    PlayListManager::instance()->activatePlayList(pl);
    pl->clear();

    d->plConnections << connect(pl, &PlayListModel::tracksAdded, MediaPlayer::instance(), &MediaPlayer::play);
    d->plConnections << connect(pl, &PlayListModel::tracksAdded, this, [d] { d->disconnectPl(); });
    d->plConnections << connect(pl, &PlayListModel::loaderFinished, this, [d] { d->disconnectPl(); });
    pl->addPaths(paths);
}

UiHelper *UiHelper::instance()
{
    return UiHelperPrivate::instance;
}
