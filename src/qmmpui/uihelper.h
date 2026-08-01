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
#ifndef UIHELPER_H
#define UIHELPER_H

#include <QObject>
#include <QMap>
#include <QList>
#include <QPointer>
#include <QStringList>
#include <QApplication>
#include "playlistmanager.h"
#include "playlistmodel.h"
#include "qmmpui_export.h"

class QAction;
class QMenu;
class QWidget;
class General;
class GeneralFactory;
class JumpToTrackDialog;
class UiHelperPrivate;

/*! @brief The UiHelper class provides simple api to access general plugins and some gui features.
 * @author Ilya Kotov <forkotov02@ya.ru>
 */
class QMMPUI_EXPORT UiHelper : public QObject
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(UiHelper)
public:
    /*!
     * Object constructor,
     * \param parent Parent object
     */
    UiHelper(QObject *parent = nullptr);
    /*!
     * Destructor
     */
    ~UiHelper();
    /*!
     * Enum of menu type
     */
    enum MenuType
    {
        TOOLS_MENU = 0, /*!< tools menu */
        PLAYLIST_MENU,  /*!< playlist context menu */
        ADD_MENU        /*!< "add"/"file" menu */
    };
    /*!
     * Returns \b true if one of the general plugin can change visibility, otherwise returns \b false
     */
    bool visibilityControl() const;
    /*!
     * Adds the newly created action to the menu with type \b type.
     * Menu with type \b type should be created first.
     * \param action Pointer of action.
     * \param type Menu type.
     */
    void addAction(QAction *action, MenuType type = TOOLS_MENU);
    /*!
     * Removes action \b action from created menus.
     */
    void removeAction(QAction *action);
    /*!
     * Returns a list of actions of the menu type \b type
     */
    QList<QAction *> actions(MenuType type = TOOLS_MENU);
    /*!
     * Creates menu with type \b type
     * \param type Menu type.
     * \param title Menu title.
     * \param autoHide Hide empty menu.
     * \param parent Parent widget
     */
    QMenu *createMenu(MenuType type, const QString &title = QString(), bool autoHide = true, QWidget *parent = nullptr);
    /*!
     * Registers existing menu for access from general plugins.
     * \param type Menu type.
     * \param menu Menu pointer.
     * \param autoHide Hide empty menu.
     * \param before An action, after which the rest are added.
     */
    void registerMenu(MenuType type, QMenu *menu, bool autoHide = false, QAction *before = nullptr);
    /*!
     * Opens 'Add Files' dialog
     * \param parent Parent widget
     * \param model Destination playlist model
     */
    void addFiles(QWidget *parent = qApp->activeWindow(),
                 PlayListModel *model = PlayListManager::instance()->selectedPlayList());
    /*!
     * Opens 'Play Files' dialog
     * \param parent Parent widget
     * \param model Destination playlist model
     */
    void playFiles(QWidget *parent = qApp->activeWindow(),
                   PlayListModel *model = PlayListManager::instance()->selectedPlayList());
    /*!
     * Opens 'Add Directory' dialog
     * \param parent Parent widget
     * \param model Destination playlist model
     */
    void addDirectory(QWidget *parent = qApp->activeWindow(),
                      PlayListModel *model = PlayListManager::instance()->selectedPlayList());
    /*!
     * Opens 'Add URL' dialog
     * \param parent Parent widget
     * \param model Destination playlist model
     */
    void addUrl(QWidget *parent = qApp->activeWindow(),
                PlayListModel *model = PlayListManager::instance()->selectedPlayList());
    /*!
     * Opens 'Load Playlist' dialog
     * \param parent Parent widget
     * \param model Destination playlist model
     */
    void loadPlayList(QWidget *parent = qApp->activeWindow(),
                     PlayListModel *model = PlayListManager::instance()->selectedPlayList());
    /*!
     * Opens 'Save Playlist' dialog
     * \param parent Parent widget
     * \param model Destination playlist model
     */
    void savePlayList(QWidget *parent = qApp->activeWindow(),
                     PlayListModel *model = PlayListManager::instance()->selectedPlayList());
    /*!
     * Opens 'Jump To Track' dialog
     * \param parent Parent widget
     * \param model Destination playlist model
     */
    void jumpToTrack(QWidget *parent = qApp->activeWindow(),
                     PlayListModel *model = PlayListManager::instance()->selectedPlayList());
    /*!
     * Shows about dialog
     * \param parent Parent widget
     */
    void about(QWidget *parent = nullptr);
    /*!
     * Returns a pointer to the object's instance.
     */
    static UiHelper *instance();

public slots:
    /*!
     * Toggles player window visibility.
     */
    void toggleVisibility();
    /*!
     * Shows the main window of the player.
     */
    void showMainWindow();
    /*!
     * Tells the player to exit.
     */
    void exit();
    /*!
     * This function does several tasks:
     * 1) stops playback;
     * 2) activates selected playlist;
     * 3) clears selected playlist;
     * 4) loads tracks for specified paths to the selected playlist;
     * 5) starts playback from the first added track.
     * \param paths A list of files or/and directories.
     */
    void replaceAndPlay(const QStringList &paths);

signals:
    /*!
     * Emitted when some general plugin requires to toggle visibility.
     * Use it to show/hide player's window.
     */
    void toggleVisibilityCalled();
    /*!
     * Emitted when some general plugin requires to show main window.
     * Use it to show player's window and to raise it to the top.
     */
    void showMainWindowCalled();
    /*!
     * Emitted when some general plugin adds widget.
     * User interface should create this widget using the given \b id.
     */
    void widgetAdded(const QString &id);
    /*!
     * Emitted when some general plugin removes widget \b id.
     * User interface should remove widget with the given \b id.
     */
    void widgetRemoved(const QString &id);
    /*!
     * Emitted when some general plugin changes settings of the widget \b id.
     * User interface should recreate widget with the given \b id.
     */
    void widgetUpdated(const QString &id);

private:
    UiHelperPrivate *d_ptr;
};

#endif
