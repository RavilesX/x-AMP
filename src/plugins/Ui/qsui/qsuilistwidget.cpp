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
#include <QScrollBar>
#include <QMimeData>
#include <QVariantAnimation>
#include <qmmpui/playlistitem.h>
#include <qmmpui/playlistmodel.h>
#include <qmmpui/qmmpuisettings.h>
#include <qmmpui/mediaplayer.h>
#include "qsuilistwidget.h"
#include "qsuiplaylistheader.h"
#include "qsuiactionmanager.h"
#include "qsuipopupwidget.h"

QSUiListWidget::QSUiListWidget(PlayListModel *model, QWidget *parent) : QWidget(parent),
    m_model(model)
{
    setFocusPolicy(Qt::StrongFocus);
    m_ui_settings = QmmpUiSettings::instance();
    m_timer = new QTimer(this);
    m_timer->setInterval(50);
    m_header = new QSUiPlayListHeader(this);
    m_scrollBar = new QScrollBar(Qt::Vertical, this);
    m_hslider = new QScrollBar(Qt::Horizontal, this);
    m_hslider->setPageStep(50);
    m_scrollAnimation = new QVariantAnimation(this);
    m_scrollAnimation->setDuration(125);

    setAcceptDrops(true);
    setMouseTracking(true);

    readSettings();
    connect(m_ui_settings, &QmmpUiSettings::repeatableTrackChanged, this, &QSUiListWidget::updateRepeatIndicator);
    connect(m_timer, &QTimer::timeout, this, &QSUiListWidget::autoscroll);
    connect(m_scrollBar, &QScrollBar::valueChanged, this, &QSUiListWidget::scroll);
    connect(m_hslider, &QScrollBar::valueChanged, m_header, &QSUiPlayListHeader::scroll);
    connect(m_hslider, &QScrollBar::valueChanged, this, qOverload<>(&QSUiListWidget::update));
    connect(m_model, &PlayListModel::scrollToRequest, this, &QSUiListWidget::scrollTo);
    connect(m_model, &PlayListModel::listChanged, this, &QSUiListWidget::updateList);
    connect(m_model, &PlayListModel::sortingByColumnFinished, m_header, &QSUiPlayListHeader::showSortIndicator);
    connect(m_scrollAnimation, &QVariantAnimation::valueChanged, this, [this](const QVariant &value) {
        scroll(value.toInt());
    });
    SET_ACTION(QSUiActionManager::PL_SHOW_HEADER, this, &QSUiListWidget::readSettings);
}

QSUiListWidget::~QSUiListWidget()
{
    qDeleteAll(m_rows);
    m_rows.clear();
}

void QSUiListWidget::readSettings()
{
    QSettings settings;
    settings.beginGroup(u"Simple"_s);
    m_show_protocol = settings.value(u"pl_show_protocol"_s, false).toBool();
    m_smooth_scrolling = settings.value(u"pl_smooth_scrolling"_s, true).toBool();
    bool show_popup = settings.value(u"pl_show_popup"_s, false).toBool();

    m_header->readSettings();
    m_header->setVisible(ACTION(QSUiActionManager::PL_SHOW_HEADER)->isChecked());
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
    m_scrollBar->setSingleStep(m_smooth_scrolling ? m_drawer.rowHeight() : 1);

    m_update = true;

    if(show_popup)
        m_popupWidget = new QSUiPopupWidget(this);
}

int QSUiListWidget::visibleRows() const
{
    int top = m_header->isVisibleTo(this) ?  m_header->geometry().bottom() : 0;
    int bottom = m_hslider->isVisibleTo(this) ? m_hslider->geometry().top() : height();
    int count = 0;

    for(QSUiListWidgetRow *row : std::as_const(m_rows))
    {
        if(row->rect.top() >= top && row->rect.bottom() <= bottom)
            count++;
    }

    return count;
}

int QSUiListWidget::firstVisibleLine() const
{
    int top = m_header->isVisibleTo(this) ?  m_header->requiredHeight() : 0;

    for(QSUiListWidgetRow *row : std::as_const(m_rows))
    {
        if(row->rect.top() >= top)
            return row->line;
    }

    return -1;
}

int QSUiListWidget::anchorLine() const
{
    return m_anchorLine;
}

void QSUiListWidget::setAnchorLine(int index)
{
    m_anchorLine = index;
    updateList(PlayListModel::SELECTION);
}

QMenu *QSUiListWidget::menu()
{
    return m_menu;
}

void QSUiListWidget::setMenu(QMenu *menu)
{
    m_menu = menu;
}

PlayListModel *QSUiListWidget::model()
{
    Q_ASSERT(m_model);
    return m_model;
}

bool QSUiListWidget::filterMode() const
{
    return m_filterMode;
}

void QSUiListWidget::setModel(PlayListModel *selected, PlayListModel *previous)
{
    if(m_filterMode)
    {
        m_filterMode = false;
        m_filteredItems.clear();
    }

    if(previous)
    {
        previous->setProperty("scrollbar_maximum", m_scrollBar->maximum());
        previous->setProperty("scrollbar_value", m_scrollBar->value());
        disconnect(previous, nullptr, this, nullptr); //disconnect previous model
        disconnect(previous, nullptr, m_header, nullptr);
    }
    qApp->processEvents();
    m_model = selected;
    m_lineCount = m_model->lineCount();
    m_firstItem = nullptr;

    if(m_model->property("scrollbar_value").isValid())
    {
        m_scrollBar->setMaximum(m_model->property("scrollbar_maximum").toInt());
        m_scrollBar->setValue(m_model->property("scrollbar_value").toInt());
        updateList(PlayListModel::STRUCTURE);
    }
    else
    {
        m_scrollBar->setValue(0);
        updateList(PlayListModel::STRUCTURE | PlayListModel::CURRENT);
    }
    connect(m_model, &PlayListModel::scrollToRequest, this, &QSUiListWidget::scrollTo);
    connect(m_model, &PlayListModel::listChanged, this, &QSUiListWidget::updateList);
    connect(m_model, &PlayListModel::sortingByColumnFinished, m_header, &QSUiPlayListHeader::showSortIndicator);
}

void QSUiListWidget::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    m_drawer.fillBackground(&painter, width(), height());
    painter.setLayoutDirection(Qt::LayoutDirectionAuto);
    const bool rtl = (layoutDirection() == Qt::RightToLeft);
    const int scroll_bar_width = m_scrollBar->isVisibleTo(this) ? m_scrollBar->sizeHint().width() : 0;
    const int linesPerGroup = m_model->linesPerGroup();

    painter.setClipRect(5, 0, width() - scroll_bar_width - 9, height());
    painter.translate(rtl ? m_header->offset() : -m_header->offset(), 0);

    for(int i = 0; i < m_rows.size(); ++i)
    {
        if(m_rows[i]->flags & QSUiListWidgetRow::GROUP)
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
        m_drawer.drawDropLine(&painter, m_dropLine * m_drawer.rowHeight() - m_scrollBar->value() +
                              (m_header->isVisible() ? m_header->height() : 0), width());
    }
}

void QSUiListWidget::mouseDoubleClickEvent(QMouseEvent *e)
{
    int y = e->position().y();
    int lineIndex = lineAt(y);

    if(lineIndex >= 0)
    {
        if(m_filterMode)
        {
            m_filterMode = false;
            m_filteredItems.clear();
            scrollTo(lineIndex);
        }

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

        MediaPlayer *player = MediaPlayer::instance();
        player->playListManager()->selectPlayList(m_model);
        player->playListManager()->activatePlayList(m_model);
        player->stop();
        player->play();
        emit doubleClicked();
        update();
    }
}

void QSUiListWidget::mousePressEvent(QMouseEvent *e)
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

void QSUiListWidget::resizeEvent(QResizeEvent *e)
{
    m_header->setGeometry(0, 0, width(), m_header->requiredHeight());
    if(e->oldSize().height() < 10)
        updateList(PlayListModel::STRUCTURE | PlayListModel::CURRENT); //recenter to current on first resize
    else
        updateList(PlayListModel::STRUCTURE);
    QWidget::resizeEvent(e);
}

void QSUiListWidget::wheelEvent(QWheelEvent *e)
{
    if(m_hslider->underMouse() || m_model->lineCount() * m_drawer.rowHeight() <= viewportHeight())
        return;

    int delta = e->angleDelta().y() * m_drawer.rowHeight() * qApp->wheelScrollLines() / 120;
    int endValue = m_scrollBar->value() - delta;

    if(m_smooth_scrolling)
    {
        int startValue = m_scrollBar->value();

        if(m_scrollAnimation->state() == QAbstractAnimation::Running)
        {
            startValue = m_scrollAnimation->currentValue().toInt();
            endValue = m_scrollAnimation->endValue().toInt() - delta;
        }

        m_scrollAnimation->stop();
        m_scrollAnimation->setStartValue(startValue);
        m_scrollAnimation->setEndValue(qBound(0, endValue, m_scrollBar->maximum()));
        m_scrollAnimation->start();
    }
    else
    {
        scroll(qBound(0, endValue, m_scrollBar->maximum()));
    }
}

void QSUiListWidget::showEvent(QShowEvent *)
{
    if(!m_rows.isEmpty())
        updateList(PlayListModel::METADATA);
}

bool QSUiListWidget::event(QEvent *e)
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
    if(e->type() == QEvent::StyleChange || e->type() == QEvent::PaletteChange)
        readSettings();

    return QWidget::event(e);
}

void QSUiListWidget::updateList(int flags)
{
    m_hslider->setRange(0, m_header->maxScrollValue());
    m_hslider->setValue(m_header->offset());
    m_hslider->setVisible(m_header->maxScrollValue() > 0);

    if(m_viewportHeight != viewportHeight())
    {
        m_viewportHeight = viewportHeight();
        flags |= PlayListModel::STRUCTURE;
    }

    if(flags & PlayListModel::STRUCTURE && m_filterMode)
    {
        m_filteredItems = m_model->findTracks(m_filterString);
    }

    if(flags & PlayListModel::CURRENT)
        recenterTo(m_model->currentIndex());

    QList<PlayListItem *> items;
    int count = m_filterMode ? m_filteredItems.count() : m_model->lineCount();
    int playListHeight = count * m_drawer.rowHeight();
    int firstLine = m_scrollBar->value() / m_drawer.rowHeight();
    int lineCount = m_viewportHeight / m_drawer.rowHeight() + 2;

    if(flags & PlayListModel::STRUCTURE || flags & PlayListModel::CURRENT)
    {
        m_scrollBar->blockSignals(true);
        if(m_viewportHeight >= playListHeight)
        {
            firstLine = 0;
            lineCount = count;
            m_scrollBar->setMaximum(0);
            m_scrollBar->setValue(0);
        }
        else if(m_viewportHeight < playListHeight && m_scrollBar->value() == 0)
        {
            m_scrollBar->setMaximum(playListHeight - m_viewportHeight);
        }
        else if(!m_filterMode && m_firstItem && m_model->itemAtLine(firstLine) != m_firstItem &&
                m_lineCount < m_model->lineCount())
        {
            //restore first visible
            firstLine = m_model->findLine(m_firstItem);
            m_scrollBar->setMaximum(playListHeight - m_viewportHeight);
            m_scrollBar->setValue(firstLine * m_drawer.rowHeight());
        }
        else
        {
            m_scrollBar->setMaximum(playListHeight - m_viewportHeight);
            m_scrollBar->setValue(qBound(0, m_scrollBar->value(), m_scrollBar->maximum()));
            firstLine = m_scrollBar->value() / m_drawer.rowHeight();
        }

        m_scrollBar->blockSignals(false);

        if(m_filterMode)
        {
            items = m_filteredItems.mid(firstLine, lineCount);
        }
        else
        {
            m_firstItem = m_model->isEmpty() ? nullptr : m_model->itemAtLine(firstLine);
            m_lineCount = m_model->lineCount();
            items = m_model->itemsAtLines(firstLine, lineCount);
        }

        while(m_rows.count() < qMin(lineCount, items.count()))
            m_rows << new QSUiListWidgetRow;
        while(m_rows.count() > qMin(lineCount, items.count()))
            delete m_rows.takeFirst();

        m_scrollBar->setVisible(playListHeight > m_viewportHeight);
    }
    else
    {
        items = m_filterMode ? m_filteredItems.mid(firstLine, lineCount) : m_model->itemsAtLines(firstLine, lineCount);
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

    int scroll_bar_width = m_scrollBar->isVisibleTo(this) ? m_scrollBar->sizeHint().width() : 0;
    int trackStateColumn = m_header->trackStateColumn();
    bool rtl = layoutDirection() == Qt::RightToLeft;
    m_header->setScrollBarWidth(scroll_bar_width);

    updateScrollBars();

    const int linesPerGroup = m_model->linesPerGroup();

    for(int i = 0; i < items.count(); ++i)
    {
        QSUiListWidgetRow *row = m_rows[i];
        row->autoResize = m_header->hasAutoResizeColumn();
        row->trackStateColumn = trackStateColumn;
        row->line = firstLine + i;
        row->subIndex = m_model->subIndexOfLine(row->line);
        row->alternateColor = m_model->alternateColor(row->line);


        if(items[i]->isSelected())
            row->flags |= QSUiListWidgetRow::SELECTED;
        else
            row->flags &= ~QSUiListWidgetRow::SELECTED;

        if(i == (m_anchorLine - firstLine))
            row->flags |= QSUiListWidgetRow::ANCHOR;
        else
            row->flags &= ~QSUiListWidgetRow::ANCHOR;

        if(flags == PlayListModel::SELECTION)
            continue;

        row->titles = items[i]->formattedTitles();
        row->length = items.at(i)->formattedDuration();
        row->sizes = m_header->sizes();
        row->alignment = m_header->alignment();

        if(items[i] == m_model->currentTrack())
            row->flags |= QSUiListWidgetRow::CURRENT;
        else
            row->flags &= ~QSUiListWidgetRow::CURRENT;

        if(items[i]->isGroup())
        {
            row->flags |= QSUiListWidgetRow::GROUP;
            row->number = -1;
            row->cover = items.at(i)->cover();
            row->length.clear();
        }
        else
        {
            row->flags &= ~QSUiListWidgetRow::GROUP;
            row->number = items.at(i)->trackIndex() + 1;
            row->extraString = getExtraString(items.at(i));
        }

        int rect_w = width() + m_header->maxScrollValue() - 10 - scroll_bar_width;
        int rect_h = m_drawer.rowHeight() - 1;
        int rect_x = rtl ? (width() - rect_w - 5) : 5;
        int rect_y = (m_header->isVisibleTo(this) ? m_header->height() : 0) + i * m_drawer.rowHeight();
        rect_y += firstLine * m_drawer.rowHeight() - m_scrollBar->value();

        if((row->flags & QSUiListWidgetRow::GROUP) && linesPerGroup > 1)
        {
            rect_h += (linesPerGroup - 1) * m_drawer.rowHeight();
            rect_y -= row->subIndex * m_drawer.rowHeight();
        }

        row->rect = QRect(rect_x, rect_y, rect_w, rect_h);
        m_drawer.prepareRow(row);  //elide titles
    }
    update();
}

void QSUiListWidget::autoscroll()
{
    if(m_filterMode)
        return;

    SimpleSelection sel = m_model->getSelection(m_model->trackIndexAtLine(m_pressedLine));
    if((sel.top == 0 && m_scroll_direction == TOP && sel.count() > 1) ||
            (sel.bottom == m_model->trackCount() - 1 && m_scroll_direction == DOWN && sel.count() > 1))
        return;

    if(m_scroll_direction == DOWN)
    {
        int line = lastVisibleLine() + 1;
        int y = (line + 1) * m_drawer.rowHeight() - viewportHeight();
        m_scrollBar->blockSignals(true);
        m_scrollBar->setValue(qBound(0, y, m_scrollBar->maximum()));
        m_scrollBar->blockSignals(false);
        m_model->moveTracks(m_model->trackIndexAtLine(m_pressedLine), m_model->trackIndexAtLine(line));
        m_pressedLine = line;
    }
    else if(m_scroll_direction == TOP && firstVisibleLine() > 0)
    {
        int line = firstVisibleLine() - 1;
        int y = line * m_drawer.rowHeight();
        m_scrollBar->blockSignals(true);
        m_scrollBar->setValue(qBound(0, y, m_scrollBar->maximum()));
        m_scrollBar->blockSignals(false);
        m_model->moveTracks(m_model->trackIndexAtLine(m_pressedLine), m_model->trackIndexAtLine(line));
        m_pressedLine = line;
    }

    updateList(PlayListModel::STRUCTURE);
}

void QSUiListWidget::updateRepeatIndicator()
{
    updateList(PlayListModel::CURRENT | PlayListModel::STRUCTURE);
}

void QSUiListWidget::scrollTo(int index)
{
    if(viewportHeight() > 0 && !m_filterMode)
    {
        recenterTo(index);
        updateList(PlayListModel::STRUCTURE);
    }
}

void QSUiListWidget::setViewPosition(int sc, bool bottom)
{
    if(m_model->lineCount() * m_drawer.rowHeight() <= viewportHeight())
        return;

    int y = sc * m_drawer.rowHeight();

    if(bottom)
        y = y + m_drawer.rowHeight() - viewportHeight();

    m_scrollBar->blockSignals(true);
    m_scrollBar->setValue(qBound(0, y, m_scrollBar->maximum()));
    m_scrollBar->blockSignals(false);
    updateList(PlayListModel::STRUCTURE);
}

void QSUiListWidget::scroll(int y)
{
    if(m_model->lineCount() * m_drawer.rowHeight() <= viewportHeight())
        return;

    if(!m_smooth_scrolling && y != m_scrollBar->maximum())
    {
        y = y - y % m_drawer.rowHeight();
    }

    m_scrollBar->setValue(qBound(0, y, m_scrollBar->maximum()));
    updateList(PlayListModel::STRUCTURE);
}

void QSUiListWidget::setFilterString(const QString &str)
{
    m_filterString = str;
    if(str.isEmpty())
    {
        m_filteredItems.clear();
        m_filterString.clear();
        m_filterMode = false;
    }
    else
    {
        m_filterMode = true;
    }
    updateList(PlayListModel::STRUCTURE);
}

void QSUiListWidget::clear()
{
    if(m_filterMode)
    {
        m_model->removeTracks(m_filteredItems);
        m_filteredItems.clear();
    }
    else
    {
        m_model->clear();
    }
}

void QSUiListWidget::removeSelected()
{
    if(m_filterMode)
    {
        QList<PlayListItem *> items;
        for(PlayListItem *item : std::as_const(m_filteredItems))
        {
            if(item->isSelected())
                items << item;
        }
        m_model->removeTracks(items);
    }
    else
    {
        m_model->removeSelected();
    }
}

void QSUiListWidget::removeUnselected()
{
    if(m_filterMode)
    {
        QList<PlayListItem *> items;
        for(PlayListItem *item : m_filteredItems)
        {
            if(!item->isSelected())
                items << item;
        }
        m_model->removeTracks(items);
    }
    else
    {
        m_model->removeUnselected();
    }
}

void QSUiListWidget::dragEnterEvent(QDragEnterEvent *event)
{
    if(event->mimeData()->hasFormat(u"text/uri-list"_s) || event->mimeData()->hasFormat(u"application/json"_s))
        event->acceptProposedAction();
}

void QSUiListWidget::dropEvent(QDropEvent *event)
{
    if(!m_filterMode && (event->mimeData()->hasUrls() || event->mimeData()->hasFormat(u"application/json"_s)))
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

void QSUiListWidget::dragLeaveEvent(QDragLeaveEvent *)
{
    m_dropLine = -1;
    update();
}

void QSUiListWidget::dragMoveEvent(QDragMoveEvent *event)
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

QString QSUiListWidget::getExtraString(PlayListItem *item) const
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

void QSUiListWidget::updateScrollBars()
{
    bool rtl = layoutDirection() == Qt::RightToLeft;

    int vslider_width = m_scrollBar->isVisibleTo(this) ? m_scrollBar->sizeHint().width() : 0;
    int hslider_height = m_hslider->isVisibleTo(this) ? m_hslider->sizeHint().height() : 0;

    if(rtl)
    {
        m_scrollBar->setGeometry(0, 0, m_scrollBar->sizeHint().width(), height() - hslider_height);
        m_hslider->setGeometry(vslider_width, height() - m_hslider->sizeHint().height(),
                               width() - vslider_width, m_hslider->sizeHint().height());
    }
    else
    {
        m_scrollBar->setGeometry(width() - m_scrollBar->sizeHint().width(), 0,
                                 m_scrollBar->sizeHint().width(), height() - hslider_height);
        m_hslider->setGeometry(0, height() - m_hslider->sizeHint().height(), width() - vslider_width,
                               m_hslider->sizeHint().height());
    }
}

int QSUiListWidget::viewportHeight() const
{
    int h = height();
    if(m_header->isVisibleTo(this))
        h -= m_header->requiredHeight();
    if(m_hslider->isVisibleTo(this))
        h -= m_hslider->height();

    return qMax(h, 0);
}

int QSUiListWidget::lastVisibleLine() const
{
    int bottom = m_hslider->isVisibleTo(this) ? m_hslider->geometry().top() : height();

    for(int i = m_rows.count() - 1; i >= 0; i--)
    {
        if(m_rows[i]->rect.bottom() <= bottom)
            return m_rows[i]->line;
    }

    return -1;
}

void QSUiListWidget::mouseMoveEvent(QMouseEvent *e)
{
    if(m_filterMode)
        return;

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

void QSUiListWidget::mouseReleaseEvent(QMouseEvent *e)
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

int QSUiListWidget::lineAt(int y) const
{
    for(QSUiListWidgetRow *row : std::as_const(m_rows))
    {
        int lines = row->rect.height() / m_drawer.rowHeight() + 1;

        if(row->rect.top() <= y && y <= row->rect.top() + lines * m_drawer.rowHeight())
        {
            if(!m_filterMode)
                return row->line;

            if(row->line >= 0 && row->line < m_filteredItems.count())
            {
                const PlayListItem *item = m_filteredItems.at(row->line);
                return m_model->findLine(item);
            }
        }

    }

    return -1;
}

PlayListTrack *QSUiListWidget::trackAt(int y) const
{
    int line = lineAt(y);
    return line >= 0 ? m_model->trackAtLine(line) : nullptr;
}

void QSUiListWidget::contextMenuEvent(QContextMenuEvent * event)
{
    if(menu())
        menu()->exec(event->globalPos());
}

void QSUiListWidget::recenterTo(int index)
{
    if(viewportHeight() > 0 && index >= 0 && !m_filterMode)
    {
        int line = m_model->findLine(index);
        if(line < 0)
            return;

        if((line + 1) * m_drawer.rowHeight() > m_scrollBar->value() + viewportHeight())
        {
            m_scrollBar->setValue(qMin(m_scrollBar->maximum(), line * m_drawer.rowHeight() - viewportHeight() / 2));
        }
        else if(line * m_drawer.rowHeight() < m_scrollBar->value())
        {
            m_scrollBar->setValue(qMax(line * m_drawer.rowHeight() - viewportHeight() / 2, 0));
        }
    }
}
