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
#include <QPainter>
#include <QResizeEvent>
#include <QSettings>
#include <QMenu>
#include <QAction>
#include <QCloseEvent>
#include <QInputDialog>
#include <QScreen>
#include <QStyle>
#include <algorithm>
#include <qmmpui/playlistitem.h>
#include <qmmpui/playlistmodel.h>
#include <qmmpui/playlistmanager.h>
#include <qmmpui/uihelper.h>
#include <qmmpui/qmmpuisettings.h>
#include <qmmpui/mediaplayer.h>
#include <qmmp/soundcore.h>
#include "dock.h"
#include "skin.h"
#include "skinnedlistwidget.h"
#include "skinnedbutton.h"
#include "skinnedplaylisttitlebar.h"
#include "skinnedplaylistslider.h"
#include "pixmapwidget.h"
#include "skinnedkeyboardmanager.h"
#include "skinnedplaylistbrowser.h"
#include "skinnedplaylistselector.h"
#include "windowsystem.h"
#include "skinnedactionmanager.h"
#include "skinnedplaylistheader.h"
#include "skinnedplaylist.h"

SkinnedPlayList::SkinnedPlayList(PlayListManager *manager, SkinnedMainWindow *parent) : QWidget(parent)
{
    setAttribute(Qt::WA_AlwaysShowToolTips,true);
    setWindowTitle(tr("Playlist"));
    m_pl_manager = manager;
    m_ui_settings = QmmpUiSettings::instance();
    m_skin = Skin::instance();
    m_ratio = m_skin->ratio();

#ifdef QMMP_WS_X11
    QString wm_name = WindowSystem::netWindowManagerName();
    m_compiz = wm_name.contains(u"compiz"_s, Qt::CaseInsensitive);
    if(wm_name.contains(u"openbox"_s, Qt::CaseInsensitive) || wm_name.contains(u"xfwm4"_s, Qt::CaseInsensitive))
        setWindowFlags (Qt::Drawer | Qt::FramelessWindowHint);
    else if(wm_name.contains(u"metacity"_s, Qt::CaseInsensitive))
        setWindowFlags(Qt::Tool | Qt::FramelessWindowHint);
    else
#endif
        setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);

#ifdef QMMP_WS_X11
    if(m_compiz)
    {
        setFixedSize(m_skin->scaled(275), m_skin->scaled(116));
    }
    else
#endif
    {
        resize (m_skin->scaled(275), m_skin->scaled(116));
        setSizeIncrement (m_skin->scaled(25), m_skin->scaled(29));
        setMinimumSize(m_skin->scaled(275), m_skin->scaled(116));
    }

    m_listWidget = new SkinnedListWidget (this);
    m_plslider = new SkinnedPlayListSlider(this);
    m_buttonAdd = new SkinnedButton(this, Skin::PL_BT_ADD,Skin::PL_BT_ADD, Skin::CUR_PNORMAL);
    m_buttonSub = new SkinnedButton(this, Skin::PL_BT_SUB,Skin::PL_BT_SUB, Skin::CUR_PNORMAL);
    m_selectButton  = new SkinnedButton(this, Skin::PL_BT_SEL,Skin::PL_BT_SEL, Skin::CUR_PNORMAL);
    m_sortButton= new SkinnedButton(this, Skin::PL_BT_SORT,Skin::PL_BT_SORT, Skin::CUR_PNORMAL);
    m_playlistButton = new SkinnedButton(this, Skin::PL_BT_LST,Skin::PL_BT_LST, Skin::CUR_PNORMAL);
    m_resizeWidget = new QWidget(this);
    m_resizeWidget->resize(25, 25);
    m_resizeWidget->setCursor(m_skin->getCursor(Skin::CUR_PSIZE));
    m_keyboardManager = new SkinnedKeyboardManager(m_listWidget);

    connect(m_listWidget, &SkinnedListWidget::doubleClicked, this, [this] {
        MediaPlayer::instance()->stop();
        m_pl_manager->activatePlayList(m_pl_manager->selectedPlayList());
        MediaPlayer::instance()->play();
    });

    connect(m_plslider, &SkinnedPlayListSlider::sliderMoveRequest, m_listWidget, &SkinnedListWidget::scroll);
    connect(m_listWidget, &SkinnedListWidget::scrollPositionChanged, m_plslider, &SkinnedPlayListSlider::setPos);
    connect(m_skin, &Skin::skinChanged, this, &SkinnedPlayList::updateSkin);
    connect(m_buttonAdd, &SkinnedButton::clicked, this, &SkinnedPlayList::showAddMenu);
    connect(m_buttonSub, &SkinnedButton::clicked, this, &SkinnedPlayList::showSubMenu);
    connect(m_selectButton, &SkinnedButton::clicked, this, &SkinnedPlayList::showSelectMenu);
    connect(m_sortButton, &SkinnedButton::clicked, this, &SkinnedPlayList::showSortMenu);
    connect(m_playlistButton, &SkinnedButton::clicked, this, &SkinnedPlayList::showPlaylistMenu);

    connect(m_pl_manager, &PlayListManager::selectedPlayListChanged, m_listWidget, &SkinnedListWidget::setModel);
    m_listWidget->setModel(m_pl_manager->selectedPlayList());

    createMenus();
    createActions();
    readSettings();

    m_titleBar = new SkinnedPlayListTitleBar (this);
    m_titleBar->setMinimumSize(0,0);
    m_titleBar->move(0,0);
    connect(m_pl_manager, &PlayListManager::currentPlayListChanged, this, &SkinnedPlayList::onCurrentPlayListChanged);
    m_titleBar->setModel(m_pl_manager->currentPlayList());

    setCursor(m_skin->getCursor(Skin::CUR_PNORMAL));
    updatePositions();
}

SkinnedPlayList::~SkinnedPlayList()
{
    delete m_keyboardManager;
}

void SkinnedPlayList::updatePositions()
{
    int sx = (width() - m_skin->scaled(275)) / 25;
    int sy = (height() - m_skin->scaled(116)) / 29;
    if(sx < 0 || sy < 0 || m_shaded) //skip shaded mode
        return;

    m_titleBar->resize(m_skin->scaled(275) + 25 * sx, m_skin->scaled(20));
    m_plslider->resize(m_skin->scaled(20), m_skin->scaled(58) + sy * 29);

    int pl_x = m_skin->scaled(12);
    int pl_y = m_skin->scaled(20);
    int pl_w = m_skin->scaled(243) + 25 * sx;
    int pl_h = m_skin->scaled(58) + 29 * sy;

    if(m_pl_selector)
    {
        m_pl_selector->resize(m_skin->scaled(243) + 25 * sx, m_pl_selector->height());
        m_pl_selector->move(m_skin->scaled(12), m_skin->scaled(20) + m_skin->scaled(58) + 29 * sy - m_pl_selector->height());
        pl_h -= m_pl_selector->height();
    }

    m_listWidget->resize(pl_w, pl_h);
    m_listWidget->move(pl_x, pl_y);

    m_buttonAdd->move(m_skin->scaled(11), m_skin->scaled(86) + 29 * sy);
    m_buttonSub->move(m_skin->scaled(40), m_skin->scaled(86) + 29 * sy);
    m_selectButton->move(m_skin->scaled(70), m_skin->scaled(86) + 29 * sy);
    m_sortButton->move(m_skin->scaled(99), m_skin->scaled(86) + 29 * sy);

    m_playlistButton->move(m_skin->scaled(228) + sx * 25, m_skin->scaled(86) + 29 * sy);

    m_plslider->move(m_skin->scaled(255) + sx * 25, m_skin->scaled(20));
    m_resizeWidget->move(width() - 25, height() - 29);
}

void SkinnedPlayList::createMenus()
{
    m_addMenu = new QMenu(this);
    m_subMenu = new QMenu(this);
    m_selectMenu = new QMenu(this);
    m_sortMenu = new QMenu(this);
    m_playlistMenu = new QMenu(this);
    m_copySelectedMenu = new QMenu(tr("&Copy Selection To"), m_listWidget->menu());
    m_copySelectedMenu->setIcon(QIcon::fromTheme(u"edit-copy"_s));
    connect(m_copySelectedMenu, &QMenu::aboutToShow, this, &SkinnedPlayList::generateCopySelectedMenu);
    connect(m_copySelectedMenu, &QMenu::triggered, this, &SkinnedPlayList::copySelectedMenuActionTriggered);
}

void SkinnedPlayList::createActions()
{
    //add menu
    m_addMenu->addAction(ACTION(SkinnedActionManager::PL_ADD_FILE));
    m_addMenu->addAction(ACTION(SkinnedActionManager::PL_ADD_DIRECTORY));
    m_addMenu->addAction(ACTION(SkinnedActionManager::PL_ADD_URL));
    UiHelper::instance()->registerMenu(UiHelper::ADD_MENU, m_addMenu, false);
    //sub menu
    m_subMenu->addAction(SET_ACTION(SkinnedActionManager::PL_REMOVE_SELECTED, m_pl_manager, &PlayListManager::removeSelected));
    m_subMenu->addAction(SET_ACTION(SkinnedActionManager::PL_REMOVE_ALL, m_pl_manager, &PlayListManager::clear));
    m_subMenu->addAction(SET_ACTION(SkinnedActionManager::PL_REMOVE_UNSELECTED, m_pl_manager, &PlayListManager::removeUnselected));
    m_subMenu->addSeparator();
    m_subMenu->addAction(SET_ACTION(SkinnedActionManager::PL_REFRESH, m_pl_manager, &PlayListManager::refresh));
    m_subMenu->addAction(SET_ACTION(SkinnedActionManager::PL_REMOVE_INVALID, m_pl_manager, &PlayListManager::removeInvalidTracks));
    m_subMenu->addAction(SET_ACTION(SkinnedActionManager::PL_REMOVE_DUPLICATES, m_pl_manager, &PlayListManager::removeDuplicates));
    //sort menu
    m_sortMenu->addAction(SET_ACTION(SkinnedActionManager::PL_SHOW_INFO, m_pl_manager, &PlayListManager::showDetails));
    m_sortMenu->addSeparator();

    QMenu* sort_mode_menu = new QMenu(tr("Sort List"), this);
    sort_mode_menu->setIcon(QIcon::fromTheme(u"view-sort-ascending"_s));

    QAction *titleAct = sort_mode_menu->addAction(tr("By Title"));
    connect(titleAct, &QAction::triggered, this, [this]{ m_pl_manager->sort(PlayListModel::TITLE); } );

    QAction *albumAct = sort_mode_menu->addAction(tr("By Album"));
    connect(albumAct, &QAction::triggered, this, [this]{ m_pl_manager->sort(PlayListModel::ALBUM); } );

    QAction *artistAct = sort_mode_menu->addAction(tr("By Artist"));
    connect(artistAct, &QAction::triggered, this, [this]{ m_pl_manager->sort(PlayListModel::ARTIST); } );

    QAction *albumArtistAct = sort_mode_menu->addAction(tr("By Album Artist"));
    connect(albumArtistAct, &QAction::triggered, this, [this]{ m_pl_manager->sort(PlayListModel::ALBUMARTIST); } );

    QAction *nameAct = sort_mode_menu->addAction(tr("By Filename"));
    connect(nameAct, &QAction::triggered, this, [this]{ m_pl_manager->sort(PlayListModel::FILENAME); } );

    QAction *pathnameAct = sort_mode_menu->addAction(tr("By Path + Filename"));
    connect(pathnameAct, &QAction::triggered, this, [this]{ m_pl_manager->sort(PlayListModel::PATH_AND_FILENAME); } );

    QAction *dateAct = sort_mode_menu->addAction(tr("By Date"));
    connect(dateAct, &QAction::triggered, this, [this]{ m_pl_manager->sort(PlayListModel::DATE); } );

    QAction *trackAct = sort_mode_menu->addAction(tr("By Track Number"));
    connect(trackAct, &QAction::triggered, this, [this]{ m_pl_manager->sort(PlayListModel::TRACK); } );

    QAction *discAct = sort_mode_menu->addAction(tr("By Disc Number"));
    connect(discAct, &QAction::triggered, this, [this]{ m_pl_manager->sort(PlayListModel::DISCNUMBER); } );

    QAction *fileCreationDateAct = sort_mode_menu->addAction(tr("By File Creation Date"));
    connect(fileCreationDateAct, &QAction::triggered, this, [this]{ m_pl_manager->sort(PlayListModel::FILE_CREATION_DATE); } );

    QAction *fileModificationDateAct = sort_mode_menu->addAction(tr("By File Modification Date"));
    connect(fileModificationDateAct, &QAction::triggered, this, [this]{ m_pl_manager->sort(PlayListModel::FILE_MODIFICATION_DATE); } );

    QAction *groupAct = sort_mode_menu->addAction(tr("By Group"));
    connect(groupAct, &QAction::triggered, this, [this]{ m_pl_manager->sort(PlayListModel::GROUP); } );

    m_sortMenu->addMenu (sort_mode_menu);

    sort_mode_menu = new QMenu(tr("Sort Selection"), m_sortMenu);
    sort_mode_menu->setIcon(QIcon::fromTheme(u"view-sort-ascending"_s));
    titleAct = sort_mode_menu->addAction(tr("By Title"));
    connect(titleAct, &QAction::triggered, this, [this]{ m_pl_manager->sortSelection(PlayListModel::TITLE); });

    albumAct = sort_mode_menu->addAction(tr("By Album"));
    connect(albumAct, &QAction::triggered, this, [this]{ m_pl_manager->sortSelection(PlayListModel::ALBUM); });

    artistAct = sort_mode_menu->addAction(tr("By Artist"));
    connect(artistAct, &QAction::triggered, this, [this]{ m_pl_manager->sortSelection(PlayListModel::ARTIST); });

    albumArtistAct = sort_mode_menu->addAction(tr("By Album Artist"));
    connect(albumArtistAct, &QAction::triggered, this, [this]{ m_pl_manager->sortSelection(PlayListModel::ALBUMARTIST); });

    nameAct = sort_mode_menu->addAction(tr("By Filename"));
    connect(nameAct, &QAction::triggered, this, [this]{ m_pl_manager->sortSelection(PlayListModel::FILENAME); });

    pathnameAct = sort_mode_menu->addAction(tr("By Path + Filename"));
    connect(pathnameAct, &QAction::triggered, this, [this]{ m_pl_manager->sortSelection(PlayListModel::PATH_AND_FILENAME); });

    dateAct = sort_mode_menu->addAction(tr("By Date"));
    connect(dateAct, &QAction::triggered, this, [this]{ m_pl_manager->sortSelection(PlayListModel::DATE); });

    trackAct = sort_mode_menu->addAction(tr("By Track Number"));
    connect(trackAct, &QAction::triggered, this, [this]{ m_pl_manager->sortSelection(PlayListModel::TRACK); });

    discAct = sort_mode_menu->addAction(tr("By Disc Number"));
    connect(discAct, &QAction::triggered, this, [this]{ m_pl_manager->sortSelection(PlayListModel::DISCNUMBER); });

    fileCreationDateAct = sort_mode_menu->addAction(tr("By File Creation Date"));
    connect(fileCreationDateAct, &QAction::triggered, this, [this]{ m_pl_manager->sortSelection(PlayListModel::FILE_CREATION_DATE); });

    fileModificationDateAct = sort_mode_menu->addAction(tr("By File Modification Date"));
    connect(fileModificationDateAct, &QAction::triggered, this, [this]{ m_pl_manager->sortSelection(PlayListModel::FILE_MODIFICATION_DATE ); });

    m_sortMenu->addMenu(sort_mode_menu);
    m_sortMenu->addSeparator();
    m_sortMenu->addAction(QIcon::fromTheme(u"media-playlist-shuffle"_s), tr("Randomize List"),
                           m_pl_manager, &PlayListManager::randomizeList);
    m_sortMenu->addAction(QIcon::fromTheme(u"view-sort-descending"_s), tr("Reverse List"),
                           m_pl_manager, &PlayListManager::reverseList);
    //playlist context menu
    m_listWidget->menu()->addAction(SkinnedActionManager::instance()->action(SkinnedActionManager::PL_SHOW_INFO));
    m_listWidget->menu()->addSeparator();
    m_listWidget->menu()->addMenu(m_copySelectedMenu);
    m_listWidget->menu()->addActions(m_subMenu->actions().mid(0, 3)); //use 3 first actions
    m_listWidget->menu()->addMenu(UiHelper::instance()->createMenu(UiHelper::PLAYLIST_MENU,
                                  tr("Actions"), true, this));
    m_listWidget->menu()->addSeparator();
    m_listWidget->menu()->addAction(SET_ACTION(SkinnedActionManager::PL_ENQUEUE, m_pl_manager, &PlayListManager::addToQueue));
    //select menu
    m_selectMenu->addAction(SET_ACTION(SkinnedActionManager::PL_INVERT_SELECTION, m_pl_manager, &PlayListManager::invertSelection));
    m_selectMenu->addSeparator();
    m_selectMenu->addAction(SET_ACTION(SkinnedActionManager::PL_CLEAR_SELECTION, m_pl_manager, &PlayListManager::clearSelection));
    m_selectMenu->addAction(SET_ACTION(SkinnedActionManager::PL_SELECT_ALL, m_pl_manager, &PlayListManager::selectAll));
    //Playlist Menu
    m_playlistMenu->addAction(SET_ACTION(SkinnedActionManager::PL_NEW, m_pl_manager, [this] { m_pl_manager->createPlayList(); }));
    m_playlistMenu->addAction(SET_ACTION(SkinnedActionManager::PL_CLOSE, this, &SkinnedPlayList::deletePlaylist));
    m_playlistMenu->addAction(SET_ACTION(SkinnedActionManager::PL_RENAME, this, &SkinnedPlayList::renamePlaylist));
    m_playlistMenu->addSeparator();
    m_playlistMenu->addAction(SET_ACTION(SkinnedActionManager::PL_LOAD, this, &SkinnedPlayList::loadPlaylist));
    m_playlistMenu->addAction(SET_ACTION(SkinnedActionManager::PL_SAVE, this, &SkinnedPlayList::savePlaylist));
    m_playlistMenu->addSeparator();
    m_playlistMenu->addAction(SET_ACTION(SkinnedActionManager::PL_SELECT_NEXT, m_pl_manager, &PlayListManager::selectNextPlayList));
    m_playlistMenu->addAction(SET_ACTION(SkinnedActionManager::PL_SELECT_PREVIOUS, m_pl_manager, &PlayListManager::selectPreviousPlayList));
    m_playlistMenu->addAction(SET_ACTION(SkinnedActionManager::PL_SHOW_MANAGER, this, &SkinnedPlayList::showPlayLists));

    //actions
    SET_ACTION(SkinnedActionManager::PL_GROUP_TRACKS, m_ui_settings, &QmmpUiSettings::setGroupsEnabled);
    ACTION(SkinnedActionManager::PL_GROUP_TRACKS)->setChecked(m_ui_settings->isGroupsEnabled());
    SET_ACTION(SkinnedActionManager::PL_SHOW_TABBAR, this, &SkinnedPlayList::readSettings);
}

void SkinnedPlayList::closeEvent (QCloseEvent *e)
{
    if(e->spontaneous ())
        parentWidget()->close();
}

void SkinnedPlayList::paintEvent (QPaintEvent *)
{
    int sx = (width() - m_skin->scaled(275)) / 25;
    int sy = (height() - m_skin->scaled(116)) / 29;

    QPainter paint(this);
    drawPixmap (&paint, 0, m_skin->scaled(20), m_skin->getPlPart(Skin::PL_LFILL));
    for (int i = 1; i < sy + m_skin->scaled(2); i++)
    {
        drawPixmap (&paint, 0, m_skin->scaled(20) + 29 * i, m_skin->getPlPart(Skin::PL_LFILL));
    }
    drawPixmap (&paint, 0, m_skin->scaled(78) + 29 * sy, m_skin->getPlPart(Skin::PL_LSBAR));

    for (int i = 0; i < sx; i++)
    {
        drawPixmap(&paint, m_skin->scaled(125) + i * 25, m_skin->scaled(78) + sy * 29, m_skin->getPlPart(Skin::PL_SFILL1));
    }
    drawPixmap(&paint, m_skin->scaled(125) + sx * 25, m_skin->scaled(78) + sy * 29, m_skin->getPlPart(Skin::PL_RSBAR));

}

void SkinnedPlayList::drawPixmap(QPainter *painter, int x, int y, const QPixmap &pix)
{
    style()->drawItemPixmap(painter, QRect(x, y, pix.width(), pix.height()), Qt::AlignCenter, pix);
}

void SkinnedPlayList::resizeEvent(QResizeEvent *)
{
    updatePositions();
}

void SkinnedPlayList::mousePressEvent (QMouseEvent *e)
{
    Q_UNUSED(e);
    if(m_resizeWidget->underMouse())
    {
        m_resize = true;
        setCursor(m_skin->getCursor(Skin::CUR_PSIZE));
    }
    else
        m_resize = false;
}

void SkinnedPlayList::mouseMoveEvent (QMouseEvent *e)
{
    if(m_resize)
    {
        int dx = m_skin->scaled(25);
        int dy = m_skin->scaled(29);

        int sx = ((e->position().x() - m_skin->scaled(275)) + 14) / dx;
        int sy = ((e->position().y() - m_skin->scaled(116)) + 14) / dy;

        sx = qMax(sx, 0);
        sy = qMax(sy, 0);

#ifdef QMMP_WS_X11
        if(m_compiz)
            setFixedSize(m_skin->scaled(275) + dx * sx, m_skin->scaled(116) + dy * sy);
        else
#endif
            resize(m_skin->scaled(275) + dx * sx, m_skin->scaled(116) + dy * sy);


#ifdef QMMP_WS_X11
        //avoid right corner moving during resize
        if(layoutDirection() == Qt::RightToLeft)
            WindowSystem::revertGravity(winId());
#endif
    }
}

void SkinnedPlayList::mouseReleaseEvent(QMouseEvent *)
{
    setCursor(m_skin->getCursor (Skin::CUR_PNORMAL));
    /*if(m_resize)
        m_listWidget->updateList();*/
    m_resize = false;
    Dock::instance()->updateDock();
}

void SkinnedPlayList::changeEvent(QEvent * event)
{
    if(event->type() == QEvent::ActivationChange)
    {
        m_titleBar->setActive(isActiveWindow());
    }
}

void SkinnedPlayList::readSettings()
{
    if(ACTION(SkinnedActionManager::PL_SHOW_TABBAR)->isChecked())
    {
        if(!m_pl_selector)
            m_pl_selector = new SkinnedPlayListSelector(m_pl_manager, this);
        m_pl_selector->show();
        m_copySelectedMenu->menuAction()->setVisible(true);
    }
    else
    {
        if(m_pl_selector)
        {
            m_pl_selector->deleteLater();
            m_pl_selector = nullptr;
        }
        m_copySelectedMenu->menuAction()->setVisible(false);
    }

    if(m_update)
    {
        m_listWidget->readSettings();
        m_titleBar->readSettings();
        if(m_pl_selector)
            m_pl_selector->readSettings();
        updatePositions();
    }
    else
    {
        QScreen *primaryScreen = QGuiApplication::primaryScreen();
        QRect availableGeometry = primaryScreen->availableGeometry();
        QSettings settings;
        QPoint pos = settings.value("Skinned/pl_pos"_L1, QPoint (100, 332)).toPoint();
        m_ratio = m_skin->ratio();
        //TODO QGuiApplication::screenAt
        const QList<QScreen *> screens = QGuiApplication::screens();
        auto it = std::find_if(screens.cbegin(), screens.cend(), [pos](QScreen *screen){ return screen->availableGeometry().contains(pos); });
        if(it != screens.cend())
            availableGeometry = (*it)->availableGeometry();
        pos.setX(qBound(availableGeometry.left(), pos.x(), availableGeometry.right() - m_skin->scaled(275)));
        pos.setY(qBound(availableGeometry.top(), pos.y(), availableGeometry.bottom() - m_skin->scaled(116)));
        move(pos); //position
        m_update = true;
    }
}

#ifdef QMMP_WS_X11
bool SkinnedPlayList::event(QEvent *event)
{
    if(event->type() == QEvent::WinIdChange || event->type() == QEvent::Show)
    {
        WindowSystem::ghostWindow(winId());
        WindowSystem::setWinHint(winId(), "playlist", "xamp");
    }
    return QWidget::event(event);
}
#endif

void SkinnedPlayList::writeSettings()
{
    QSettings settings;
    //position
    settings.setValue("Skinned/pl_pos"_L1, this->pos());
}

#ifdef QMMP_WS_X11
bool SkinnedPlayList::useCompiz() const
{
    return m_compiz;
}
#endif

void SkinnedPlayList::showAddMenu()
{
    m_addMenu->exec(m_buttonAdd->mapToGlobal(QPoint (0,0)));
}

void SkinnedPlayList::showSubMenu()
{
    m_subMenu->exec(m_buttonSub->mapToGlobal(QPoint (0,0)));
}

void SkinnedPlayList::showSelectMenu()
{
    m_selectMenu->exec(m_selectButton->mapToGlobal(QPoint (0,0)));
}

void SkinnedPlayList::showSortMenu()
{
    m_sortMenu->exec(m_sortButton->mapToGlobal(QPoint (0,0)));
}

void SkinnedPlayList::showPlaylistMenu()
{
    m_playlistMenu->exec(m_playlistButton->mapToGlobal(QPoint(0,0)));
}

void SkinnedPlayList::keyPressEvent(QKeyEvent *ke)
{
    m_keyboardManager->handleKeyPress(ke);
}

void SkinnedPlayList::updateSkin()
{
    if(m_pl_selector)
        m_pl_selector->readSettings();
    setCursor(m_skin->getCursor(Skin::CUR_PNORMAL)); // TODO shaded
    m_resizeWidget->setCursor(m_skin->getCursor (Skin::CUR_PSIZE));
    m_ratio = m_skin->ratio();
    setMinimalMode(m_shaded);
}

void SkinnedPlayList::deletePlaylist()
{
    m_pl_manager->removePlayList(m_pl_manager->selectedPlayList());
}

void SkinnedPlayList::renamePlaylist()
{
    bool ok = false;
    QString name = QInputDialog::getText(this,
                                         tr("Rename Playlist"), tr("Playlist name:"),
                                         QLineEdit::Normal,
                                         m_pl_manager->selectedPlayList()->name(), &ok);
    if(ok)
        m_pl_manager->selectedPlayList()->setName(name);
}

void SkinnedPlayList::showPlayLists()
{
    if(!m_pl_browser)
    {
        m_pl_browser = new SkinnedPlayListBrowser(m_pl_manager, this);
        m_pl_browser->show();
    }
    else
        m_pl_browser->show();
}

void SkinnedPlayList::generateCopySelectedMenu()
{
    m_copySelectedMenu->clear();
    m_newPlayListAction = m_copySelectedMenu->addAction(tr("&New PlayList"));
    m_newPlayListAction->setIcon(QIcon::fromTheme(u"document-new"_s));
    m_copySelectedMenu->addSeparator();

    for(const QString &name : m_pl_manager->playListNames())
    {
        if(name == m_pl_manager->selectedPlayList()->name()) //skip selected playlist
            continue;

        QString actionText = name;
        actionText.replace(u"&"_s, u"&&"_s);
        actionText.prepend(QLatin1Char('&'));
        m_copySelectedMenu->addAction(actionText);
    }
}

void SkinnedPlayList::copySelectedMenuActionTriggered(QAction *action)
{
    PlayListModel *targetPlayList = nullptr;
    QString actionText = action->text();
    QList<PlayListTrack *> selectedTracks = m_pl_manager->selectedPlayList()->selectedTracks();

    if(selectedTracks.isEmpty())
        return;

    if(action == m_newPlayListAction)
    {
        targetPlayList = m_pl_manager->createPlayList(m_pl_manager->selectedPlayList()->name());
    }
    else
    {
        actionText.remove(0, 1).replace(u"&&"_s, u"&"_s);
        for(PlayListModel *model : m_pl_manager->playLists())
        {
            if(model->name() == actionText)
            {
                targetPlayList = model;
                break;
            }
        }
    }
    if(!targetPlayList)
    {
        qCWarning(plugin, "Error: Cannot find target playlist '%s'",qPrintable(actionText));
        return;
    }

    QList<PlayListTrack *> theCopy;
    for(PlayListTrack *track : std::as_const(selectedTracks))
    {
        theCopy << new PlayListTrack(track);
    }
    targetPlayList->addTracks(theCopy);
}

void SkinnedPlayList::onCurrentPlayListChanged(PlayListModel *current, PlayListModel *previous)
{
    Q_UNUSED(previous);
    m_titleBar->setModel(current);
}

void SkinnedPlayList::setMinimalMode(bool b)
{
    if(!m_shaded)
        m_height = height();
    m_shaded = b;

#ifdef QMMP_WS_X11
    if(m_compiz)
    {
        if(m_shaded)
        {
            m_height = height();
            setFixedSize(qMax(width(), m_skin->scaled(275)), m_skin->scaled(14));
        }
        else
        {
            setFixedSize(qMax(width(), m_skin->scaled(275)), qMax(m_height, m_skin->scaled(116)));
        }
    }
    else
#endif
    {
        if(m_shaded)
        {
            m_height = height();
            setSizeIncrement(m_skin->scaled(25), 1);
            setMinimumSize(m_skin->scaled(275), m_skin->scaled(14));
            resize(width(), m_skin->scaled(14));
        }
        else
        {
            setMinimumSize(m_skin->scaled(275), m_skin->scaled(116));
            resize(width(), m_height);
            setSizeIncrement(m_skin->scaled(25), m_skin->scaled(29));
        }
    }
    updatePositions();
    update();
}
