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
#include <QPixmap>
#include <QResizeEvent>
#include <QPainter>
#include <QFont>
#include <QFontMetrics>
#include <QSettings>
#include <QMenu>
#include <QUrl>
#include <QApplication>
#include <QHelpEvent>
#include <QTimer>
#include <QMimeData>
#include <QVariantAnimation>
#include <qmmpui/playlistitem.h>
#include <qmmpui/playlistmodel.h>
#include <qmmpui/qmmpuisettings.h>
#include <qmmpui/playlistmanager.h>
#include "skinnedlistwidget.h"
#include "skinnedplaylistheader.h"
#include "skinnedactionmanager.h"
#include "skin.h"
#include "skinnedpopupwidget.h"
#include "skinnedhorizontalslider.h"
#include "skinnedplaylist.h"

SkinnedListWidget::SkinnedListWidget(QWidget *parent) : QWidget(parent)
{
    m_popupWidget = nullptr;

    m_skin = Skin::instance();
    m_ui_settings = QmmpUiSettings::instance();
    m_menu = new QMenu(this);
    m_timer = new QTimer(this);
    m_timer->setInterval(50);

    m_header = new SkinnedPlayListHeader(this);
    m_hslider = new SkinnedHorizontalSlider(this);
    m_scrollAnimation = new QVariantAnimation(this);
    m_scrollAnimation->setDuration(125);

    setAcceptDrops(true);
    setMouseTracking(true);

    readSettings();
    connect(m_skin, &Skin::skinChanged, this, &SkinnedListWidget::updateSkin);
    connect(m_ui_settings, &QmmpUiSettings::repeatableTrackChanged, this, &SkinnedListWidget::updateRepeatIndicator);
    connect(m_timer, &QTimer::timeout, this, &SkinnedListWidget::autoscroll);
    connect(m_hslider, &SkinnedHorizontalSlider::sliderMoved, m_header, &SkinnedPlayListHeader::scroll);
    connect(m_hslider, &SkinnedHorizontalSlider::sliderMoved, this, qOverload<>(&SkinnedListWidget::update));
    connect(m_scrollAnimation, &QVariantAnimation::valueChanged, this, [this](const QVariant &value) {
        scroll(value.toInt());
    });
    SET_ACTION(SkinnedActionManager::PL_SHOW_HEADER, this, &SkinnedListWidget::readSettings);
}

SkinnedListWidget::~SkinnedListWidget()
{
    qDeleteAll(m_rows);
    m_rows.clear();
}

void SkinnedListWidget::readSettings()
{
    QSettings settings;
    settings.beginGroup("Skinned"_L1);
    m_show_protocol = settings.value("pl_show_protocol"_L1, false).toBool();
    m_smooth_scrolling = settings.value(u"pl_smooth_scrolling"_s, true).toBool();
    bool show_popup = settings.value("pl_show_popup"_L1, false).toBool();

    m_header->readSettings();
    m_header->setVisible(ACTION(SkinnedActionManager::PL_SHOW_HEADER)->isChecked());
    m_header->setGeometry(0, 0, width(), m_header->requiredHeight());

    if(m_update)
    {
        m_drawer.readSettings();
        updateList(PlayListModel::STRUCTURE);
        if(m_popupWidget)
        {
            m_popupWidget->deleteLater();
            m_popupWidget = nullptr;
        }
    }

    m_update = true;

    if(show_popup)
        m_popupWidget = new SkinnedPopupWidget(this);
}

int SkinnedListWidget::visibleRows() const
{
    int top = m_header->isVisibleTo(this) ?  m_header->geometry().bottom() : 0;
    int bottom = m_hslider->isVisibleTo(this) ? m_hslider->geometry().top() : height();
    int count = 0;

    for(SkinnedListWidgetRow *row : std::as_const(m_rows))
    {
        if(row->rect.top() >= top && row->rect.bottom() <= bottom)
            count++;
    }

    return count;
}

int SkinnedListWidget::firstVisibleLine() const
{
    int top = m_header->isVisibleTo(this) ?  m_header->requiredHeight() : 0;

    for(SkinnedListWidgetRow *row : std::as_const(m_rows))
    {
        if(row->rect.top() >= top)
            return row->line;
    }

    return -1;
}

int SkinnedListWidget::anchorLine() const
{
    return m_anchorLine;
}

void SkinnedListWidget::setAnchorLine(int line)
{
    m_anchorLine = line;
    updateList(PlayListModel::SELECTION);
}

QMenu *SkinnedListWidget::menu()
{
    return m_menu;
}

PlayListModel *SkinnedListWidget::model()
{
    Q_ASSERT(m_model);
    return m_model;
}

void SkinnedListWidget::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    m_drawer.fillBackground(&painter, width(), height());
    painter.setLayoutDirection(Qt::LayoutDirectionAuto);
    bool rtl = (layoutDirection() == Qt::RightToLeft);
    const int linesPerGroup = m_model->linesPerGroup();

    painter.setClipRect(5, 0, width() - 9, height());
    painter.translate(rtl ? m_header->offset() : -m_header->offset(), 0);

    for(int i = 0; i < m_rows.size(); ++i)
    {
        if(m_rows[i]->flags & SkinnedListWidgetRow::GROUP)
        {
            if(linesPerGroup == 1)
            {
                m_drawer.drawBackground(&painter, m_rows[i]);
                m_drawer.drawSeparator(&painter, m_rows[i], rtl);
            }
            else if(m_rows[i]->subIndex == 0 || (i == 0 && m_rows[i]->subIndex > 0))
            {
                m_drawer.drawBackground(&painter, m_rows[i]);
                m_drawer.drawMultiLineSeparator(&painter, m_rows[i], rtl);
            }
        }
        else
        {
            m_drawer.drawBackground(&painter, m_rows[i]);
            m_drawer.drawTrack(&painter, m_rows[i], rtl);
        }
    }
    //draw drop line
    if(m_dropLine >= 0)
    {
        m_drawer.drawDropLine(&painter, m_dropLine * m_drawer.rowHeight() - m_scroll_value +
                              (m_header->isVisible() ? m_header->height() : 0), width());
    }
}

void SkinnedListWidget::mouseDoubleClickEvent(QMouseEvent *e)
{
    int y = e->position().y();
    int lineIndex = lineAt(y);

    if(lineIndex >= 0)
    {
        PlayListItem *item = m_model->itemAtLine(lineIndex);
        if(!item)
            return;

        if(item->isGroup())
        {
            PlayListGroup *group = static_cast<PlayListGroup *>(item);
            m_model->setCurrent(group->tracks().constFirst());
        }
        else
        {
            PlayListTrack *track = static_cast<PlayListTrack *>(item);
            m_model->setCurrent(track);
        }

        emit doubleClicked();
        update();
    }
}

void SkinnedListWidget::mousePressEvent(QMouseEvent *e)
{
    if(m_popupWidget)
        m_popupWidget->hide();

    const int pressedLine = lineAt(e->position().y());

    if(pressedLine >= 0 && pressedLine < m_model->lineCount())
    {
        m_pressedLine = pressedLine;
        PlayListItem *item = m_model->itemAtLine(pressedLine);

        if(e->button() == Qt::RightButton)
        {
            if(!item->isSelected())
            {
                m_anchorLine = m_pressedLine;
                m_model->clearSelection();
                m_model->setSelected(item);
            }
            if(item->isGroup() && m_model->selectedTracks().isEmpty())
            {
                m_anchorLine = m_pressedLine;
                PlayListGroup *group = static_cast<PlayListGroup *>(item);
                m_model->setSelected(group->tracks());
            }
            QWidget::mousePressEvent(e);
            return;
        }

        if(item->isSelected() && (e->modifiers() == Qt::NoModifier))
        {
            m_select_on_release = true;
            QWidget::mousePressEvent(e);
            return;
        }

        if((Qt::ShiftModifier & e->modifiers()))
        {
            int prevAnchorLine = m_anchorLine;
            m_anchorLine = m_pressedLine;
            m_model->setSelectedLines(m_pressedLine, prevAnchorLine, true);
        }
        else //ShiftModifier released
        {
            m_anchorLine = m_pressedLine;
            if((Qt::ControlModifier & e->modifiers()))
            {
                m_model->setSelected(item, !item->isSelected());
            }
            else //ControlModifier released
            {
                m_model->clearSelection();
                m_model->setSelected(item);
            }
        }

        update();
    }
    QWidget::mousePressEvent(e);
}

void SkinnedListWidget::resizeEvent(QResizeEvent *e)
{
    m_header->setGeometry(0, 0, width(), m_header->requiredHeight());
    m_hslider->setGeometry(5, height() - 7, width() - 10, 7);
    updateList(PlayListModel::STRUCTURE);
    QWidget::resizeEvent(e);
}

void SkinnedListWidget::wheelEvent(QWheelEvent *e)
{
    if(m_hslider->underMouse() || m_model->lineCount() * m_drawer.rowHeight() <= viewportHeight())
        return;

    int delta = e->angleDelta().y() * m_drawer.rowHeight() * qApp->wheelScrollLines() / 120;
    int endValue = m_scroll_value - delta;

    if(m_smooth_scrolling)
    {
        int startValue = m_scroll_value;

        if(m_scrollAnimation->state() == QAbstractAnimation::Running)
        {
            startValue = m_scrollAnimation->currentValue().toInt();
            endValue = m_scrollAnimation->endValue().toInt() - delta;
        }

        m_scrollAnimation->stop();
        m_scrollAnimation->setStartValue(startValue);
        m_scrollAnimation->setEndValue(qBound(0, endValue, m_scroll_maximum));
        m_scrollAnimation->start();
    }
    else
    {
        scroll(qBound(0, endValue, m_scroll_maximum));
    }
}

bool SkinnedListWidget::event(QEvent *e)
{
    if(m_popupWidget)
    {
        if(e->type() == QEvent::ToolTip)
        {
            QHelpEvent *helpEvent = (QHelpEvent *) e;
            PlayListTrack *track = trackAt(helpEvent->y());
            if(!track)
            {
                m_popupWidget->deactivate();
                return QWidget::event(e);
            }
            e->accept();
            m_popupWidget->prepare(track, helpEvent->globalPos());
            return true;
        }

        if(e->type() == QEvent::Leave)
            m_popupWidget->deactivate();
    }

    return QWidget::event(e);
}

void SkinnedListWidget::updateList(int flags)
{
    m_hslider->setVisible(m_header->maxScrollValue() > 0);
    m_hslider->setPos(m_header->offset(), m_header->maxScrollValue());

    if(m_viewportHeight != viewportHeight())
    {
        m_viewportHeight = viewportHeight();
        flags |= PlayListModel::STRUCTURE;
    }

    if(flags & PlayListModel::CURRENT)
        recenterTo(m_model->currentIndex());

    QList<PlayListItem *> items;
    int count = m_model->lineCount();
    int playListHeight = count * m_drawer.rowHeight();
    int firstLine = m_scroll_value / m_drawer.rowHeight();
    int lineCount = m_viewportHeight / m_drawer.rowHeight() + 2;

    if(flags & PlayListModel::STRUCTURE || flags & PlayListModel::CURRENT)
    {
        if(m_viewportHeight >= playListHeight)
        {
            firstLine = 0;
            lineCount = count;
            setScrollPosition(0, 0);
        }
        else if(m_viewportHeight < playListHeight && m_scroll_value == 0)
        {
            setScrollPosition(0, playListHeight - m_viewportHeight);
        }
        else if(m_firstItem && m_model->itemAtLine(firstLine) != m_firstItem &&
                m_lineCount < m_model->lineCount())
        {
            //restore first visible
            firstLine = m_model->findLine(m_firstItem);
            setScrollPosition(firstLine * m_drawer.rowHeight(), playListHeight - m_viewportHeight);
        }
        else
        {
            setScrollPosition(qBound(0, m_scroll_value, playListHeight - m_viewportHeight),
                              playListHeight - m_viewportHeight);
            firstLine = m_scroll_value / m_drawer.rowHeight();

        }

        m_firstItem = m_model->isEmpty() ? nullptr : m_model->itemAtLine(firstLine);
        m_lineCount = m_model->lineCount();
        items = m_model->itemsAtLines(firstLine, lineCount);

        while(m_rows.count() < qMin(lineCount, items.count()))
            m_rows << new SkinnedListWidgetRow;
        while(m_rows.count() > qMin(lineCount, items.count()))
            delete m_rows.takeFirst();
    }
    else
    {
        items = m_model->itemsAtLines(firstLine, lineCount);
    }

    if(flags & PlayListModel::STRUCTURE)
        m_header->hideSortIndicator();

    if(flags & PlayListModel::STRUCTURE || flags & PlayListModel::METADATA)
    {
        //song numbers width
        m_drawer.calculateNumberWidth(m_model->trackCount());
        m_drawer.setSingleColumnMode(m_model->columnCount() == 1);
        m_header->setNumberWidth(m_drawer.numberWidth());
    }

    int trackStateColumn = m_header->trackStateColumn();
    bool rtl = (layoutDirection() == Qt::RightToLeft);

    const int linesPerGroup = m_model->linesPerGroup();

    for(int i = 0; i < items.count(); ++i)
    {
        SkinnedListWidgetRow *row = m_rows[i];
        row->autoResize = m_header->hasAutoResizeColumn();
        row->trackStateColumn = trackStateColumn;
        row->line = firstLine + i;
        row->subIndex = m_model->subIndexOfLine(row->line);
        row->alternateColor = m_model->alternateColor(row->line);

        if(items[i]->isSelected())
            row->flags |= SkinnedListWidgetRow::SELECTED;
        else
            row->flags &= ~SkinnedListWidgetRow::SELECTED;

        if(i == (m_anchorLine - firstLine))
            row->flags |= SkinnedListWidgetRow::ANCHOR;
                else
            row->flags &= ~SkinnedListWidgetRow::ANCHOR;

        if(flags == PlayListModel::SELECTION)
            continue;

        row->titles = items[i]->formattedTitles();
        row->length = items.at(i)->formattedDuration();
        row->sizes = m_header->sizes();
        row->alignment = m_header->alignment();

        if(items[i] == m_model->currentTrack())
            row->flags |= SkinnedListWidgetRow::CURRENT;
        else
            row->flags &= ~SkinnedListWidgetRow::CURRENT;


        if(items[i]->isGroup())
        {
            row->flags |= SkinnedListWidgetRow::GROUP;
            row->number = -1;
            row->cover = items.at(i)->cover();
            row->length.clear();
        }
        else
        {
            row->flags &= ~SkinnedListWidgetRow::GROUP;
            row->number = items.at(i)->trackIndex() + 1;
            row->extraString = getExtraString(items.at(i));
        }

        int rect_w = width() + m_header->maxScrollValue() - 10;
        int rect_h = m_drawer.rowHeight() - 1;
        int rect_x = rtl ? (width() - rect_w - 5) : 5;
        int rect_y = (m_header->isVisibleTo(this) ? m_header->height() : 0) + i * m_drawer.rowHeight();
        rect_y += firstLine * m_drawer.rowHeight() - m_scroll_value;

        if((row->flags & SkinnedListWidgetRow::GROUP) && linesPerGroup > 1)
        {
            rect_h += (linesPerGroup - 1) * m_drawer.rowHeight();
            rect_y -= row->subIndex * m_drawer.rowHeight();
        }

        row->rect = QRect(rect_x, rect_y, rect_w, rect_h);
        m_drawer.prepareRow(row);  //elide titles
    }
    update();
}

void SkinnedListWidget::scroll(int y)
{
    if(m_model->lineCount() * m_drawer.rowHeight() <= viewportHeight())
        return;

    if(!m_smooth_scrolling && y != m_scroll_maximum)
    {
        y = y - y % m_drawer.rowHeight();
        if(y == m_scroll_value)
            return;
    }

    m_scroll_value = qBound(0, y, m_scroll_maximum);
    updateList(PlayListModel::STRUCTURE);
}

void SkinnedListWidget::autoscroll()
{
    SimpleSelection sel = m_model->getSelection(m_model->trackIndexAtLine(m_pressedLine));
    if((sel.top == 0 && m_scroll_direction == TOP && sel.count() > 1) ||
            (sel.bottom == m_model->trackCount() - 1 && m_scroll_direction == DOWN && sel.count() > 1))
        return;

    if(m_scroll_direction == DOWN)
    {
        int line = lastVisibleLine() + 1;
        int y = (line + 1) * m_drawer.rowHeight() - viewportHeight();
        m_scroll_value = qBound(0, y, m_scroll_maximum);
        m_model->moveTracks(m_model->trackIndexAtLine(m_pressedLine), m_model->trackIndexAtLine(line));
        m_pressedLine = line;
    }
    else if(m_scroll_direction == TOP && firstVisibleLine() > 0)
    {
        int line = firstVisibleLine() - 1;
        int y = line * m_drawer.rowHeight();
        m_scroll_value = qBound(0, y, m_scroll_maximum);
        m_model->moveTracks(m_model->trackIndexAtLine(m_pressedLine), m_model->trackIndexAtLine(line));
        m_pressedLine = line;
    }

    updateList(PlayListModel::STRUCTURE);
}

void SkinnedListWidget::updateRepeatIndicator()
{
    updateList(PlayListModel::CURRENT | PlayListModel::STRUCTURE);
}

void SkinnedListWidget::scrollTo(int index)
{
    if(viewportHeight() > 0)
    {
        recenterTo(index);
        updateList(PlayListModel::STRUCTURE);
    }
}

void SkinnedListWidget::setModel(PlayListModel *selected, PlayListModel *previous)
{
    if(previous)
    {
        previous->setProperty("scrollbar_maximum", m_scroll_maximum);
        previous->setProperty("scrollbar_value", m_scroll_value);
        disconnect(previous, nullptr, this, nullptr); //disconnect previous model
        disconnect(previous, nullptr, m_header, nullptr);
    }
    qApp->processEvents();
    m_model = selected;
    m_lineCount = m_model->lineCount();
    m_firstItem = nullptr;

    if(m_model->property("scrollbar_value").isValid())
    {
        m_scroll_maximum = m_model->property("scrollbar_maximum").toInt();
        m_scroll_value = m_model->property("scrollbar_value").toInt();
        updateList(PlayListModel::STRUCTURE);
    }
    else
    {
        m_scroll_value = 0;
        updateList(PlayListModel::STRUCTURE | PlayListModel::CURRENT);
    }
    connect(m_model, &PlayListModel::scrollToRequest, this, &SkinnedListWidget::scrollTo);
    connect(m_model, &PlayListModel::listChanged, this, &SkinnedListWidget::updateList);
    connect(m_model, &PlayListModel::sortingByColumnFinished, m_header, &SkinnedPlayListHeader::showSortIndicator);
}

void SkinnedListWidget::setViewPosition(int sc, bool bottom)
{
    if(m_model->lineCount() * m_drawer.rowHeight() <= viewportHeight())
        return;

    int y = sc * m_drawer.rowHeight();

    if(bottom)
        y = y + m_drawer.rowHeight() - viewportHeight();

    m_scroll_value = qBound(0, y, m_scroll_maximum);
    updateList(PlayListModel::STRUCTURE);
}

void SkinnedListWidget::updateSkin()
{
    m_drawer.readSettings();
    m_header->readSettings();
    update();
}

void SkinnedListWidget::dragEnterEvent(QDragEnterEvent *event)
{
    if(event->mimeData()->hasFormat(u"text/uri-list"_s) || event->mimeData()->hasFormat(u"application/json"_s))
        event->acceptProposedAction();
}

void SkinnedListWidget::dropEvent(QDropEvent *event)
{
    if(event->mimeData()->hasUrls() || event->mimeData()->hasFormat(u"application/json"_s))
    {
        event->acceptProposedAction();
        QApplication::restoreOverrideCursor();

        int index = lineAt(event->position().y());
        if(index < 0)
            index = qMin(lastVisibleLine() + 1, m_model->lineCount());

        if(event->mimeData()->hasUrls())
        {
            QList<QUrl> list_urls = event->mimeData()->urls();
            m_model->insertUrls(index, list_urls);
        }
        else if(event->mimeData()->hasFormat(u"application/json"_s))
        {
            QByteArray json = event->mimeData()->data(u"application/json"_s);
            m_model->insertJson(index, json);
        }
    }
    m_dropLine = -1;
}

void SkinnedListWidget::dragLeaveEvent(QDragLeaveEvent *)
{
    m_dropLine = -1;
    update();
}

void SkinnedListWidget::dragMoveEvent(QDragMoveEvent *event)
{
    int index = lineAt(event->position().y());
    if(index < 0)
        index = qMin(visibleRows(), m_model->lineCount());
    if(index != m_dropLine)
    {
        m_dropLine = index;
        update();
    }
    if(event->mimeData()->hasFormat(u"text/uri-list"_s))
        event->acceptProposedAction();
}

const QString SkinnedListWidget::getExtraString(PlayListItem *item)
{
    if(item->isGroup())
        return QString();

    QString extra_string;
    PlayListTrack *track = static_cast<PlayListTrack *>(item);

    if(m_show_protocol && track->path().contains(u"://"_s))
        extra_string = QLatin1Char('[') + track->path().split(u"://"_s).constFirst() + QLatin1Char(']');

    if(track->isQueued())
        extra_string += QLatin1Char('|') + QString::number(track->queuedIndex() + 1) + QLatin1Char('|');

    if(m_model->currentTrack() == track && m_ui_settings->isRepeatableTrack())
        extra_string += u"|R|"_s;
    else if(m_model->isStopAfter(track))
        extra_string += u"|S|"_s;

    return extra_string.trimmed(); //remove white space
}

int SkinnedListWidget::viewportHeight() const
{
    int h = height();
    if(m_header->isVisibleTo(this))
        h -= m_header->requiredHeight();
    if(m_hslider->isVisibleTo(this))
        h -= m_hslider->height();
    return qMax(0, h);
}

int SkinnedListWidget::lastVisibleLine() const
{
    int bottom = m_hslider->isVisibleTo(this) ? m_hslider->geometry().top() : height();

    for(int i = m_rows.count() - 1; i >= 0; i--)
    {
        if(m_rows[i]->rect.bottom() <= bottom)
            return m_rows[i]->line;
    }

    return -1;
}

void SkinnedListWidget::setScrollPosition(int value, int maximum)
{
    m_scroll_value = value;
    m_scroll_maximum = maximum;
    emit scrollPositionChanged(value, maximum);
}

void SkinnedListWidget::mouseMoveEvent(QMouseEvent *e)
{    
    if(e->buttons() == Qt::LeftButton)
    {
        if(m_prev_y > e->position().y())
            m_scroll_direction = TOP;
        else if(m_prev_y < e->position().y())
            m_scroll_direction = DOWN;
        else
            m_scroll_direction = NONE;

        if(e->position().y() < 0 || e->position().y() > height())
        {
            if(!m_timer->isActive())
                m_timer->start();
            return;
        }
        m_timer->stop();

        int index = lineAt(e->position().y());

        if(index >= 0)
        {
            m_anchorLine = index;
            SimpleSelection sel = m_model->getSelection(m_model->trackIndexAtLine(m_pressedLine));
            if(sel.count() > 1 && m_scroll_direction == TOP)
            {
                if(sel.top == 0 || sel.top == m_model->trackIndexAtLine(firstVisibleLine()))
                    return;
            }
            else if(sel.count() > 1 && m_scroll_direction == DOWN)
            {
                if(sel.bottom == m_model->trackIndexAtLine(m_model->lineCount() - 1) ||
                        sel.bottom == m_model->trackIndexAtLine(lastVisibleLine()))
                    return;
            }
            m_model->moveTracks(m_model->trackIndexAtLine(m_pressedLine),
                               m_model->trackIndexAtLine(index));

            m_prev_y = e->position().y();
            m_pressedLine = index;
        }
    }
    else if(m_popupWidget)
    {
        PlayListTrack *track = trackAt(e->position().y());
        if(!track || m_popupWidget->url() != track->path())
            m_popupWidget->deactivate();
    }
}

void SkinnedListWidget::mouseReleaseEvent(QMouseEvent *e)
{
    if(m_select_on_release && m_pressedLine >= 0)
    {
        m_model->clearSelection();
        PlayListItem *item = m_model->itemAtLine(m_pressedLine);
        m_model->setSelected(item, true);
        m_anchorLine = m_pressedLine;
        m_select_on_release = false;
    }
    m_pressedLine = -1;
    m_scroll_direction = NONE;
    m_timer->stop();
    QWidget::mouseReleaseEvent(e);
}

int SkinnedListWidget::lineAt(int y) const
{
    for(SkinnedListWidgetRow *row : std::as_const(m_rows))
    {
        int lines = row->rect.height() / m_drawer.rowHeight() + 1;

        if(row->rect.top() <= y && y <= row->rect.top() + lines * m_drawer.rowHeight())
            return row->line;
    }

    return -1;
}

PlayListTrack *SkinnedListWidget::trackAt(int y) const
{
    int line = lineAt(y);
    return line >= 0 ? m_model->trackAtLine(line) : nullptr;
}

void SkinnedListWidget::contextMenuEvent(QContextMenuEvent * event)
{
    if(menu())
        menu()->exec(event->globalPos());
}

void SkinnedListWidget::recenterTo(int index)
{
    if(viewportHeight() > 0 && index >= 0)
    {
        int line = m_model->findLine(index);
        if(line < 0)
            return;

        if((line + 1) * m_drawer.rowHeight() > m_scroll_value + viewportHeight())
        {
             m_scroll_value = qMin(m_scroll_maximum, line * m_drawer.rowHeight() - viewportHeight() / 2);
        }
        else if(line * m_drawer.rowHeight() < m_scroll_value)
        {
            m_scroll_value = qMax(line * m_drawer.rowHeight() - viewportHeight() / 2, 0);
        }
    }
}
