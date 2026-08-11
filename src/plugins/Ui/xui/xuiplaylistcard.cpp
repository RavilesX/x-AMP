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
#include <QPainterPath>
#include <QSettings>
#include <QVBoxLayout>
#include <qmmpui/mediaplayer.h>
#include <qmmpui/playlistmanager.h>
#include <qmmpui/playlistmodel.h>
#include <qmmpui/uihelper.h>
#include "xuitheme.h"
#include "xuicontrols.h"
#include "xuilistview.h"
#include "xuisettings.h"
#include "xuiplaylistcard.h"

XUiPlaylistCard::XUiPlaylistCard(QWidget *parent) : QWidget(parent)
{
    m_uiHelper = UiHelper::instance();
    m_player = MediaPlayer::instance();
    m_manager = PlayListManager::instance();

    QVBoxLayout *root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    m_header = buildHeader();
    root->addWidget(m_header);

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

    reloadBackground();
}

QWidget *XUiPlaylistCard::buildHeader()
{
    QWidget *header = new QWidget(this);
    header->setFixedHeight(XUi::CardHeaderHeight);
    QHBoxLayout *layout = new QHBoxLayout(header);
    layout->setContentsMargins(XUi::CardPadding, 0, XUi::CardPadding, 0);
    layout->setSpacing(4);

    QLabel *title = new QLabel(tr("PLAYLIST"), header);
    //decorative: this bar is the window's drag handle
    title->setAttribute(Qt::WA_TransparentForMouseEvents);
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

    XUiIconButton *close = new XUiIconButton(XUiIcons::Close, header);
    close->setToolTip(tr("Hide the playlist"));
    connect(close, &XUiIconButton::clicked, this, &XUiPlaylistCard::closeRequested);

    layout->addWidget(search);
    layout->addWidget(close);
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

void XUiPlaylistCard::reloadBackground()
{
    const QString path = QSettings().value(XUiSettings::BackgroundKey).toString();
    m_backdrop = path.isEmpty() ? QImage() : QImage(path);
    //an unreadable or deleted file leaves a null image, which simply means no
    //engraving -- not worth an error the user would meet on every startup
    buildEngraving();
    update();
}

void XUiPlaylistCard::buildEngraving()
{
    m_engraving = QPixmap();
    if(m_backdrop.isNull() || width() <= 0 || height() <= 0)
        return;

    const qreal ratio = devicePixelRatioF();
    const QSize target = (QSizeF(size()) * ratio).toSize();

    //Cover the card and crop what hangs over, rather than stretching: a
    //photograph squeezed to the card's proportions is the one thing that
    //would give the effect away as a pasted-in picture.
    QImage grey = m_backdrop.scaled(target, Qt::KeepAspectRatioByExpanding,
                                    Qt::SmoothTransformation)
                      .convertToFormat(QImage::Format_Grayscale8);
    grey = grey.copy(QRect(QPoint((grey.width() - target.width()) / 2,
                                  (grey.height() - target.height()) / 2), target));

    //The engraving itself. Not a greyscale: the 256 levels of brightness are
    //mapped onto a ramp that starts at the card's own colour and rises only
    //DEPTH in lightness at the same hue and saturation, so the image reads as
    //the surface being lit rather than as a picture lying on top of it.
    constexpr float DEPTH = 0.115f;
    const float h = XUi::Card.hslHueF() < 0 ? 0.0f : XUi::Card.hslHueF();
    const float s = XUi::Card.hslSaturationF();
    const float l = XUi::Card.lightnessF();
    QList<QRgb> ramp;
    ramp.reserve(256);
    for(int i = 0; i < 256; ++i)
        ramp.append(QColor::fromHslF(h, s, qMin(1.0f, l + DEPTH * i / 255.0f)).rgb());

    //Grayscale8 and Indexed8 hold one byte per pixel alike, so the ramp can be
    //hung off the existing buffer instead of walking every pixel by hand.
    //`grey` has to outlive `tinted`, which borrows its memory.
    QImage tinted(grey.constBits(), grey.width(), grey.height(),
                  grey.bytesPerLine(), QImage::Format_Indexed8);
    tinted.setColorTable(ramp);
    QImage engraving = tinted.convertToFormat(QImage::Format_ARGB32_Premultiplied);

    //Fade towards the top. The first rows of the list are the ones most often
    //read, and the header sits over the same pixels, so the image is let in
    //gradually and only reaches full strength near the footer.
    {
        QPainter fading(&engraving);
        fading.setCompositionMode(QPainter::CompositionMode_DestinationIn);
        QLinearGradient fade(0, 0, 0, target.height());
        fade.setColorAt(0.0, QColor(0, 0, 0, 30));
        fade.setColorAt(0.35, QColor(0, 0, 0, 105));
        fade.setColorAt(1.0, QColor(0, 0, 0, 165));
        fading.fillRect(engraving.rect(), fade);
    }

    //Flattened onto the card's own gradient, in a format that has no alpha
    //channel at all. The window is translucent so its corners can be rounded,
    //which means anything left partly transparent here is not blended with the
    //card behind: it is a hole through to the desktop. RGB32 cannot make one.
    QImage flat(target, QImage::Format_RGB32);
    QPainter p(&flat);
    QLinearGradient card(0, 0, 0, target.height());
    card.setColorAt(0.0, XUi::CardTop);
    card.setColorAt(1.0, XUi::Card);
    p.fillRect(flat.rect(), card);
    p.drawImage(0, 0, engraving);
    p.end();

    m_engraving = QPixmap::fromImage(flat);
    m_engraving.setDevicePixelRatio(ratio);
}

void XUiPlaylistCard::resizeEvent(QResizeEvent *e)
{
    buildEngraving();
    QWidget::resizeEvent(e);
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

    if(!m_engraving.isNull())
    {
        //clipped a pixel inside the card so the image cannot eat the border it
        //sits within
        QPainterPath inside;
        inside.addRoundedRect(QRectF(rect()).adjusted(1, 1, -1, -1),
                              XUi::CardRadius - 1, XUi::CardRadius - 1);
        p.save();
        p.setClipPath(inside);
        p.drawPixmap(0, 0, m_engraving);
        p.restore();
    }

    p.setPen(QPen(XUi::Border, 1));
    p.drawLine(QPointF(1, XUi::CardHeaderHeight), QPointF(width() - 1, XUi::CardHeaderHeight));
}
