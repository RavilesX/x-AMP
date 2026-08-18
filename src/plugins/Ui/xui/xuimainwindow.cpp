/***************************************************************************
 *   Copyright (C) 2026 by x-AMP contributors                              *
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

#include <QAction>
#include <QApplication>
#include <QHBoxLayout>
#include <QLabel>
#include <QMenu>
#include <QShortcut>
#include <QMouseEvent>
#include <QColor>
#include <QGuiApplication>
#include <QScreen>
#include <QSettings>
#include <QVBoxLayout>
#include <qmmp/soundcore.h>
#include <qmmpui/mediaplayer.h>
#include <qmmpui/playlistmanager.h>
#include <qmmpui/uihelper.h>
#include <qmmpui/configdialog.h>
#include "xuitheme.h"
#include "xuicontrols.h"
#include "xuiplayercard.h"
#include "xuiequalizercard.h"
#include "xuiplaylistcard.h"
#include "xuisettings.h"
#include "xuistyle.h"
#include "xuidialogs.h"
#include "xuidock.h"
#include "xuiwindow.h"
#include "xuimainwindow.h"

namespace
{
    //first-run sizes: the three windows stacked come to roughly what the
    //single window used to be
    constexpr int COLUMN_WIDTH = 560;
    constexpr int PLAYER_HEIGHT = 330;
    constexpr int EQUALIZER_HEIGHT = 250;
    constexpr int PLAYLIST_HEIGHT = 380;
}

XUiMainWindow::XUiMainWindow(QWidget *parent) : XUiWindow(QStringLiteral("player"), parent)
{
    m_uiHelper = UiHelper::instance();
    m_core = SoundCore::instance();
    m_player = MediaPlayer::instance();
    m_playListManager = PlayListManager::instance();

    //menus, tooltips and dialogs are plain Qt widgets and would otherwise
    //follow the desktop's theme, which need not be dark like the cards
    //the accent has to be in place before the palette and sheet are built,
    //since both bake it in
    XUi::setAccent(QSettings().value(XUi::AccentKey, XUi::defaultAccent()).value<QColor>());

    //Applied to the whole application rather than to this window: the shared
    //dialogs from libqmmpui (track details, preferences) are separate
    //top-level windows, and only one interface is ever loaded per process,
    //so nothing else is affected. Order matters -- setStyle resets the
    //palette, so it goes first.
    qApp->setStyle(new XUiStyle);
    qApp->setPalette(XUi::palette());
    qApp->setStyleSheet(XUi::styleSheet());
    //dialogs drop the window manager's grey frame; their accent border and
    //their own buttons take over from it
    XUiDialogs::install();
    //This window is now only the title bar and the player; the equalizer and
    //the playlist each get one of their own, so all three can be moved and
    //sized apart and snapped back together.
    QVBoxLayout *root = new QVBoxLayout(this);
    //margin leaves room for the rounded corners to show the desktop through
    root->setContentsMargins(XUi::CardGap, XUi::CardGap / 2, XUi::CardGap, XUi::CardGap);
    root->setSpacing(XUi::CardGap);

    m_titleBar = buildTitleBar();
    root->addWidget(m_titleBar);
    setDragHandle(m_titleBar);

    m_playerCard = new XUiPlayerCard(this);
    connect(m_playerCard, &XUiPlayerCard::schedulerRequested,
            this, &XUiMainWindow::showSchedulerSettings);
    root->addWidget(m_playerCard, 1);

    XUiDock::instance()->setMainWindow(this);

    auto companion = [this](QWidget *card, const QString &key, const QString &title) {
        //Qt::Tool, and owned by the player: the three then minimise and raise
        //together and only the player takes a slot in the task bar, which is
        //how the skinned interface's windows behave.
        XUiWindow *window = new XUiWindow(key, this);
        window->setWindowFlags(Qt::Tool | Qt::FramelessWindowHint);
        window->setWindowTitle(title);
        QVBoxLayout *layout = new QVBoxLayout(window);
        layout->setContentsMargins(XUi::CardGap, XUi::CardGap,
                                   XUi::CardGap, XUi::CardGap);
        layout->addWidget(card);
        card->setParent(window);
        return window;
    };

    m_equalizerCard = new XUiEqualizerCard;
    m_equalizerWindow = companion(m_equalizerCard, QStringLiteral("equalizer"),
                                  tr("x-AMP Equalizer"));
    //the card's own header bar is what drags its window
    m_equalizerWindow->setDragHandle(m_equalizerCard->header());
    connect(m_equalizerCard, &XUiEqualizerCard::closeRequested, this, [this] {
        hideCompanion(XUiSettings::ShowEqualizerKey, m_equalizerAction);
    });

    m_playlistCard = new XUiPlaylistCard;
    m_playlistWindow = companion(m_playlistCard, QStringLiteral("playlist"),
                                 tr("x-AMP Playlist"));
    m_playlistWindow->setDragHandle(m_playlistCard->header());
    connect(m_playlistCard, &XUiPlaylistCard::closeRequested, this, [this] {
        hideCompanion(XUiSettings::ShowPlaylistKey, m_playlistAction);
    });

    connect(m_core, &SoundCore::trackInfoChanged, this, &XUiMainWindow::updateWindowTitle);
    connect(m_player, &MediaPlayer::playbackFinished, this, &XUiMainWindow::updateWindowTitle);
    connect(m_uiHelper, &UiHelper::showMainWindowCalled, this, [this] {
        show();
        applyCardVisibility(); //the companions were hidden along with us
    });

    updateWordmark();
    createShortcuts();
    readSettings();
    updateWindowTitle();
    //the starter only constructs the interface; showing it is ours to do
    show();
    //after show(): placing a companion needs this window's real geometry
    stackCompanions();
    applyCardVisibility();
}

XUiMainWindow::~XUiMainWindow() = default;

QWidget *XUiMainWindow::buildTitleBar()
{
    QWidget *bar = new QWidget(this);
    bar->setFixedHeight(XUi::TitleBarHeight);
    bar->setMouseTracking(true);

    QHBoxLayout *layout = new QHBoxLayout(bar);
    layout->setContentsMargins(4, 0, 0, 0);
    layout->setSpacing(4);

    XUiIconButton *menuButton = new XUiIconButton(XUiIcons::Menu, bar);
    connect(menuButton, &XUiIconButton::clicked, this, &XUiMainWindow::showMainMenu);
    layout->addWidget(menuButton);

    //wordmark: the leading X carries the accent, as in the brand. Kept as a
    //member because the colour is baked into its markup, so it has to be
    //rewritten when the accent changes.
    m_wordmark = new QLabel(bar);
    //decorative: this bar is the window's drag handle
    m_wordmark->setAttribute(Qt::WA_TransparentForMouseEvents);
    QFont f = m_wordmark->font();
    f.setPointSizeF(f.pointSizeF() * 1.25);
    f.setBold(true);
    m_wordmark->setFont(f);
    layout->addSpacing(6);
    layout->addWidget(m_wordmark);
    layout->addStretch(1);

    XUiIconButton *minimise = new XUiIconButton(XUiIcons::Minimize, bar);
    connect(minimise, &XUiIconButton::clicked, this, &QWidget::showMinimized);
    XUiIconButton *maximise = new XUiIconButton(XUiIcons::Maximize, bar);
    connect(maximise, &XUiIconButton::clicked, this, &XUiMainWindow::toggleMaximised);
    XUiIconButton *close = new XUiIconButton(XUiIcons::Close, bar);
    connect(close, &XUiIconButton::clicked, this, [this] { m_uiHelper->exit(); });
    layout->addWidget(minimise);
    layout->addWidget(maximise);
    layout->addWidget(close);
    return bar;
}

void XUiMainWindow::showMainMenu()
{
    if(!m_mainMenu)
    {
        m_mainMenu = new QMenu(this);

        QMenu *file = m_mainMenu->addMenu(tr("&Add"));
        file->addAction(tr("Add &File..."), QKeySequence(tr("Ctrl+Shift+A")),
                        this, [this] { m_uiHelper->addFiles(this); });
        file->addAction(tr("Add &Directory..."), QKeySequence(tr("Ctrl+Shift+D")),
                        this, [this] { m_uiHelper->addDirectory(this); });
        file->addAction(tr("Add &URL..."), QKeySequence(tr("Ctrl+U")),
                        this, [this] { m_uiHelper->addUrl(this); });

        QMenu *playback = m_mainMenu->addMenu(tr("&Playback"));
        playback->addAction(tr("&Play/Pause"), QKeySequence(Qt::Key_Space),
                            m_player, &MediaPlayer::pause);
        playback->addAction(tr("&Stop"), QKeySequence(tr("Ctrl+.")),
                            m_player, &MediaPlayer::stop);
        playback->addAction(tr("&Next"), QKeySequence(tr("Ctrl+Right")),
                            m_player, &MediaPlayer::next);
        playback->addAction(tr("P&revious"), QKeySequence(tr("Ctrl+Left")),
                            m_player, &MediaPlayer::previous);

        //view toggles mirror the preferences page, so both stay in step
        QMenu *view = m_mainMenu->addMenu(tr("&View"));
        view->addAction(tr("&Search Playlist"), QKeySequence(tr("Ctrl+F")),
                        m_playlistCard, &XUiPlaylistCard::toggleSearch);
        view->addSeparator();
        QSettings settings;
        m_equalizerAction = view->addAction(tr("&Equalizer"));
        m_equalizerAction->setCheckable(true);
        m_equalizerAction->setChecked(settings.value(XUiSettings::ShowEqualizerKey,
                                                     true).toBool());
        connect(m_equalizerAction, &QAction::toggled, this, [this](bool on) {
            QSettings().setValue(XUiSettings::ShowEqualizerKey, on);
            applyCardVisibility();
        });
        m_playlistAction = view->addAction(tr("&Playlist"));
        m_playlistAction->setCheckable(true);
        m_playlistAction->setChecked(settings.value(XUiSettings::ShowPlaylistKey,
                                                    true).toBool());
        connect(m_playlistAction, &QAction::toggled, this, [this](bool on) {
            QSettings().setValue(XUiSettings::ShowPlaylistKey, on);
            applyCardVisibility();
        });

        m_mainMenu->addSeparator();
        m_mainMenu->addAction(tr("&Preferences..."), QKeySequence(tr("Ctrl+P")),
                              this, &XUiMainWindow::showPreferences);
        m_mainMenu->addAction(tr("&About x-AMP"), this, [this] { m_uiHelper->about(this); });
        m_mainMenu->addSeparator();
        m_mainMenu->addAction(tr("&Quit"), QKeySequence(tr("Ctrl+Q")),
                              this, [this] { m_uiHelper->exit(); });
    }
    m_mainMenu->exec(mapToGlobal(QPoint(XUi::CardGap, XUi::TitleBarHeight)));
}

void XUiMainWindow::showPreferences()
{
    openPreferences(false);
}

void XUiMainWindow::showSchedulerSettings()
{
    openPreferences(true);
}

void XUiMainWindow::openPreferences(bool scheduler)
{
    ConfigDialog dialog(this);
    XUiSettings *page = new XUiSettings(&dialog);
    connect(page, &XUiSettings::accentApplied, this, &XUiMainWindow::applyAccent);
    connect(page, &XUiSettings::backgroundApplied,
            m_playlistCard, &XUiPlaylistCard::reloadBackground);
    //the other pages carry icons, so ours would sit oddly without one
    dialog.addPage(tr("Interface"), page, QIcon(QStringLiteral(":/xui/interface.png")));

    //ConfigDialog restores its size and never its position, so Qt centres it
    //on this window. It is nearly twice the height of the player, which put
    //its title bar above the top of the screen whenever the player sat near
    //it -- and a dialog whose title bar is off screen cannot be moved back.
    //Anchoring it to this window's own corner keeps it where the eye already
    //is; the clamp is for the opposite case, a player near the bottom right.
    const QScreen *display = screen() ? screen() : QGuiApplication::primaryScreen();
    if(display)
    {
        const QRect area = display->availableGeometry();
        const int x = qMin(pos().x(), qMax(area.left(), area.right() + 1 - dialog.width()));
        const int y = qMin(pos().y(), qMax(area.top(), area.bottom() + 1 - dialog.height()));
        dialog.move(qMax(area.left(), x), qMax(area.top(), y));
    }

    if(scheduler)
        dialog.showSchedulerSettings();

    if(dialog.exec() == QDialog::Accepted)
    {
        page->writeSettings();
        applyAccent();
        applyCardVisibility();
        m_playlistCard->reloadBackground();
    }
}

void XUiMainWindow::createShortcuts()
{
    //Shortcuts on the window rather than the menu: the menu is built lazily,
    //so its own shortcuts would not work until it had been opened once.
    struct { const char *keys; void (MediaPlayer::*slot)(); } bindings[] = {
        { "Space",      &MediaPlayer::pause },
        { "Ctrl+.",     &MediaPlayer::stop },
        { "Ctrl+Right", &MediaPlayer::next },
        { "Ctrl+Left",  &MediaPlayer::previous },
    };
    for(const auto &binding : bindings)
        new QShortcut(QKeySequence(QLatin1String(binding.keys)), this,
                      m_player, binding.slot);

    new QShortcut(QKeySequence(QStringLiteral("Ctrl+P")), this,
                  this, &XUiMainWindow::showPreferences);
    new QShortcut(QKeySequence(QStringLiteral("Ctrl+Shift+A")), this,
                  this, [this] { m_uiHelper->addFiles(this); });
    new QShortcut(QKeySequence(QStringLiteral("Ctrl+Shift+D")), this,
                  this, [this] { m_uiHelper->addDirectory(this); });
    new QShortcut(QKeySequence(QStringLiteral("Ctrl+F")), this,
                  m_playlistCard, &XUiPlaylistCard::toggleSearch);
    new QShortcut(QKeySequence(QStringLiteral("Ctrl+Q")), this,
                  this, [this] { m_uiHelper->exit(); });
}

void XUiMainWindow::updateWordmark()
{
    m_wordmark->setText(QStringLiteral("<span style='color:%1'>X</span>"
                                       "<span style='color:%2'>-AMP</span>")
                        .arg(XUi::Accent.name(), XUi::Text.name()));
}

void XUiMainWindow::applyAccent()
{
    const QColor chosen = QSettings().value(XUi::AccentKey,
                                            XUi::defaultAccent()).value<QColor>();
    if(chosen == XUi::Accent)
        return;

    XUi::setAccent(chosen);
    //the palette and the sheet both hold copies of the colour, so they have to
    //be rebuilt; the cards paint themselves and only need telling to repaint
    qApp->setPalette(XUi::palette());
    qApp->setStyleSheet(XUi::styleSheet());
    updateWordmark();
    //labels keep their colour in a palette, which a repaint does not revisit
    m_playerCard->applyAccent();
    const QList<QWidget *> children = findChildren<QWidget *>();
    for(QWidget *child : children)
        child->update();
    update();
}

void XUiMainWindow::stackCompanions()
{
    QSettings settings;
    struct { XUiWindow *window; QString key; int height; } items[] = {
        { m_equalizerWindow, QStringLiteral("XUi/equalizer_geometry"), EQUALIZER_HEIGHT },
        { m_playlistWindow, QStringLiteral("XUi/playlist_geometry"), PLAYLIST_HEIGHT },
    };

    int y = geometry().bottom() + 1;
    for(const auto &item : items)
    {
        item.window->restoreGeometry(QSize(width(), item.height));
        if(settings.contains(item.key))
            continue; //placed by the user already; leave it where it was

        //first run: line them up under the player, flush and touching, so the
        //three read as one window until the user pulls them apart
        item.window->resize(width(), item.height);
        item.window->move(geometry().left(), y);
        //the asked-for height, not height(): a window that is not on screen
        //yet may still be reporting whatever its layout last worked out
        y += item.height + 1;
    }
}

void XUiMainWindow::hideCompanion(const QString &key, QAction *action)
{
    //Only the explicit gestures -- this button, the menu, the preferences --
    //write the setting. A window closed on the way out of the application must
    //not be read as the user putting the card away, or every card would come
    //back unticked after a quit.
    QSettings().setValue(key, false);
    if(action)
        action->setChecked(false); //keeps the menu in step with the button
    applyCardVisibility();
}

void XUiMainWindow::applyCardVisibility()
{
    QSettings settings;
    m_hideOnClose = settings.value(XUiSettings::HideOnCloseKey, false).toBool();

    //Each card is a window of its own now, so showing one is no longer the
    //height arithmetic the single window needed -- every window keeps the
    //geometry it was last given.
    auto toggle = [](XUiWindow *window, bool show) {
        if(show)
            window->show();
        else if(!window->isHidden())
        {
            window->saveGeometry();
            window->hide();
        }
    };

    toggle(m_equalizerWindow, settings.value(XUiSettings::ShowEqualizerKey, true).toBool());
    toggle(m_playlistWindow, settings.value(XUiSettings::ShowPlaylistKey, true).toBool());
}

void XUiMainWindow::toggleMaximised()
{
    if(isMaximized())
        showNormal();
    else
        showMaximized();
}

void XUiMainWindow::updateWindowTitle()
{
    const QString title = m_core->trackInfo().value(Qmmp::TITLE);
    setWindowTitle(title.isEmpty() ? QStringLiteral("x-AMP")
                                   : QStringLiteral("%1 - x-AMP").arg(title));
}

void XUiMainWindow::mouseDoubleClickEvent(QMouseEvent *e)
{
    if(e->button() == Qt::LeftButton && m_titleBar->geometry().contains(e->pos()))
        toggleMaximised();
}

void XUiMainWindow::readSettings()
{
    {
        //left behind by the single-window layout, where one size and position
        //covered all three cards
        QSettings settings;
        settings.remove(QStringLiteral("XUi/size"));
        settings.remove(QStringLiteral("XUi/position"));
    }

    //the player card alone is far shorter than the old single window
    if(restoreGeometry(QSize(COLUMN_WIDTH, PLAYER_HEIGHT)))
        return;

    //Nothing saved: place the window rather than leaving it to the window
    //manager, which would drop it wherever it pleased -- and stackCompanions()
    //reads this geometry to line the other two up underneath.
    QScreen *screen = QGuiApplication::primaryScreen();
    if(!screen)
        return;
    const QRect available = screen->availableGeometry();
    const int column = PLAYER_HEIGHT + EQUALIZER_HEIGHT + PLAYLIST_HEIGHT + 2;
    move(available.left() + (available.width() - width()) / 2,
         qMax(available.top(), available.top() + (available.height() - column) / 2));
}

void XUiMainWindow::writeSettings()
{
    saveGeometry();
    //a hidden companion saved its geometry when it was hidden
    if(!m_equalizerWindow->isHidden())
        m_equalizerWindow->saveGeometry();
    if(!m_playlistWindow->isHidden())
        m_playlistWindow->saveGeometry();
}

void XUiMainWindow::closeEvent(QCloseEvent *e)
{
    writeSettings();
    if(m_hideOnClose && m_uiHelper->visibilityControl())
    {
        //something else can bring us back (tray icon, MPRIS, --toggle-visibility)
        hide();
        //the companions are windows of their own now: left alone they would
        //stay on screen after the player had gone
        m_equalizerWindow->hide();
        m_playlistWindow->hide();
        e->ignore();
        return;
    }
    QWidget::closeEvent(e);
    //Put away here rather than left to the closeAllWindows() the exit runs:
    //that reaches them only after the player has begun unwinding -- the audio
    //engine thread, the playlists being written -- and until it does the two
    //cards stand on screen belonging to a player that is already gone. The
    //branch above hides them by hand for the same reason.
    m_equalizerWindow->hide();
    m_playlistWindow->hide();
    m_uiHelper->exit();
}
