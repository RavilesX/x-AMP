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
    setMinimumSize(560, 320);
    resize(720, 470);

    QVBoxLayout *root = new QVBoxLayout(this);
    //margin leaves room for the rounded corners to show the desktop through
    root->setContentsMargins(XUi::CardGap, 0, XUi::CardGap, XUi::CardGap);
    root->setSpacing(XUi::CardGap);

    m_titleBar = buildTitleBar();
    root->addWidget(m_titleBar);

    m_playerCard = new XUiPlayerCard(this);
    root->addWidget(m_playerCard);

    //the equalizer and playlist cards land here in phases 5.2 and 5.3
    root->addStretch(1);

    connect(m_core, &SoundCore::trackInfoChanged, this, &XUiMainWindow::updateWindowTitle);
    connect(m_player, &MediaPlayer::playbackFinished, this, &XUiMainWindow::updateWindowTitle);
    connect(m_uiHelper, &UiHelper::showMainWindowCalled, this, &QWidget::show);

    readSettings();
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
        m_mainMenu->addAction(tr("&Add File..."), this, [this] { m_uiHelper->addFiles(this); });
        m_mainMenu->addAction(tr("Add &Directory..."), this, [this] { m_uiHelper->addDirectory(this); });
        m_mainMenu->addSeparator();
        m_mainMenu->addAction(tr("&Preferences..."), this, [this] {
            //phase 5.4 adds this interface's own pages; the shared ones are
            //enough to configure output, plugins and audio meanwhile
            ConfigDialog dialog(this);
            dialog.exec();
        });
        m_mainMenu->addAction(tr("&About x-AMP"), this, [this] { m_uiHelper->about(this); });
        m_mainMenu->addSeparator();
        m_mainMenu->addAction(tr("&Quit"), this, [this] { m_uiHelper->exit(); });
    }
    m_mainMenu->exec(mapToGlobal(QPoint(XUi::CardGap, XUi::TitleBarHeight)));
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
    if(size.isValid())
        resize(size);
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
    QWidget::closeEvent(e);
    m_uiHelper->exit();
}
