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

#include <QCoreApplication>
#include <QKeyEvent>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QPainter>
#include <QPainterPath>
#include <QScrollArea>
#include <QScrollBar>
#include <QSettings>
#include <QVBoxLayout>
#include <QWheelEvent>
#include <qmmpui/mediaplayer.h>
#include <qmmpui/playlistmanager.h>
#include <qmmpui/playlistmodel.h>
#include <qmmpui/uihelper.h>
#include "xuitheme.h"
#include "xuicontrols.h"
#include "xuilistview.h"
#include "xuisettings.h"
#include "xuiplaylistcard.h"

namespace
{
    /*!
     * The strip of tabs is cut wherever the header runs out of room. This
     * sits over that cut and fades it into the card, so a clipped name reads
     * as "there is more this way" instead of as a drawing fault.
     */
    class TabFade : public QWidget //no signals of its own, so no Q_OBJECT
    {
    public:
        TabFade(Qt::Edge side, QWidget *parent) : QWidget(parent), m_side(side)
        {
            setAttribute(Qt::WA_TransparentForMouseEvents);
        }

        static constexpr int Width = 26;

    protected:
        void paintEvent(QPaintEvent *) override
        {
            //The card's gradient has barely moved this near its top, so the
            //header's own colour is CardTop for all the eye can tell.
            QColor clear = XUi::CardTop;
            clear.setAlpha(0);
            QLinearGradient g(0, 0, width(), 0);
            const bool right = m_side == Qt::RightEdge;
            g.setColorAt(0.0, right ? clear : XUi::CardTop);
            g.setColorAt(1.0, right ? XUi::CardTop : clear);
            QPainter(this).fillRect(rect(), g);
        }

    private:
        Qt::Edge m_side;
    };
}

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
        //the search found what it was for, so it has no more to say
        hideSearch();
    });
    m_list->installEventFilter(this); //Escape, while the search is open
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
    //Fixed, so that a long row of tabs is the thing that gives way when the
    //card is narrow. Left to itself the label shrinks to nothing and the
    //header loses the one word saying what the card is.
    title->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
    layout->addWidget(title);

    header->installEventFilter(this);

    m_search = new QLineEdit(header);
    m_search->setPlaceholderText(tr("Search tracks..."));
    m_search->setFixedHeight(30);
    m_search->hide(); //revealed by the search button
    m_search->installEventFilter(this); //Escape gives up on it
    connect(m_search, &QLineEdit::textChanged, this, [this](const QString &text) {
        m_list->setFilter(text);
    });
    layout->addWidget(m_search, 1);
    //A gap as wide as the title itself is all that a full row of tabs has to
    //leave free: they may run the length of the bar and stop just short of
    //the word. It stretches, so a short row still sits at the far end.
    layout->addSpacerItem(new QSpacerItem(title->sizeHint().width(), 0,
                                          QSizePolicy::Expanding,
                                          QSizePolicy::Minimum));

    //One tab per playlist, gathered at the far end of the bar beside the two
    //glyphs: the footer's Playlists menu still lists them all, but switching
    //should not cost a menu.
    //Stretched, so that the room the header has over is offered to the tabs
    //first and to the gap before them only once they have all they need. With
    //no factor here the two shared it and the row was cut off mid-header.
    layout->addWidget(buildTabs(), 1);

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

QWidget *XUiPlaylistCard::buildTabs()
{
    QWidget *strip = new QWidget(this);
    m_tabLayout = new QHBoxLayout(strip);
    m_tabLayout->setContentsMargins(0, 0, 0, 0);
    m_tabLayout->setSpacing(5);

    //Nothing here is marked transparent for mouse events, tempting as that is
    //to keep the whole header draggable: the attribute takes the widget's
    //children down with it, and the tabs would stop answering the pointer.
    //The row is only as wide as its tabs, so the rest of the bar still drags.
    m_tabArea = new QScrollArea(this);
    m_tabArea->setWidget(strip);
    m_tabArea->setWidgetResizable(true);
    m_tabArea->setFrameShape(QFrame::NoFrame);
    m_tabArea->setFixedHeight(28);
    m_tabArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_tabArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_tabArea->viewport()->setAutoFillBackground(false);
    //A resizable scroll area passes its widget's minimum width on as its own,
    //which here is the whole row of tabs -- enough to push the title out of a
    //narrow header. The row is what should give way, so it is allowed to.
    m_tabArea->setMinimumWidth(0);
    //Expanding, so the row takes whatever the header has free rather than the
    //narrow width a scroll area asks for on its own. It never crosses into
    //the title: it is capped at the width its own tabs need (see
    //rebuildTabs()), and the spacer before it holds a gap open regardless.
    m_tabArea->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    m_tabArea->setStyleSheet(u"QScrollArea, QScrollArea > QWidget > QWidget"
                              " { background: transparent; }"_s);

    //Parented to the viewport, not to the scroll area: the area restacks its
    //viewport above its own children whenever it lays itself out, which left
    //the fade drawn and then covered over.
    m_tabFadeLeft = new TabFade(Qt::LeftEdge, m_tabArea->viewport());
    m_tabFadeRight = new TabFade(Qt::RightEdge, m_tabArea->viewport());
    m_tabFadeLeft->hide(); //each is wanted only once that edge cuts a tab
    m_tabFadeRight->hide();
    m_tabArea->installEventFilter(this);
    m_tabArea->viewport()->installEventFilter(this);
    connect(m_tabArea->horizontalScrollBar(), &QScrollBar::valueChanged,
            this, [this] { updateTabFade(); });
    connect(m_tabArea->horizontalScrollBar(), &QScrollBar::rangeChanged,
            this, [this] { updateTabFade(); });

    connect(m_manager, &PlayListManager::playListsChanged,
            this, &XUiPlaylistCard::rebuildTabs);
    connect(m_manager, &PlayListManager::selectedPlayListChanged,
            this, &XUiPlaylistCard::syncTabs);
    connect(m_manager, &PlayListManager::currentPlayListChanged,
            this, &XUiPlaylistCard::syncTabs);
    rebuildTabs();
    return m_tabArea;
}

void XUiPlaylistCard::rebuildTabs()
{
    while(QLayoutItem *item = m_tabLayout->takeAt(0))
    {
        //deleteLater rather than delete: a rebuild can be set off from inside
        //a tab's own event handler -- renaming one from its context menu does
        //exactly that -- and freeing the widget under its handler is a crash.
        //The item is null for the trailing stretch, which is fine.
        if(QWidget *old = item->widget())
        {
            old->hide();
            old->deleteLater();
        }
        delete item;
    }

    const QStringList names = m_manager->playListNames();
    int wanted = 0;
    for(int i = 0; i < names.size(); ++i)
    {
        XUiTabButton *tab = new XUiTabButton(names.at(i), m_tabArea->widget());
        wanted += tab->sizeHint().width() + (i ? m_tabLayout->spacing() : 0);
        connect(tab, &XUiTabButton::clicked, this, [this, i] {
            m_manager->selectPlayListIndex(i);
        });
        connect(tab, &XUiTabButton::menuRequested, this, [this, i] {
            showTabMenu(i);
        });
        m_tabLayout->addWidget(tab);
        //shown here rather than left to the event loop: a hidden widget is
        //nothing to the layout, so the row would measure as empty and the new
        //tab would have no geometry to be scrolled to
        tab->show();
    }
    //Right-aligned: the strip is stretched to the viewport whenever the tabs
    //do not fill it, and the leading stretch keeps them against its far end
    //instead of spread across the width.
    m_tabLayout->insertStretch(0, 1);

    //As wide as the tabs need and no wider, so the rest of the header is left
    //for the title and for dragging the window. The width is added up from
    //the tabs themselves rather than asked of the layout: a tab built here is
    //not shown until the event loop gets round to it, and until then the
    //layout counts it as empty -- which capped the row at nothing at all on
    //every rebuild after the first.
    //the layout's own figure rather than the sum above, which cannot know
    //what the spacing and the margins add
    m_tabLayout->activate();
    m_tabArea->setMaximumWidth(qMax(wanted, m_tabLayout->sizeHint().width()));
    //A scroll area measures its widget once and keeps the answer, so after a
    //rebuild it still asked the header for room for the tabs there used to
    //be: a playlist added to a row that already fitted was left with nowhere
    //to go. A layout request is what drops that cached size.
    QEvent request(QEvent::LayoutRequest);
    QCoreApplication::sendEvent(m_tabArea, &request);
    m_tabArea->updateGeometry();

    //the width above changes how much of the header the row gets, and what is
    //visible of it is what decides where scrolling has to land
    if(m_header && m_header->layout())
        m_header->layout()->activate();
    updateTabFade();
    syncTabs();
    //Scrolling to the selected tab needs it to have a geometry, and a tab
    //built here has none until the layout has run: a playlist created while
    //others are already there was left off the end of the row, out of sight.
    QMetaObject::invokeMethod(this, &XUiPlaylistCard::syncTabs, Qt::QueuedConnection);
}

void XUiPlaylistCard::syncTabs()
{
    const int selected = m_manager->selectedPlayListIndex();
    const int playing = m_manager->currentPlayListIndex();
    //the row is laid out on demand: without this the tabs of a rebuild still
    //sit at nothing wide, and scrolling to one of them goes nowhere
    if(QLayout *strip = m_tabArea->widget()->layout())
        strip->activate();
    //the layout carries a stretch of its own, so count the tabs rather than
    //reading the playlist's index off the item's place in it
    int index = 0;
    for(int i = 0; i < m_tabLayout->count(); ++i)
    {
        XUiTabButton *tab = qobject_cast<XUiTabButton *>(m_tabLayout->itemAt(i)->widget());
        if(!tab)
            continue;
        tab->setChecked(index == selected);
        //only worth marking when it is not the list on screen anyway
        tab->setPlaying(playing == index && playing != selected);
        if(index == selected)
            m_tabArea->ensureWidgetVisible(tab);
        ++index;
    }
}

void XUiPlaylistCard::showTabMenu(int index)
{
    PlayListModel *model = m_manager->playListAt(index);
    if(!model)
        return;

    QMenu menu(this);
    connect(menu.addAction(tr("&Rename...")), &QAction::triggered, this, [this, model] {
        bool accepted = false;
        const QString name = QInputDialog::getText(this, tr("Rename Playlist"), tr("Name:"),
                                                   QLineEdit::Normal, model->name(),
                                                   &accepted);
        if(accepted && !name.isEmpty())
            model->setName(name);
    });
    connect(menu.addAction(tr("&Remove Playlist")), &QAction::triggered, this, [this, model] {
        m_manager->removePlayList(model);
    });
    menu.exec(QCursor::pos());
}

void XUiPlaylistCard::updateTabFade()
{
    const QScrollBar *bar = m_tabArea->horizontalScrollBar();
    QWidget *viewport = m_tabArea->viewport();
    m_tabFadeLeft->setGeometry(0, 0, TabFade::Width, viewport->height());
    m_tabFadeRight->setGeometry(viewport->width() - TabFade::Width, 0,
                                TabFade::Width, viewport->height());
    m_tabFadeLeft->setVisible(bar->value() > bar->minimum());
    m_tabFadeRight->setVisible(bar->value() < bar->maximum());
    m_tabFadeLeft->raise();
    m_tabFadeRight->raise();
}

bool XUiPlaylistCard::eventFilter(QObject *watched, QEvent *event)
{
    //The header is filtered from the moment it is built, which is before the
    //row of tabs inside it exists; its first events arrive with nothing to act
    //on yet.
    if(!m_tabArea)
        return QWidget::eventFilter(watched, event);

    if(watched == m_tabArea && event->type() == QEvent::Resize)
        updateTabFade();

    //Escape gives up on the search from the field or from the list, rather
    //than leaving it open until the glyph is pressed a second time
    if((watched == m_search || watched == m_list)
       && event->type() == QEvent::KeyPress && m_search->isVisible()
       && static_cast<QKeyEvent *>(event)->key() == Qt::Key_Escape)
    {
        hideSearch();
        return true;
    }

    //A wheel anywhere along the bar walks the tabs sideways. Qt would send a
    //vertical scroll to the row, which has nowhere to go and would swallow it.
    if((watched == m_header || watched == m_tabArea->viewport())
       && event->type() == QEvent::Wheel && m_tabArea->isVisible())
    {
        QWheelEvent *wheel = static_cast<QWheelEvent *>(event);
        const int delta = wheel->angleDelta().y() ? wheel->angleDelta().y()
                                                  : wheel->angleDelta().x();
        QScrollBar *bar = m_tabArea->horizontalScrollBar();
        if(delta && bar->maximum() > 0)
        {
            bar->setValue(bar->value() - delta / 3);
            return true;
        }
    }
    return QWidget::eventFilter(watched, event);
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
    if(m_search->isVisible())
    {
        hideSearch();
        return;
    }
    m_search->show();
    m_tabArea->hide(); //the field wants the whole header
    m_search->setFocus();
}

void XUiPlaylistCard::hideSearch()
{
    if(!m_search->isVisible())
        return;
    m_search->hide();
    m_tabArea->show(); //the tabs come back with it gone
    m_search->clear(); //hiding it must not leave the list filtered
    m_list->setFocus(); //or the keyboard is left on a hidden field
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
