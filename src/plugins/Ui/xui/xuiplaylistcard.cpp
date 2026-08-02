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

#include <QHBoxLayout>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QPainter>
#include <QVBoxLayout>
#include <qmmpui/mediaplayer.h>
#include <qmmpui/playlistmanager.h>
#include <qmmpui/playlistmodel.h>
#include <qmmpui/uihelper.h>
#include "xuitheme.h"
#include "xuicontrols.h"
#include "xuilistview.h"
#include "xuiplaylistcard.h"

XUiPlaylistCard::XUiPlaylistCard(QWidget *parent) : QWidget(parent)
{
    m_uiHelper = UiHelper::instance();
    m_player = MediaPlayer::instance();
    m_manager = PlayListManager::instance();

    QVBoxLayout *root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    root->addWidget(buildHeader());

    m_list = new XUiListView(m_manager, this);
    connect(m_list, &XUiListView::activated, this, [this] {
        //MediaPlayer::play() only resumes when paused -- it never looks at the
        //current track. Without the stop, activating a different row while
        //paused carried on with the old track behind the new row's label.
        m_player->stop();
        m_player->play();
    });
    root->addWidget(m_list, 1);

    root->addWidget(buildFooter());
}

QWidget *XUiPlaylistCard::buildHeader()
{
    QWidget *header = new QWidget(this);
    header->setFixedHeight(XUi::CardHeaderHeight);
    QHBoxLayout *layout = new QHBoxLayout(header);
    layout->setContentsMargins(XUi::CardPadding, 0, XUi::CardPadding, 0);
    layout->setSpacing(4);

    QLabel *title = new QLabel(tr("PLAYLIST"), header);
    QFont f = title->font();
    f.setBold(true);
    f.setPointSizeF(f.pointSizeF() * 1.05);
    f.setLetterSpacing(QFont::AbsoluteSpacing, 1.4);
    title->setFont(f);
    QPalette pal = title->palette();
    pal.setColor(QPalette::WindowText, XUi::Text);
    title->setPalette(pal);
    layout->addWidget(title);

    m_search = new QLineEdit(header);
    m_search->setPlaceholderText(tr("Search tracks..."));
    m_search->setFixedHeight(30);
    m_search->hide(); //revealed by the search button
    connect(m_search, &QLineEdit::textChanged, this, [this](const QString &text) {
        m_list->setFilter(text);
    });
    layout->addWidget(m_search, 1);
    layout->addStretch(1);

    //adding and playlists live in the footer; only search is here, since it
    //acts on this header's own field
    XUiIconButton *search = new XUiIconButton(XUiIcons::Search, header);
    search->setToolTip(tr("Search (Ctrl+F)"));
    connect(search, &XUiIconButton::clicked, this, &XUiPlaylistCard::toggleSearch);
    layout->addWidget(search);
    return header;
}

QWidget *XUiPlaylistCard::buildFooter()
{
    QWidget *footer = new QWidget(this);
    footer->setFixedHeight(46);
    QHBoxLayout *layout = new QHBoxLayout(footer);
    layout->setContentsMargins(XUi::CardPadding, 0, XUi::CardPadding, 12);
    layout->setSpacing(8);

    //Glyphs rather than the abbreviations these had: "Sel" and "Lst" say
    //little, and mixing drawn glyphs with typed +/- would show in the stroke
    //weight. The tooltips carry the meaning.
    auto add = [&](XUiIcons::Icon icon, const QString &tip,
                   void (XUiPlaylistCard::*slot)()) {
        XUiMenuButton *button = new XUiMenuButton(icon, footer);
        button->setToolTip(tip);
        connect(button, &XUiMenuButton::clicked, this, slot);
        layout->addWidget(button);
        return button;
    };

    add(XUiIcons::Plus, tr("Add tracks"), &XUiPlaylistCard::showAddMenu);
    add(XUiIcons::Minus, tr("Remove tracks"), &XUiPlaylistCard::showRemoveMenu);
    add(XUiIcons::SelectAll, tr("Select tracks"), &XUiPlaylistCard::showSelectMenu);
    layout->addStretch(1);
    m_playlists = add(XUiIcons::List, tr("Playlists"),
                      &XUiPlaylistCard::showPlaylistsMenu);
    return footer;
}

void XUiPlaylistCard::toggleSearch()
{
    m_search->setVisible(!m_search->isVisible());
    if(m_search->isVisible())
        m_search->setFocus();
    else
        m_search->clear(); //hiding it must not leave the list filtered
}

void XUiPlaylistCard::showAddMenu()
{
    QMenu menu(this);
    connect(menu.addAction(tr("Add &File...")), &QAction::triggered,
            this, [this] { m_uiHelper->addFiles(this); });
    connect(menu.addAction(tr("Add &Directory...")), &QAction::triggered,
            this, [this] { m_uiHelper->addDirectory(this); });
    connect(menu.addAction(tr("Add &URL...")), &QAction::triggered,
            this, [this] { m_uiHelper->addUrl(this); });
    menu.exec(QCursor::pos());
}

void XUiPlaylistCard::showRemoveMenu()
{
    PlayListModel *model = m_manager->selectedPlayList();
    QMenu menu(this);
    connect(menu.addAction(tr("Remove &Selected")), &QAction::triggered,
            model, &PlayListModel::removeSelected);
    connect(menu.addAction(tr("Remove &All")), &QAction::triggered,
            model, &PlayListModel::clear);
    menu.exec(QCursor::pos());
}

void XUiPlaylistCard::showSelectMenu()
{
    PlayListModel *model = m_manager->selectedPlayList();
    QMenu menu(this);
    connect(menu.addAction(tr("Select &All")), &QAction::triggered, this, [model] {
        model->setSelectedLines(0, model->trackCount() - 1, true);
    });
    connect(menu.addAction(tr("Select &None")), &QAction::triggered, this, [model] {
        model->setSelectedLines(0, model->trackCount() - 1, false);
    });
    menu.exec(QCursor::pos());
    update();
}

void XUiPlaylistCard::showPlaylistsMenu()
{
    QMenu menu(this);
    const QStringList names = m_manager->playListNames();
    for(int i = 0; i < names.size(); ++i)
    {
        QAction *action = menu.addAction(names.at(i));
        action->setCheckable(true);
        action->setChecked(i == m_manager->selectedPlayListIndex());
        connect(action, &QAction::triggered, this, [this, i] {
            m_manager->selectPlayListIndex(i);
        });
    }
    menu.addSeparator();
    connect(menu.addAction(tr("&New Playlist")), &QAction::triggered, this, [this] {
        bool accepted = false;
        const QString name = QInputDialog::getText(this, tr("New Playlist"), tr("Name:"),
                                                   QLineEdit::Normal, tr("Playlist"),
                                                   &accepted);
        if(accepted && !name.isEmpty())
            m_manager->selectPlayList(m_manager->createPlayList(name));
    });
    connect(menu.addAction(tr("&Remove Playlist")), &QAction::triggered, this, [this] {
        m_manager->removePlayList(m_manager->selectedPlayList());
    });
    menu.exec(QCursor::pos());
}

void XUiPlaylistCard::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);
    QLinearGradient g(rect().topLeft(), rect().bottomLeft());
    g.setColorAt(0.0, XUi::CardTop);
    g.setColorAt(1.0, XUi::Card);
    p.setPen(QPen(XUi::Border, 1));
    p.setBrush(g);
    p.drawRoundedRect(QRectF(rect()).adjusted(0.5, 0.5, -0.5, -0.5),
                      XUi::CardRadius, XUi::CardRadius);
    p.setPen(QPen(XUi::Border, 1));
    p.drawLine(QPointF(1, XUi::CardHeaderHeight), QPointF(width() - 1, XUi::CardHeaderHeight));
}
