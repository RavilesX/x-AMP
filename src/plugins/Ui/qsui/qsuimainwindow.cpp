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
#include <QPushButton>
#include <QHBoxLayout>
#include <QLabel>
#include <QTreeView>
#include <QMessageBox>
#include <QMenu>
#include <QSettings>
#include <QInputDialog>
#include <QGridLayout>
#include <QToolButton>
#include <qmmp/soundcore.h>
#include <qmmp/decoder.h>
#include <qmmp/metadatamanager.h>
#include <qmmp/effect.h>
#include <qmmpui/general.h>
#include <qmmpui/playlistparser.h>
#include <qmmpui/playlistformat.h>
#include <qmmpui/filedialog.h>
#include <qmmpui/playlistmodel.h>
#include <qmmpui/mediaplayer.h>
#include <qmmpui/uihelper.h>
#include <qmmpui/configdialog.h>
#include <qmmpui/qmmpuisettings.h>
#include <qmmpui/visualmenu.h>
#include "qsuitabbar.h"
#include "qsuitoolbareditor.h"
#include "qsuistatusbareditor.h"
#include "qsuiactionmanager.h"
#include "qsuivisualization.h"
#include "qsuilistwidget.h"
#include "qsuipositionslider.h"
#include "qsuimainwindow.h"
#include "qsuisettings.h"
#include "qsuihotkeyeditor.h"
#include "filesystembrowser.h"
#include "aboutqsuidialog.h"
#include "qsuikeyboardmanager.h"
#include "qsuicoverwidget.h"
#include "qsuiplaylistbrowser.h"
#include "volumeslider.h"
#include "qsuitabwidget.h"
#include "qsuiquicksearch.h"
#include "qsuiwaveformseekbar.h"
#include "qsuistatusbar.h"
#include "dockwidgetlist.h"
#include "ui_qsuimainwindow.h"
#include "qsuiequalizer.h"

QSUiMainWindow::QSUiMainWindow(QWidget *parent) : QMainWindow(parent), m_ui(new Ui::QSUiMainWindow)
{
    m_ui->setupUi(this);
    m_titleFormatter.setPattern(u"%if(%p,%p - %t,%t)"_s);
    //qmmp objects
    m_player = MediaPlayer::instance();
    connect(m_player, &MediaPlayer::playbackFinished, this, &QSUiMainWindow::restoreWindowTitle);
    m_core = SoundCore::instance();
    m_pl_manager = PlayListManager::instance();
    m_uiHelper = UiHelper::instance();
    m_ui_settings = QmmpUiSettings::instance();
    connect(m_uiHelper, &UiHelper::toggleVisibilityCalled, this, &QSUiMainWindow::toggleVisibility);
    connect(m_uiHelper, &UiHelper::showMainWindowCalled, this, &QSUiMainWindow::showAndRaise);
    m_visMenu = new VisualMenu(this); //visual menu
    m_ui->menuTools->addMenu(m_visMenu);
    m_ui->menuTools->addSeparator();
    m_pl_menu = new QMenu(this); //playlist menu
    m_copySelectedMenu = new QMenu(tr("&Copy Selection To"), m_pl_menu);
    m_copySelectedMenu->setIcon(QIcon::fromTheme(u"edit-copy"_s));
    connect(m_copySelectedMenu, &QMenu::aboutToShow, this, &QSUiMainWindow::generateCopySelectedMenu);
    connect(m_copySelectedMenu, &QMenu::triggered, this, &QSUiMainWindow::copySelectedMenuActionTriggered);
    new QSUiActionManager(this); //action manager
    createWidgets(); //widgets
    //status
    connect(m_core, &SoundCore::elapsedChanged, this, &QSUiMainWindow::updatePosition);
    connect(m_core, &SoundCore::stateChanged, this, &QSUiMainWindow::showState);
    connect(m_core, &SoundCore::trackInfoChanged, this, &QSUiMainWindow::showMetaData);
    //keyboard manager
    m_key_manager = new QSUiKeyboardManager(this);
    //create tabs
    for(PlayListModel *model : m_pl_manager->playLists())
    {
        if(m_pl_manager->currentPlayList() != model)
            m_tabWidget->addTab(model->name());
        else
            m_tabWidget->addTab(QLatin1Char('[') + model->name() + QLatin1Char(']'));
        connect(model, &PlayListModel::nameChanged, this, &QSUiMainWindow::updateTabs);
    }
    m_tabWidget->setCurrentIndex(m_pl_manager->selectedPlayListIndex());
    m_key_manager->setListWidget(m_listWidget);

    m_positionSlider = new QSUiPositionSlider(this);
    m_positionSlider->setFocusPolicy(Qt::NoFocus);
    m_positionSlider->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    //prepare visualization
    Visual::initialize(this, m_visMenu, &VisualMenu::updateActions);
    //playlist manager
    connect(m_positionSlider, &QSUiPositionSlider::sliderReleased, this, &QSUiMainWindow::seek);
    connect(m_pl_manager, &PlayListManager::currentPlayListChanged, this, &QSUiMainWindow::onCurrentPlayListChanged);
    connect(m_pl_manager, &PlayListManager::selectedPlayListChanged, this, &QSUiMainWindow::updateTabs);
    connect(m_pl_manager, &PlayListManager::playListRemoved, this, &QSUiMainWindow::removeTab);
    connect(m_pl_manager, &PlayListManager::playListAdded, this, &QSUiMainWindow::addTab);
    connect(m_pl_manager, &PlayListManager::selectedPlayListChanged, m_listWidget, &QSUiListWidget::setModel);
    connect(m_pl_manager->currentPlayList(), &PlayListModel::listChanged, this, &QSUiMainWindow::onListChanged);
    connect(m_tabWidget, &QSUiTabWidget::currentChanged, m_pl_manager, &PlayListManager::selectPlayListIndex);
    connect(m_tabWidget, &QSUiTabWidget::tabCloseRequested, m_pl_manager, &PlayListManager::removePlayListIndex);
    connect(m_tabWidget, &QSUiTabWidget::tabMoved, m_pl_manager, &PlayListManager::move);
    connect(m_tabWidget, &QSUiTabWidget::createPlayListRequested, m_pl_manager, [this] { m_pl_manager->createPlayList(); });

    m_tabWidget->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_tabWidget, &QSUiTabWidget::customContextMenuRequested, this, &QSUiMainWindow::showTabMenu);
    m_tab_menu = new QMenu(m_tabWidget);
    //status bar
    m_statusBar = new QSUiStatusBar(this);
    m_ui->statusbar->addPermanentWidget(m_statusBar, 1);
    m_ui->statusbar->setStyleSheet(u"QStatusBar::item { border: 0px solid black; };"_s);
    //volume
    m_volumeSlider = new VolumeSlider(this);
    m_volumeSlider->setFocusPolicy(Qt::NoFocus);
    m_volumeSlider->setFixedWidth(100);
    m_volumeSlider->setRange(0, 100);
    SET_ACTION(QSUiActionManager::VOL_MUTE, m_core, &SoundCore::setMuted);
    connect(m_volumeSlider, &QSlider::sliderMoved, m_core, &SoundCore::setVolume);
    connect(m_core, &SoundCore::volumeChanged, m_volumeSlider, &VolumeSlider::setValue);
    connect(m_core, &SoundCore::volumeChanged, this, &QSUiMainWindow::updateVolumeIcon);
    connect(m_core, &SoundCore::mutedChanged, this, &QSUiMainWindow::updateVolumeIcon);
    connect(m_core, &SoundCore::mutedChanged, ACTION(QSUiActionManager::VOL_MUTE), &QAction::setChecked);
    m_volumeSlider->setValue(m_core->volume());
    updateVolumeIcon();
    //balance
    m_balanceSlider = new VolumeSlider(this);
    m_balanceSlider->setFocusPolicy(Qt::NoFocus);
    m_balanceSlider->setFixedWidth(100);
    m_balanceSlider->setRange(-100, 100);
    m_balanceSlider->setTickInterval(100);
    m_balanceSlider->setTickPosition(QSlider::TicksAbove);
    connect(m_balanceSlider, &QSlider::sliderMoved, m_core, &SoundCore::setBalance);
    connect(m_core, &SoundCore::balanceChanged, m_balanceSlider, &VolumeSlider::setValue);
    m_balanceSlider->setValue(m_core->balance());

    //quick search
    m_quickSearch = new QSUiQuickSearch(m_listWidget, this);
    m_quickSearch->setMaximumWidth(250);
    //visualization
    m_analyzer = new QSUIVisualization(this);
    m_ui->analyzerDockWidget->setWidget(m_analyzer);
    Visual::add(m_analyzer);
    //waveform seek bar
    m_seekBar = new QSUiWaveformSeekBar(this);
    m_ui->waveformSeekBarDockWidget->setWidget(m_seekBar);
    //filesystem browser
    m_ui->fileSystemDockWidget->setWidget(new FileSystemBrowser(this));
    //cover
    m_ui->coverDockWidget->setWidget(new QSUiCoverWidget(this));
    //playlists
    m_ui->playlistsDockWidget->setWidget(new QSUiPlayListBrowser(m_pl_manager, this));
    //dock widgets (plugins)
    m_dockWidgetList = new DockWidgetList(this);
    //other
    createActions();
    readSettings();
    restoreWindowTitle();
}

QSUiMainWindow::~QSUiMainWindow()
{
    delete m_ui;
}

void QSUiMainWindow::addDir()
{
    m_uiHelper->addDirectory(this);
}

void QSUiMainWindow::addFiles()
{
    m_uiHelper->addFiles(this);
}

void QSUiMainWindow::playFiles()
{
    m_uiHelper->playFiles(this);
}

void QSUiMainWindow::record(bool enabled)
{
    EffectFactory *fileWriterFactory = Effect::findFactory(u"filewriter"_s);
    if(fileWriterFactory)
        Effect::setEnabled(fileWriterFactory, enabled);
}

void QSUiMainWindow::addUrl()
{
    m_uiHelper->addUrl(this);
}

void QSUiMainWindow::updatePosition(qint64 pos)
{
    m_positionSlider->setMaximum(m_core->duration() / 1000);
    if(!m_positionSlider->isSliderDown())
        m_positionSlider->setValue(pos / 1000);
}

void QSUiMainWindow::seek()
{
    m_core->seek(m_positionSlider->value()*1000);
}

void QSUiMainWindow::showState(Qmmp::State state)
{
    if(state != Qmmp::Playing)
    {
        ACTION(QSUiActionManager::PLAY_PAUSE)->setIcon(QIcon::fromTheme(u"media-playback-start"_s));
    }

    switch((int) state)
    {
    case Qmmp::Playing:
    {
        m_analyzer->setCover(MetaDataManager::instance()->getCover(m_core->path()));
        QSUiCoverWidget *cw = qobject_cast<QSUiCoverWidget *>(m_ui->coverDockWidget->widget());
        cw->setCover(MetaDataManager::instance()->getCover(m_core->path()));
        ACTION(QSUiActionManager::PLAY_PAUSE)->setIcon(QIcon::fromTheme(u"media-playback-pause"_s));
        break;
    }
    case Qmmp::Paused:
        break;
    case Qmmp::Stopped:
        m_positionSlider->setValue(0);
        m_analyzer->clearCover();
        qobject_cast<QSUiCoverWidget *>(m_ui->coverDockWidget->widget())->clearCover();
        break;
    default:
        ;
    }
}

void QSUiMainWindow::updateTabs()
{
    for(int i = 0; i < m_pl_manager->count(); ++i)
    {
        PlayListModel *model = m_pl_manager->playListAt(i);
        if(model == m_pl_manager->currentPlayList())
            m_tabWidget->setTabText(i, QLatin1Char('[') + model->name() + QLatin1Char(']'));
        else
            m_tabWidget->setTabText(i, model->name());
    }
    m_tabWidget->setCurrentIndex(m_pl_manager->selectedPlayListIndex());
}

void QSUiMainWindow::removePlaylist()
{
    m_pl_manager->removePlayList(m_pl_manager->selectedPlayList());
}

void QSUiMainWindow::removePlaylistWithIndex(int index)
{
    m_pl_manager->removePlayList(m_pl_manager->playListAt(index));
}

void QSUiMainWindow::addTab(int index)
{
    m_tabWidget->insertTab(index, m_pl_manager->playListAt(index)->name());
    connect(m_pl_manager->playListAt(index), &PlayListModel::nameChanged, this, &QSUiMainWindow::updateTabs);
    updateTabs();
}

void QSUiMainWindow::removeTab(int index)
{
    m_tabWidget->removeTab(index);
    updateTabs();
}

void QSUiMainWindow::renameTab()
{
    bool ok = false;
    QString name = QInputDialog::getText(this, tr("Rename Playlist"), tr("Playlist name:"), QLineEdit::Normal,
                                         m_pl_manager->selectedPlayList()->name(), &ok);
    if(ok)
        m_pl_manager->selectedPlayList()->setName(name);
}

void QSUiMainWindow::aboutUi()
{
    AboutQSUIDialog dialog(this);
    dialog.exec();
}

void QSUiMainWindow::about()
{
    m_uiHelper->about(this);
}

void QSUiMainWindow::toggleVisibility()
{
    if(isHidden() || isMinimized())
        showAndRaise();
    else
        hide();
}

void QSUiMainWindow::showAndRaise()
{
    show();
    if(m_wasMaximized)
        showMaximized();
    else
        showNormal();
    raise();
    activateWindow();
}

void QSUiMainWindow::showSettings()
{
    ConfigDialog *confDialog = new ConfigDialog(this);
    QSUiSettings *simpleSettings = new QSUiSettings(this);
    QSUiHotkeyEditor *hotkeyEditor = new QSUiHotkeyEditor(this);
    confDialog->addPage(tr("Appearance"), simpleSettings, QIcon(u":/qsui/qsui_settings.png"_s));
    confDialog->addPage(tr("Shortcuts"), hotkeyEditor, QIcon(u":/qsui/qsui_shortcuts.png"_s));
    if(confDialog->exec() == QDialog::Accepted)
    {
        simpleSettings->writeSettings();
        hotkeyEditor->applyShortcuts();
        readSettings();
        QSUiActionManager::instance()->saveActions();
        m_analyzer->readSettings();
        m_seekBar->readSettings();
    }
    confDialog->deleteLater();
}

void QSUiMainWindow::showAppMenu()
{
    QAction *action = qobject_cast<QAction *>(sender());
    if(!action)
        return;

    QPoint menuPos = pos();

    for(QObject *o : action->associatedObjects())
    {
        QToolButton *toolButton = qobject_cast<QToolButton *>(o);
        if(toolButton && toolButton->parentWidget())
        {
            menuPos = toolButton->parentWidget()->mapToGlobal(toolButton->geometry().bottomLeft());
            break;
        }
    }

    QMenu *appMenu = new QMenu(this);
    appMenu->setAttribute(Qt::WA_DeleteOnClose, true);
    appMenu->addActions(menuBar()->actions());
    appMenu->popup(menuPos);
}

void QSUiMainWindow::updateVolumeIcon()
{
    int maxVol = m_core->volume();

    QString iconName = u"audio-volume-high"_s;
    if(maxVol == 0 || m_core->isMuted())
        iconName = u"audio-volume-muted"_s;
    else if(maxVol < 30)
        iconName = u"audio-volume-low"_s;
    else if(maxVol < 60)
        iconName = u"audio-volume-medium"_s;

    ACTION(QSUiActionManager::VOL_MUTE)->setIcon(QIcon::fromTheme(iconName, QIcon(QStringLiteral(":/qsui/%1.png").arg(iconName))));
}

void QSUiMainWindow::jumpTo()
{
    m_uiHelper->jumpToTrack(this);
}

void QSUiMainWindow::playPause()
{
    if (m_core->state() == Qmmp::Playing)
        m_player->pause();
    else
        m_player->play();
}

bool QSUiMainWindow::event(QEvent *event)
{
    if(event->type() == QEvent::StyleChange || event->type() == QEvent::PaletteChange)
    {
        const QWidgetList widgets = m_statusBar->findChildren<QWidget *>();
        for(QWidget *w : std::as_const(widgets))
            w->setPalette(qApp->palette(w));

        m_analyzer->readSettings();
        m_seekBar->readSettings();
    }
    return QMainWindow::event(event);
}

void QSUiMainWindow::closeEvent(QCloseEvent *e)
{
    if(!m_hideOnClose || !m_uiHelper->visibilityControl())
        m_uiHelper->exit();
    QMainWindow::closeEvent(e);
}

void QSUiMainWindow::hideEvent(QHideEvent *e)
{
    writeSettings();
    m_wasMaximized = isMaximized();
    QMainWindow::hideEvent(e);
}

QMenu *QSUiMainWindow::createPopupMenu()
{
    QMenu *menu = QMainWindow::createPopupMenu();
    menu->addSeparator();
    menu->addAction(ACTION(QSUiActionManager::UI_SHOW_MENUBAR));
    ACTION(QSUiActionManager::UI_SHOW_MENUBAR)->setChecked(menuBar()->isVisible());
    menu->addAction(ACTION(QSUiActionManager::UI_SHOW_STATUSBAR));
    ACTION(QSUiActionManager::UI_SHOW_STATUSBAR)->setChecked(statusBar()->isVisible());
    return menu;
}

void QSUiMainWindow::createWidgets()
{
    m_tabWidget = new QSUiTabWidget(this);
    m_listWidget = m_tabWidget->listWidget();
    m_listWidget->setMenu(m_pl_menu);
    setCentralWidget(m_tabWidget);
    //'new playlist' button
    m_addListButton = new QToolButton(m_tabWidget);
    m_addListButton->setText(u"+"_s);
    m_addListButton->setToolButtonStyle(Qt::ToolButtonIconOnly);
    m_addListButton->setAutoRaise(true);
    m_addListButton->setIcon(QIcon::fromTheme(u"list-add"_s));
    m_addListButton->setToolTip(tr("Add new playlist"));
    connect(m_addListButton, &QToolButton::clicked, m_pl_manager, [this] { m_pl_manager->createPlayList(); });
    //playlist menu button
    m_tabListMenuButton = new QToolButton(m_tabWidget);
    m_tabListMenuButton->setToolButtonStyle(Qt::ToolButtonIconOnly);
    m_tabListMenuButton->setAutoRaise(true);
    m_tabListMenuButton->setToolTip(tr("Show all tabs"));
    m_tabListMenuButton->setArrowType(Qt::DownArrow);
    m_tabListMenuButton->setStyleSheet(u"QToolButton::menu-indicator { image: none; }"_s);
    m_tabListMenuButton->setPopupMode(QToolButton::InstantPopup);
    m_tabListMenuButton->setMenu(m_tabWidget->menu());
}

void QSUiMainWindow::createActions()
{
    //prepare checkable actions
    ACTION(QSUiActionManager::REPEAT_ALL)->setChecked(m_ui_settings->isRepeatableList());
    ACTION(QSUiActionManager::REPEAT_TRACK)->setChecked(m_ui_settings->isRepeatableTrack());
    ACTION(QSUiActionManager::SHUFFLE)->setChecked(m_ui_settings->isShuffle());
    ACTION(QSUiActionManager::NO_PL_ADVANCE)->setChecked(m_ui_settings->isNoPlayListAdvance());
    ACTION(QSUiActionManager::TRANSIT_BETWEEN_PLAYLISTS)->setChecked(m_ui_settings->isPlayListTransitionEnabled());

    connect(m_ui_settings, &QmmpUiSettings::repeatableListChanged, ACTION(QSUiActionManager::REPEAT_ALL), &QAction::setChecked);
    connect(m_ui_settings, &QmmpUiSettings::repeatableTrackChanged, ACTION(QSUiActionManager::REPEAT_TRACK), &QAction::setChecked);
    connect(m_ui_settings, &QmmpUiSettings::noPlayListAdvanceChanged, ACTION(QSUiActionManager::NO_PL_ADVANCE), &QAction::setChecked);
    connect(m_ui_settings, &QmmpUiSettings::shuffleChanged, ACTION(QSUiActionManager::SHUFFLE), &QAction::setChecked);
    connect(m_ui_settings, &QmmpUiSettings::playListTransitionChanged, ACTION(QSUiActionManager::TRANSIT_BETWEEN_PLAYLISTS), &QAction::setChecked);
    //register external actions
    QSUiActionManager::instance()->registerAction(QSUiActionManager::UI_ANALYZER,
                                                  m_ui->analyzerDockWidget->toggleViewAction(),
                                                  u"analyzer"_s);
    QSUiActionManager::instance()->registerAction(QSUiActionManager::UI_FILEBROWSER,
                                                  m_ui->fileSystemDockWidget->toggleViewAction(),
                                                  u"file_browser"_s, tr("Ctrl+0"));
    QSUiActionManager::instance()->registerAction(QSUiActionManager::UI_COVER,
                                                  m_ui->coverDockWidget->toggleViewAction(),
                                                  u"cover"_s);
    QSUiActionManager::instance()->registerAction(QSUiActionManager::UI_PLAYLIST_BROWSER,
                                                  m_ui->playlistsDockWidget->toggleViewAction(),
                                                  u"playlist_browser"_s, tr("P"));
    QSUiActionManager::instance()->registerAction(QSUiActionManager::UI_WAVEFORM_SEEKBAR,
                                                  m_ui->waveformSeekBarDockWidget->toggleViewAction(),
                                                  u"waveform_seekbar"_s, QString());
    QSUiActionManager::instance()->registerWidget(QSUiActionManager::UI_POS_SLIDER, m_positionSlider,
                                                  tr("Position"), u"position_slider"_s);
    QSUiActionManager::instance()->registerWidget(QSUiActionManager::UI_VOL_SLIDER, m_volumeSlider,
                                                  tr("Volume"), u"volume_slider"_s);
    QSUiActionManager::instance()->registerWidget(QSUiActionManager::UI_BAL_SLIDER, m_balanceSlider,
                                                  tr("Balance"), u"balance_slider"_s);

    QSUiActionManager::instance()->registerWidget(QSUiActionManager::UI_QUICK_SEARCH, m_quickSearch,
                                                  tr("Quick Search"), u"quick_search"_s);
    //playback
    SET_ACTION(QSUiActionManager::PREVIOUS, m_player, &MediaPlayer::previous);
    SET_ACTION(QSUiActionManager::PLAY, m_player, &MediaPlayer::play);
    SET_ACTION(QSUiActionManager::PAUSE, m_player, &MediaPlayer::pause);
    SET_ACTION(QSUiActionManager::STOP, m_player, &MediaPlayer::stop);
    SET_ACTION(QSUiActionManager::NEXT, m_player, &MediaPlayer::next);
    SET_ACTION(QSUiActionManager::EJECT, this, &QSUiMainWindow::playFiles);
    SET_ACTION(QSUiActionManager::RECORD, this, &QSUiMainWindow::record);

    //file menu
    m_ui->menuFile->addAction(SET_ACTION(QSUiActionManager::PL_ADD_FILE, this, &QSUiMainWindow::addFiles));
    m_ui->menuFile->addAction(SET_ACTION(QSUiActionManager::PL_ADD_DIRECTORY, this, &QSUiMainWindow::addDir));
    m_ui->menuFile->addAction(SET_ACTION(QSUiActionManager::PL_ADD_URL, this, &QSUiMainWindow::addUrl));
    QAction *sep = m_ui->menuFile->addSeparator();
    UiHelper::instance()->registerMenu(UiHelper::ADD_MENU, m_ui->menuFile, false, sep);
    m_ui->menuFile->addAction(SET_ACTION(QSUiActionManager::PL_NEW, m_pl_manager, [this] { m_pl_manager->createPlayList(); }));
    m_ui->menuFile->addAction(SET_ACTION(QSUiActionManager::PL_CLOSE, this, &QSUiMainWindow::removePlaylist));
    m_ui->menuFile->addAction(SET_ACTION(QSUiActionManager::PL_RENAME, this, &QSUiMainWindow::renameTab));
    m_ui->menuFile->addAction(SET_ACTION(QSUiActionManager::PL_SELECT_NEXT, m_pl_manager, &PlayListManager::selectNextPlayList));
    m_ui->menuFile->addAction(SET_ACTION(QSUiActionManager::PL_SELECT_PREVIOUS, m_pl_manager, &PlayListManager::selectPreviousPlayList));
    m_ui->menuFile->addSeparator();
    m_ui->menuFile->addAction(SET_ACTION(QSUiActionManager::PL_LOAD, this, &QSUiMainWindow::loadPlayList));
    m_ui->menuFile->addAction(SET_ACTION(QSUiActionManager::PL_SAVE, this, &QSUiMainWindow::savePlayList));
    m_ui->menuFile->addSeparator();
    m_ui->menuFile->addAction(SET_ACTION(QSUiActionManager::QUIT, m_uiHelper, &UiHelper::exit));
    //edit menu
    m_ui->menuEdit->addAction(SET_ACTION(QSUiActionManager::PL_SELECT_ALL, m_pl_manager, &PlayListManager::selectAll));
    m_ui->menuEdit->addSeparator();
    m_ui->menuEdit->addAction(SET_ACTION(QSUiActionManager::PL_REMOVE_SELECTED, m_listWidget, &QSUiListWidget::removeSelected));
    m_ui->menuEdit->addAction(SET_ACTION(QSUiActionManager::PL_REMOVE_UNSELECTED, m_listWidget, &QSUiListWidget::removeUnselected));
    m_ui->menuEdit->addAction(SET_ACTION(QSUiActionManager::PL_REMOVE_ALL, m_listWidget, &QSUiListWidget::clear));
    m_ui->menuEdit->addAction(SET_ACTION(QSUiActionManager::PL_REMOVE_INVALID, m_pl_manager, &PlayListManager::removeInvalidTracks));
    m_ui->menuEdit->addAction(SET_ACTION(QSUiActionManager::PL_REMOVE_DUPLICATES, m_pl_manager, &PlayListManager::removeDuplicates));
    m_ui->menuEdit->addAction(SET_ACTION(QSUiActionManager::PL_REFRESH, m_pl_manager, &PlayListManager::refresh));
    m_ui->menuEdit->addSeparator();
    //view menu
    if(qApp->platformName() != QLatin1String("wayland"))
    {
        m_ui->menuView->addAction(SET_ACTION(QSUiActionManager::WM_ALLWAYS_ON_TOP, this, &QSUiMainWindow::readSettings));
        m_ui->menuView->addSeparator();
    }
    m_ui->menuView->addAction(m_ui->analyzerDockWidget->toggleViewAction());
    m_ui->menuView->addAction(m_ui->waveformSeekBarDockWidget->toggleViewAction());
    m_ui->menuView->addAction(m_ui->fileSystemDockWidget->toggleViewAction());
    m_ui->menuView->addAction(m_ui->coverDockWidget->toggleViewAction());
    m_ui->menuView->addAction(m_ui->playlistsDockWidget->toggleViewAction());
    QAction *separator = m_ui->menuView->addSeparator();
    m_dockWidgetList->registerMenu(m_ui->menuView, separator);
    m_ui->menuView->addAction(SET_ACTION(QSUiActionManager::UI_SHOW_TABS, m_tabWidget->tabBar(), &QSUiTabBar::setVisible));
    SET_ACTION(QSUiActionManager::UI_SHOW_TABS, m_copySelectedMenu->menuAction(), &QAction::setVisible);

    m_ui->menuView->addAction(ACTION(QSUiActionManager::PL_SHOW_HEADER));
    m_ui->menuView->addAction(SET_ACTION(QSUiActionManager::PL_GROUP_TRACKS, m_ui_settings, &QmmpUiSettings::setGroupsEnabled));
    ACTION(QSUiActionManager::PL_GROUP_TRACKS)->setChecked(m_ui_settings->isGroupsEnabled());
    m_ui->menuView->addSeparator();
    m_ui->menuView->addAction(SET_ACTION(QSUiActionManager::UI_BLOCK_DOCKWIDGETS, this, &QSUiMainWindow::setDockWidgetsBlocked));
    m_ui->menuView->addAction(SET_ACTION(QSUiActionManager::UI_BLOCK_TOOLBARS, this, &QSUiMainWindow::setToolBarsBlocked));
    m_ui->menuView->addSeparator();
    m_ui->menuView->addAction(tr("Edit Toolbars"), this, &QSUiMainWindow::editToolBar);
    m_ui->menuView->addAction(tr("Edit Statusbar"), this, &QSUiMainWindow::editStatusBar);

    QMenu *sortMenu = new QMenu(tr("Sort List"), this);
    sortMenu->setIcon(QIcon::fromTheme(u"view-sort-ascending"_s));
    sortMenu->addAction(tr("By Title"), this, [this]{ m_pl_manager->sort(PlayListModel::TITLE); });
    sortMenu->addAction(tr("By Album"), this, [this]{ m_pl_manager->sort(PlayListModel::ALBUM); });
    sortMenu->addAction(tr("By Artist"), this, [this]{ m_pl_manager->sort(PlayListModel::ARTIST); });
    sortMenu->addAction(tr("By Album Artist"), this, [this]{ m_pl_manager->sort(PlayListModel::ALBUMARTIST); });
    sortMenu->addAction(tr("By Filename"), this, [this]{ m_pl_manager->sort(PlayListModel::FILENAME); });
    sortMenu->addAction(tr("By Path + Filename"), this, [this]{ m_pl_manager->sort(PlayListModel::PATH_AND_FILENAME); });
    sortMenu->addAction(tr("By Date"), this, [this]{ m_pl_manager->sort(PlayListModel::DATE); });
    sortMenu->addAction(tr("By Track Number"), this, [this]{ m_pl_manager->sort(PlayListModel::TRACK); });
    sortMenu->addAction(tr("By Disc Number"), this, [this]{ m_pl_manager->sort(PlayListModel::DISCNUMBER); });
    sortMenu->addAction(tr("By File Creation Date"), this, [this]{ m_pl_manager->sort(PlayListModel::FILE_CREATION_DATE); });
    sortMenu->addAction(tr("By File Modification Date"), this, [this]{ m_pl_manager->sort(PlayListModel::FILE_MODIFICATION_DATE); });
    sortMenu->addAction(tr("By Group"), this, [this]{ m_pl_manager->sort(PlayListModel::GROUP); });
    m_ui->menuEdit->addMenu(sortMenu);

    QMenu *sortSelectionMenu = new QMenu (tr("Sort Selection"), this);
    sortSelectionMenu->setIcon(QIcon::fromTheme(u"view-sort-ascending"_s));
    sortSelectionMenu->addAction(tr("By Title"), this, [this]{ m_pl_manager->sortSelection(PlayListModel::TITLE); });
    sortSelectionMenu->addAction(tr("By Album"), this, [this]{ m_pl_manager->sortSelection(PlayListModel::ALBUM); });
    sortSelectionMenu->addAction(tr("By Artist"), this, [this]{ m_pl_manager->sortSelection(PlayListModel::ARTIST); });
    sortSelectionMenu->addAction(tr("By Album Artist"), this, [this]{ m_pl_manager->sortSelection(PlayListModel::ALBUMARTIST); });
    sortSelectionMenu->addAction(tr("By Filename"), this, [this]{ m_pl_manager->sortSelection(PlayListModel::FILENAME); });
    sortSelectionMenu->addAction(tr("By Path + Filename"), this, [this]{ m_pl_manager->sortSelection(PlayListModel::PATH_AND_FILENAME); });
    sortSelectionMenu->addAction(tr("By Date"), this, [this]{ m_pl_manager->sortSelection(PlayListModel::DATE); });
    sortSelectionMenu->addAction(tr("By Track Number"), this, [this]{ m_pl_manager->sortSelection(PlayListModel::TRACK); });
    sortSelectionMenu->addAction(tr("By Disc Number"), this, [this]{ m_pl_manager->sortSelection(PlayListModel::DISCNUMBER); });
    sortSelectionMenu->addAction(tr("By File Creation Date"), this, [this]{ m_pl_manager->sortSelection(PlayListModel::FILE_CREATION_DATE); });
    sortSelectionMenu->addAction(tr("By File Modification Date"), this, [this]{ m_pl_manager->sortSelection(PlayListModel::FILE_MODIFICATION_DATE ); });
    m_ui->menuEdit->addMenu(sortSelectionMenu);

    m_ui->menuEdit->addSeparator();
    m_ui->menuEdit->addAction(QIcon::fromTheme(u"media-playlist-shuffle"_s), tr("Randomize List"),
                             m_pl_manager, &PlayListManager::randomizeList);
    m_ui->menuEdit->addAction(QIcon::fromTheme(u"view-sort-descending"_s), tr("Reverse List"),
                              m_pl_manager, &PlayListManager::reverseList);
    m_ui->menuEdit->addSeparator();
    m_ui->menuEdit->addAction(SET_ACTION(QSUiActionManager::SETTINGS, this, &QSUiMainWindow::showSettings));

    //playback menu
    m_ui->menuPlayback->addAction(ACTION(QSUiActionManager::PLAY));
    m_ui->menuPlayback->addAction(ACTION(QSUiActionManager::STOP));
    m_ui->menuPlayback->addAction(ACTION(QSUiActionManager::PAUSE));
    m_ui->menuPlayback->addAction(ACTION(QSUiActionManager::NEXT));
    m_ui->menuPlayback->addAction(ACTION(QSUiActionManager::PREVIOUS));
    m_ui->menuPlayback->addAction(SET_ACTION(QSUiActionManager::PLAY_PAUSE, this, &QSUiMainWindow::playPause));
    m_ui->menuPlayback->addSeparator();
    m_ui->menuPlayback->addAction(SET_ACTION(QSUiActionManager::JUMP, this, &QSUiMainWindow::jumpTo));
    m_ui->menuPlayback->addSeparator();
    m_ui->menuPlayback->addAction(ACTION(QSUiActionManager::PL_ENQUEUE));
    m_ui->menuPlayback->addAction(SET_ACTION(QSUiActionManager::CLEAR_QUEUE, m_pl_manager, &PlayListManager::clearQueue));
    m_ui->menuPlayback->addSeparator();
    m_ui->menuPlayback->addAction(SET_ACTION(QSUiActionManager::REPEAT_ALL, m_ui_settings, &QmmpUiSettings::setRepeatableList));
    m_ui->menuPlayback->addAction(SET_ACTION(QSUiActionManager::REPEAT_TRACK, m_ui_settings, &QmmpUiSettings::setRepeatableTrack));
    m_ui->menuPlayback->addAction(SET_ACTION(QSUiActionManager::SHUFFLE, m_ui_settings, &QmmpUiSettings::setShuffle));
    m_ui->menuPlayback->addAction(SET_ACTION(QSUiActionManager::NO_PL_ADVANCE, m_ui_settings, &QmmpUiSettings::setNoPlayListAdvance));
    m_ui->menuPlayback->addAction(SET_ACTION(QSUiActionManager::TRANSIT_BETWEEN_PLAYLISTS, m_ui_settings, &QmmpUiSettings::setPlayListTransitionEnabled));
    m_ui->menuPlayback->addAction(SET_ACTION(QSUiActionManager::STOP_AFTER_SELECTED, m_pl_manager, &PlayListManager::stopAfterSelected));
    m_ui->menuPlayback->addSeparator();
    m_ui->menuPlayback->addAction(SET_ACTION(QSUiActionManager::VOL_ENC, m_core, &SoundCore::volumeUp));
    m_ui->menuPlayback->addAction(SET_ACTION(QSUiActionManager::VOL_DEC, m_core, &SoundCore::volumeDown));
    m_ui->menuPlayback->addAction(SET_ACTION(QSUiActionManager::VOL_MUTE, m_core, &SoundCore::setMuted));
    connect(m_core, &SoundCore::mutedChanged, ACTION(QSUiActionManager::VOL_MUTE), &QAction::setChecked);

    //help menu
    m_ui->menuHelp->addAction(SET_ACTION(QSUiActionManager::ABOUT_UI, this, &QSUiMainWindow::aboutUi));
    m_ui->menuHelp->addAction(SET_ACTION(QSUiActionManager::ABOUT, this, &QSUiMainWindow::about));
    m_ui->menuHelp->addAction(SET_ACTION(QSUiActionManager::ABOUT_QT, qApp, &QApplication::aboutQt));
    //playlist menu
    m_pl_menu->addAction(SET_ACTION(QSUiActionManager::PL_SHOW_INFO, m_pl_manager, &PlayListManager::showDetails));
    m_pl_menu->addSeparator();
    m_pl_menu->addMenu(m_copySelectedMenu);
    m_pl_menu->addAction(ACTION(QSUiActionManager::PL_REMOVE_SELECTED));
    m_pl_menu->addAction(ACTION(QSUiActionManager::PL_REMOVE_ALL));
    m_pl_menu->addAction(ACTION(QSUiActionManager::PL_REMOVE_UNSELECTED));
    m_pl_menu->addMenu(UiHelper::instance()->createMenu(UiHelper::PLAYLIST_MENU, tr("Actions"), true, this));
    m_pl_menu->addSeparator();
    m_pl_menu->addAction(SET_ACTION(QSUiActionManager::PL_ENQUEUE, m_pl_manager, &PlayListManager::addToQueue));
    //tools menu
    m_ui->menuTools->addAction(SET_ACTION(QSUiActionManager::EQUALIZER, this, &QSUiMainWindow::showEqualizer));
    m_uiHelper->registerMenu(UiHelper::TOOLS_MENU, m_ui->menuTools, false, ACTION(QSUiActionManager::EQUALIZER));
    //tab menu
    m_tab_menu->addAction(ACTION(QSUiActionManager::PL_LOAD));
    m_tab_menu->addAction(ACTION(QSUiActionManager::PL_SAVE));
    m_tab_menu->addSeparator();
    m_tab_menu->addAction(ACTION(QSUiActionManager::PL_RENAME));
    m_tab_menu->addAction(ACTION(QSUiActionManager::PL_CLOSE));
    //seeking
    SET_ACTION(QSUiActionManager::SEEK_FORWARD_10, this, [this] { m_core->seekRelative(10000); } );
    SET_ACTION(QSUiActionManager::SEEK_FORWARD_30, this, [this] { m_core->seekRelative(30000); } );
    SET_ACTION(QSUiActionManager::SEEK_FORWARD_60, this, [this] { m_core->seekRelative(60000); } );
    SET_ACTION(QSUiActionManager::SEEK_BACKWARD_10, this, [this] { m_core->seekRelative(-10000); } );
    SET_ACTION(QSUiActionManager::SEEK_BACKWARD_30, this, [this] { m_core->seekRelative(-30000); } );
    SET_ACTION(QSUiActionManager::SEEK_BACKWARD_60, this, [this] { m_core->seekRelative(-60000); } );
    //application menu
    SET_ACTION(QSUiActionManager::APPLICATION_MENU, this, &QSUiMainWindow::showAppMenu);
    //menu bar
    SET_ACTION(QSUiActionManager::UI_SHOW_MENUBAR, menuBar(), &QMenuBar::setVisible);
    //status bar
    SET_ACTION(QSUiActionManager::UI_SHOW_STATUSBAR, statusBar(), &QStatusBar::setVisible);

    addActions(QSUiActionManager::instance()->actions());
    addActions(m_key_manager->actions());
}

void QSUiMainWindow::readSettings()
{
    QSettings settings;
    settings.beginGroup(u"Simple"_s);
    m_titleFormatter.setPattern(settings.value(u"window_title_format"_s, u"%if(%p,%p - %t,%t)"_s).toString());

    //update toolbars
    const QList<QSUiActionManager::ToolBarInfo> toolBarInfoList = QSUiActionManager::instance()->readToolBarSettings();
    QList<QToolBar *> toolBars = findChildren<QToolBar*>();

    //clear toolbars to avoid conflicts
    for(QToolBar *toolBar : std::as_const(toolBars))
        toolBar->clear();

    for(const QSUiActionManager::ToolBarInfo &info : std::as_const(toolBarInfoList))
    {
        bool found = false;
        QList<QToolBar *>::iterator it = toolBars.begin();
        while(it != toolBars.end())
        {
            if((*it)->property("uid").toString() == info.uid)
            {
                found = true;
                QSUiActionManager::instance()->updateToolBar(*it, info);
                toolBars.erase(it);
                break;
            }
            ++it;
        }

        if(!found)
        {
            addToolBar(QSUiActionManager::instance()->createToolBar(info, this));
        }
    }

    for(QToolBar *toolBar : std::as_const(toolBars))
        toolBar->deleteLater();
    toolBars.clear();
    setToolBarsBlocked(ACTION(QSUiActionManager::UI_BLOCK_TOOLBARS)->isChecked());

    if(m_update)
    {
        m_listWidget->readSettings();
        qobject_cast<FileSystemBrowser *> (m_ui->fileSystemDockWidget->widget())->readSettings();

        if(ACTION(QSUiActionManager::WM_ALLWAYS_ON_TOP)->isChecked())
            setWindowFlags(windowFlags() | Qt::WindowStaysOnTopHint);
        else
            setWindowFlags(windowFlags() & ~Qt::WindowStaysOnTopHint);

        if(m_core->state() == Qmmp::Playing || m_core->state() == Qmmp::Paused)
            showMetaData();

        m_tabWidget->readSettings();

        show();
    }
    else
    {
        restoreGeometry(settings.value(u"mw_geometry"_s).toByteArray());
        ACTION(QSUiActionManager::UI_SHOW_MENUBAR)->setChecked(settings.value(u"show_menubar"_s, true).toBool());
        menuBar()->setVisible(ACTION(QSUiActionManager::UI_SHOW_MENUBAR)->isChecked());
        ACTION(QSUiActionManager::UI_SHOW_STATUSBAR)->setChecked(settings.value(u"show_statusbar"_s, true).toBool());
        statusBar()->setVisible( ACTION(QSUiActionManager::UI_SHOW_STATUSBAR)->isChecked());

        QByteArray wstate = settings.value(u"mw_state"_s).toByteArray();
        if(wstate.isEmpty())
        {
            m_ui->fileSystemDockWidget->hide();
            m_ui->coverDockWidget->hide();
            m_ui->playlistsDockWidget->hide();
            m_ui->waveformSeekBarDockWidget->hide();
        }
        else
            restoreState(settings.value(u"mw_state"_s).toByteArray());
        if(settings.value(u"always_on_top"_s, false).toBool())
        {
            ACTION(QSUiActionManager::WM_ALLWAYS_ON_TOP)->setChecked(true);
            setWindowFlags(windowFlags() | Qt::WindowStaysOnTopHint);
        }
        show();
        qApp->processEvents();
        if(settings.value(u"start_hidden"_s).toBool())
            hide();

        bool state = settings.value(u"block_dockwidgets"_s, false).toBool();
        ACTION(QSUiActionManager::UI_BLOCK_DOCKWIDGETS)->setChecked(state);
        setDockWidgetsBlocked(state);

        state = settings.value(u"show_tabs"_s, true).toBool();
        ACTION(QSUiActionManager::UI_SHOW_TABS)->setChecked(state);
        m_tabWidget->setTabsVisible(state);

        state = settings.value(u"block_toolbars"_s, false).toBool();
        ACTION(QSUiActionManager::UI_BLOCK_TOOLBARS)->setChecked(state);
        setToolBarsBlocked(state);

        m_copySelectedMenu->menuAction()->setVisible(ACTION(QSUiActionManager::UI_SHOW_TABS)->isChecked());

        m_update = true;
    }

    m_hideOnClose = settings.value(u"hide_on_close"_s, false).toBool();

    if(settings.value(u"pl_show_new_pl_button"_s, false).toBool())
    {
        m_tabWidget->setCornerWidget(m_addListButton, Qt::TopLeftCorner);
        m_addListButton->setIconSize(QSize(16, 16));
        m_addListButton->setVisible(true);
    }
    else
    {
        m_addListButton->setVisible(false);
        m_tabWidget->setCornerWidget(nullptr, Qt::TopLeftCorner);
    }
    if(settings.value(u"pl_show_tab_list_menu"_s, false).toBool())
    {
        m_tabWidget->setCornerWidget(m_tabListMenuButton, Qt::TopRightCorner);
        m_tabListMenuButton->setIconSize(QSize(16, 16));
        m_tabListMenuButton->setVisible(true);
    }
    else
    {
        m_tabListMenuButton->setVisible(false);
        m_tabWidget->setCornerWidget(nullptr, Qt::TopRightCorner);
    }

    settings.endGroup();
    addActions(m_uiHelper->actions(UiHelper::TOOLS_MENU));
    addActions(m_uiHelper->actions(UiHelper::PLAYLIST_MENU));

    //record action
    EffectFactory *fileWriterFactory = Effect::findFactory(u"filewriter"_s);
    if(fileWriterFactory)
    {
        ACTION(QSUiActionManager::RECORD)->setEnabled(true);
        ACTION(QSUiActionManager::RECORD)->setChecked(Effect::isEnabled(fileWriterFactory));
    }
    else
    {
        ACTION(QSUiActionManager::RECORD)->setEnabled(false);
        ACTION(QSUiActionManager::RECORD)->setChecked(false);
    }
}

void QSUiMainWindow::showTabMenu(const QPoint &pos)
{
    int index = m_tabWidget->tabBar()->tabAt(m_tabWidget->tabBar()->mapFromParent(pos));
    if(index == -1)
    {
        QMenu *menu = createPopupMenu();
        menu->setAttribute(Qt::WA_DeleteOnClose, true);
        menu->popup(m_tabWidget->mapToGlobal(pos));
    }
    else
    {
        m_pl_manager->selectPlayListIndex(index);
        m_tab_menu->popup(m_tabWidget->mapToGlobal(pos));
    }
}

void QSUiMainWindow::writeSettings()
{
    QSettings settings;
    settings.setValue(u"Simple/mw_geometry"_s, saveGeometry());
    settings.setValue(u"Simple/mw_state"_s, saveState());
    settings.setValue(u"Simple/always_on_top"_s, ACTION(QSUiActionManager::WM_ALLWAYS_ON_TOP)->isChecked());
    settings.setValue(u"Simple/show_tabs"_s, ACTION(QSUiActionManager::UI_SHOW_TABS)->isChecked());
    settings.setValue(u"Simple/block_dockwidgets"_s, ACTION(QSUiActionManager::UI_BLOCK_DOCKWIDGETS)->isChecked());
    settings.setValue(u"Simple/block_toolbars"_s, ACTION(QSUiActionManager::UI_BLOCK_TOOLBARS)->isChecked());
    settings.setValue(u"Simple/show_menubar"_s, ACTION(QSUiActionManager::UI_SHOW_MENUBAR)->isChecked());
    settings.setValue(u"Simple/show_statusbar"_s, ACTION(QSUiActionManager::UI_SHOW_STATUSBAR)->isChecked());
}

void QSUiMainWindow::savePlayList()
{
    m_uiHelper->savePlayList(this);
}

void QSUiMainWindow::loadPlayList()
{
    m_uiHelper->loadPlayList(this);
}

void QSUiMainWindow::showEqualizer()
{
    QSUiEqualizer equalizer(this);
    equalizer.exec();
}

void QSUiMainWindow::showMetaData()
{
    PlayListModel *model = m_pl_manager->currentPlayList();
    PlayListTrack *track = model->currentTrack();
    if(track && track->path() == m_core->trackInfo().path())
    {
        setWindowTitle(m_titleFormatter.format(track));
    }
}

void QSUiMainWindow::setDockWidgetsBlocked(bool blocked)
{
    m_dockWidgetList->setTitleBarsVisible(!blocked);

    QList<QDockWidget *> widgetList = {
        m_ui->analyzerDockWidget,
        m_ui->fileSystemDockWidget,
        m_ui->coverDockWidget,
        m_ui->playlistsDockWidget,
        m_ui->waveformSeekBarDockWidget
    };

    if(blocked)
    {
        for(QDockWidget *w : std::as_const(widgetList))
        {
            if(!w->titleBarWidget())
                w->setTitleBarWidget(new QWidget());
        }
    }
    else
    {
        for(QDockWidget *w : std::as_const(widgetList))
        {
            QWidget *widget = w->titleBarWidget();
            if(widget)
            {
                w->setTitleBarWidget(nullptr);
                delete widget;
            }
        }
    }
}

void QSUiMainWindow::setToolBarsBlocked(bool blocked)
{
    for(QToolBar *t : findChildren<QToolBar *>())
    {
        t->setMovable(!blocked);
    }
}

void QSUiMainWindow::editToolBar()
{
    QSUiToolBarEditor *e = new QSUiToolBarEditor(this);
    if(e->exec() == QDialog::Accepted)
    {
        readSettings();
    }
    e->deleteLater();
}

void QSUiMainWindow::editStatusBar()
{
    QSUiStatusBarEditor *e = new QSUiStatusBarEditor(this);
    if(e->exec() == QDialog::Accepted)
    {
        m_statusBar->readSettings();
    }
    e->deleteLater();
}

void QSUiMainWindow::restoreWindowTitle()
{
    setWindowTitle(tr("x-AMP"));
}

void QSUiMainWindow::onListChanged(int flags)
{
    if(flags & PlayListModel::STRUCTURE)
        m_statusBar->updatePlayListStatus();
}

void QSUiMainWindow::onCurrentPlayListChanged(PlayListModel *current, PlayListModel *previous)
{
    updateTabs();
    m_statusBar->updatePlayListStatus();
    connect(current, &PlayListModel::listChanged, this, &QSUiMainWindow::onListChanged);
    if(previous)
        disconnect(current, &PlayListModel::listChanged, this, &QSUiMainWindow::onListChanged);
}

void QSUiMainWindow::generateCopySelectedMenu()
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

void QSUiMainWindow::copySelectedMenuActionTriggered(QAction *action)
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
