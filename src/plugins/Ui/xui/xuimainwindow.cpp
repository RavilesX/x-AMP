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

#include <QApplication>
#include <QHBoxLayout>
#include <QLabel>
#include <QMenu>
#include <QShortcut>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QSettings>
#include <QVBoxLayout>
#include <QWindow>
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
#include "xuimainwindow.h"

namespace
{
    //grab area for resizing along the window border
    constexpr int RESIZE_MARGIN = 6;
}

XUiMainWindow::XUiMainWindow(QWidget *parent) : QWidget(parent)
{
    m_uiHelper = UiHelper::instance();
    m_core = SoundCore::instance();
    m_player = MediaPlayer::instance();
    m_playListManager = PlayListManager::instance();

    setWindowFlags(Qt::Window | Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TranslucentBackground); //rounded corners need this
    setMouseTracking(true);
    //menus, tooltips and dialogs are plain Qt widgets and would otherwise
    //follow the desktop's theme, which need not be dark like the cards
    //Applied to the whole application rather than to this window: the shared
    //dialogs from libqmmpui (track details, preferences) are separate
    //top-level windows, and only one interface is ever loaded per process,
    //so nothing else is affected. Order matters -- setStyle resets the
    //palette, so it goes first.
    qApp->setStyle(new XUiStyle);
    qApp->setPalette(XUi::palette());
    qApp->setStyleSheet(XUi::styleSheet());
    //no hand-picked minimum: an explicit one smaller than the cards need lets
    //Qt squeeze them past their own minimums, which crushes the player card.
    //Let the layout derive it instead.
    resize(700, 720);

    QVBoxLayout *root = new QVBoxLayout(this);
    //margin leaves room for the rounded corners to show the desktop through
    root->setContentsMargins(XUi::CardGap, 0, XUi::CardGap, XUi::CardGap);
    root->setSpacing(XUi::CardGap);

    m_titleBar = buildTitleBar();
    root->addWidget(m_titleBar);

    m_playerCard = new XUiPlayerCard(this);
    root->addWidget(m_playerCard);

    m_equalizerCard = new XUiEqualizerCard(this);
    root->addWidget(m_equalizerCard);

    m_playlistCard = new XUiPlaylistCard(this);
    root->addWidget(m_playlistCard, 1); //the list absorbs spare height

    connect(m_core, &SoundCore::trackInfoChanged, this, &XUiMainWindow::updateWindowTitle);
    connect(m_player, &MediaPlayer::playbackFinished, this, &XUiMainWindow::updateWindowTitle);
    connect(m_uiHelper, &UiHelper::showMainWindowCalled, this, &QWidget::show);

    createShortcuts();
    readSettings();
    applyCardVisibility();
    updateWindowTitle();
    //the starter only constructs the interface; showing it is ours to do
    show();
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

    //wordmark: the leading X carries the accent, as in the brand
    QLabel *wordmark = new QLabel(bar);
    QFont f = wordmark->font();
    f.setPointSizeF(f.pointSizeF() * 1.25);
    f.setBold(true);
    wordmark->setFont(f);
    wordmark->setText(QStringLiteral("<span style='color:%1'>X</span>"
                                     "<span style='color:%2'>-AMP</span>")
                      .arg(XUi::Accent.name(), XUi::Text.name()));
    layout->addSpacing(6);
    layout->addWidget(wordmark);
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
        file->addAction(tr("Add &File..."), QKeySequence(tr("Ctrl+F")),
                        this, [this] { m_uiHelper->addFiles(this); });
        file->addAction(tr("Add &Directory..."), QKeySequence(tr("Ctrl+D")),
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
        QSettings settings;
        QAction *equalizer = view->addAction(tr("&Equalizer"));
        equalizer->setCheckable(true);
        equalizer->setChecked(settings.value(XUiSettings::ShowEqualizerKey, true).toBool());
        connect(equalizer, &QAction::toggled, this, [this](bool on) {
            QSettings().setValue(XUiSettings::ShowEqualizerKey, on);
            applyCardVisibility();
        });
        QAction *playlist = view->addAction(tr("&Playlist"));
        playlist->setCheckable(true);
        playlist->setChecked(settings.value(XUiSettings::ShowPlaylistKey, true).toBool());
        connect(playlist, &QAction::toggled, this, [this](bool on) {
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
    ConfigDialog dialog(this);
    XUiSettings *page = new XUiSettings(&dialog);
    dialog.addPage(tr("Interface"), page);
    if(dialog.exec() == QDialog::Accepted)
    {
        page->writeSettings();
        applyCardVisibility();
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
    new QShortcut(QKeySequence(QStringLiteral("Ctrl+F")), this,
                  this, [this] { m_uiHelper->addFiles(this); });
    new QShortcut(QKeySequence(QStringLiteral("Ctrl+D")), this,
                  this, [this] { m_uiHelper->addDirectory(this); });
    new QShortcut(QKeySequence(QStringLiteral("Ctrl+Q")), this,
                  this, [this] { m_uiHelper->exit(); });
}

void XUiMainWindow::applyCardVisibility()
{
    QSettings settings;
    const bool showEqualizer = settings.value(XUiSettings::ShowEqualizerKey, true).toBool();
    const bool showPlaylist = settings.value(XUiSettings::ShowPlaylistKey, true).toBool();
    m_hideOnClose = settings.value(XUiSettings::HideOnCloseKey, false).toBool();

    //Grow and shrink the window by exactly what each card occupied, rather
    //than snapping to sizeHint(): that discarded whatever height the user had
    //dragged the window to, so re-showing a card reset it to the default.
    int delta = 0;
    auto toggle = [&](QWidget *card, bool show, int *remembered) {
        if(card->isVisible() == show)
            return;
        if(show)
        {
            delta += (*remembered > 0 ? *remembered : card->sizeHint().height()) + XUi::CardGap;
        }
        else
        {
            *remembered = card->height();
            delta -= card->height() + XUi::CardGap;
        }
        card->setVisible(show);
    };

    const bool first = !isVisible(); //startup: nothing to grow or shrink yet
    toggle(m_equalizerCard, showEqualizer, &m_equalizerHeight);
    toggle(m_playlistCard, showPlaylist, &m_playlistHeight);

    if(!first && delta != 0)
        resize(width(), qMax(minimumSizeHint().height(), height() + delta));
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

Qt::Edges XUiMainWindow::edgesAt(const QPoint &pos) const
{
    if(isMaximized())
        return {};
    Qt::Edges edges;
    if(pos.x() <= RESIZE_MARGIN)
        edges |= Qt::LeftEdge;
    if(pos.x() >= width() - RESIZE_MARGIN)
        edges |= Qt::RightEdge;
    if(pos.y() <= RESIZE_MARGIN)
        edges |= Qt::TopEdge;
    if(pos.y() >= height() - RESIZE_MARGIN)
        edges |= Qt::BottomEdge;
    return edges;
}

void XUiMainWindow::mouseMoveEvent(QMouseEvent *e)
{
    const Qt::Edges edges = edgesAt(e->pos());
    if((edges & Qt::LeftEdge && edges & Qt::TopEdge) ||
       (edges & Qt::RightEdge && edges & Qt::BottomEdge))
        setCursor(Qt::SizeFDiagCursor);
    else if((edges & Qt::RightEdge && edges & Qt::TopEdge) ||
            (edges & Qt::LeftEdge && edges & Qt::BottomEdge))
        setCursor(Qt::SizeBDiagCursor);
    else if(edges & (Qt::LeftEdge | Qt::RightEdge))
        setCursor(Qt::SizeHorCursor);
    else if(edges & (Qt::TopEdge | Qt::BottomEdge))
        setCursor(Qt::SizeVerCursor);
    else
        unsetCursor();
}

void XUiMainWindow::leaveEvent(QEvent *)
{
    //Qt sends Leave when the pointer moves onto a child widget too. Without
    //clearing here, a resize cursor picked up at the window edge stayed set
    //and every child without a cursor of its own inherited it -- the pointer
    //kept showing the horizontal resize arrows over the playlist.
    unsetCursor();
}

void XUiMainWindow::mousePressEvent(QMouseEvent *e)
{
    if(e->button() != Qt::LeftButton)
        return;

    //Hand both gestures to the compositor. Moving the window by hand from
    //mouseMoveEvent works on X11 but not on Wayland, where clients are not
    //allowed to position themselves.
    const Qt::Edges edges = edgesAt(e->pos());
    if(edges)
    {
        windowHandle()->startSystemResize(edges);
        return;
    }
    if(m_titleBar->geometry().contains(e->pos()))
        windowHandle()->startSystemMove();
}

void XUiMainWindow::mouseDoubleClickEvent(QMouseEvent *e)
{
    if(e->button() == Qt::LeftButton && m_titleBar->geometry().contains(e->pos()))
        toggleMaximised();
}

void XUiMainWindow::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);
    QRectF body = QRectF(rect()).adjusted(XUi::CardGap - 1, 0,
                                          -(XUi::CardGap - 1), -(XUi::CardGap - 1));
    p.setPen(QPen(XUi::Border, 1));
    p.setBrush(XUi::Background);
    p.drawRoundedRect(body, XUi::WindowRadius, XUi::WindowRadius);
}

void XUiMainWindow::readSettings()
{
    QSettings settings;
    settings.beginGroup(QStringLiteral("XUi"));
    const QSize size = settings.value(QStringLiteral("size")).toSize();
    //expandToMinimum: a size saved before a card was added would clip it
    if(size.isValid())
        resize(size.expandedTo(minimumSizeHint()));
    const QPoint pos = settings.value(QStringLiteral("position")).toPoint();
    if(!pos.isNull())
        move(pos);
    settings.endGroup();
}

void XUiMainWindow::writeSettings()
{
    QSettings settings;
    settings.beginGroup(QStringLiteral("XUi"));
    if(!isMaximized())
    {
        settings.setValue(QStringLiteral("size"), size());
        settings.setValue(QStringLiteral("position"), pos());
    }
    settings.endGroup();
}

void XUiMainWindow::closeEvent(QCloseEvent *e)
{
    writeSettings();
    if(m_hideOnClose && m_uiHelper->visibilityControl())
    {
        //something else can bring us back (tray icon, MPRIS, --toggle-visibility)
        hide();
        e->ignore();
        return;
    }
    QWidget::closeEvent(e);
    m_uiHelper->exit();
}
