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
#include <QTimer>
#include <QSettings>
#include <QAction>
#include <QMenu>
#include <QMouseEvent>
#include <QApplication>
#include <qmmp/qmmp.h>
#include <qmmp/soundcore.h>
#include <qmmpui/metadataformatter.h>
#include <qmmpui/playlistmanager.h>
#include <qmmpui/playlistmodel.h>
#include <qmmpui/mediaplayer.h>
#include "skin.h"
#include "skinnedactionmanager.h"
#include "skinnedtextscroller.h"


#define SCROLL_SEP u"   *** "_s
#define TITLE_FORMAT u"%p%if(%p&%t, - ,)%t%if(%p,,%if(%t,,%f))%if(%l, - %l,)"_s

SkinnedTextScroller::SkinnedTextScroller (QWidget *parent) : QWidget (parent),
    m_defautText(QStringLiteral("x-AMP %1").arg(Qmmp::strVersion()))
{
    m_formater.setPattern(TITLE_FORMAT);
    m_core = SoundCore::instance();
    m_skin = Skin::instance();
    m_ratio = m_skin->ratio();

    m_timer = new QTimer (this);
    m_timer->setInterval(50);
    m_timer->start();

    m_menu = new QMenu(this);
    m_scrollAction = m_menu->addAction(tr("Autoscroll Songname"), this, &SkinnedTextScroller::updateText);
    m_transparencyAction = m_menu->addAction(tr("Transparent Background"), this, &SkinnedTextScroller::updateText);
    m_scrollAction->setCheckable(true);
    m_transparencyAction->setCheckable(true);
    connect(m_timer, &QTimer::timeout, this, &SkinnedTextScroller::addOffset);
    connect(m_skin, &Skin::skinChanged, this, &SkinnedTextScroller::updateSkin);
    connect(m_core, &SoundCore::stateChanged, this, &SkinnedTextScroller::processState);
    connect(m_core, &SoundCore::trackInfoChanged, this, &SkinnedTextScroller::processMetaData);
    connect(MediaPlayer::instance(), &MediaPlayer::playbackFinished, this, &SkinnedTextScroller::clearText);
    updateSkin();
}

SkinnedTextScroller::~SkinnedTextScroller()
{
    QSettings settings;
    settings.setValue("Skinned/autoscroll"_L1, m_scrollAction->isChecked());
    settings.setValue("Skinned/scroller_transparency"_L1, m_transparencyAction->isChecked());
    delete m_metrics;
}

void SkinnedTextScroller::setText(const QString &text)
{
    m_sliderText = text;
    updateText();
}

void SkinnedTextScroller::clear()
{
    setText(QString());
}

void SkinnedTextScroller::addOffset()
{
    if(!m_scroll)
    {
        m_timer->stop();
        return;
    }
    m_x1--;
    m_x2--;
    if(m_x1 < - m_pixmap.width())
        m_x1 = m_pixmap.width();
    if(m_x2 < - m_pixmap.width())
        m_x2 = m_pixmap.width();
    update();
}

void SkinnedTextScroller::updateSkin()
{
    setCursor(m_skin->getCursor(Skin::CUR_SONGNAME));
    m_color = m_skin->getMainColor(Skin::MW_FOREGROUND);
    QSettings settings;
    m_bitmap = settings.value("Skinned/bitmap_font"_L1, false).toBool();
    m_ratio = m_skin->ratio();
    QString fontname = settings.value("Skinned/mw_font"_L1, QApplication::font().toString()).toString();
    m_font.fromString(fontname);
    if(!m_metrics)
    {
        m_scrollAction->setChecked(settings.value("Skinned/autoscroll"_L1, true).toBool());
        m_transparencyAction->setChecked(settings.value("Skinned/scroller_transparency"_L1, true).toBool());
    }
    delete m_metrics;
    m_metrics = new QFontMetrics(m_font);
    updateText();
}

void SkinnedTextScroller::setProgress(int progress)
{
    m_bufferText = tr("Buffering: %1%").arg(progress);
    updateText();
}

void SkinnedTextScroller::hideEvent(QHideEvent *)
{
    m_timer->stop();
}

void SkinnedTextScroller::showEvent(QShowEvent *)
{
    if (m_scroll)
        m_timer->start();
}

inline void drawBitmapText(int x, int y, const QString &text, QPainter *paint, Skin *skin)
{
    QString lowertext = text.toLower();
    int chwidth, ypix;
    {
        QPixmap samplechar = skin->getLetter(QLatin1Char('a'));
        chwidth = samplechar.width();
        ypix = y - samplechar.height();
    }
    for (int i = 0; i < lowertext.size(); i++)
    {
        QPixmap pixchar = skin->getLetter(lowertext[i]);
        paint->drawPixmap(x, ypix, pixchar);
        x += chwidth;
    }
}

void SkinnedTextScroller::paintEvent (QPaintEvent *)
{
    QPainter paint(this);
    if(m_scroll)
    {
        paint.drawPixmap(m_x1,0, m_pixmap);
        paint.drawPixmap(m_x2,0, m_pixmap);
    }
    else
        paint.drawPixmap(0,0, m_pixmap);
}

void SkinnedTextScroller::mousePressEvent(QMouseEvent *e)
{
    if (e->button() == Qt::RightButton)
        m_menu->exec(e->globalPosition().toPoint());
    else if (e->button() == Qt::LeftButton && m_scroll)
    {
        m_timer->stop();
        m_press_pos = e->position().x() - m_x1;
        m_pressed = true;
    }
    else
        QWidget::mousePressEvent(e);
}

void SkinnedTextScroller::mouseReleaseEvent(QMouseEvent *e)
{
   if(e->button() == Qt::RightButton)
        m_menu->exec(e->globalPosition().toPoint());
    else if (e->button() == Qt::LeftButton && m_scroll)
        m_timer->start();
    else
        QWidget::mouseReleaseEvent(e);
   m_pressed = false;
}

void SkinnedTextScroller::mouseMoveEvent (QMouseEvent *e)
{
    if (m_pressed)
    {
        int bound = m_pixmap.width();
        m_x1 = (qRound(e->position().x()) - m_press_pos) % bound;
        if (m_x1 > 0)
            m_x1 -= bound;
        m_x2 = m_x1 + m_pixmap.width();
        update();
    }
    else
        QWidget::mouseMoveEvent(e);
}

void SkinnedTextScroller::mouseDoubleClickEvent(QMouseEvent *e)
{
   if(e->button() == Qt::LeftButton)
   {
       PlayListManager *manager = PlayListManager::instance();
       PlayListModel *model = manager->currentPlayList();
       model->showDetailsForCurrent(this);
   }
   else
   {
       QWidget::mouseDoubleClickEvent(e);
   }
}

void SkinnedTextScroller::processState(Qmmp::State state)
{
    switch(state)
    {
    case Qmmp::Buffering:
    {
        connect(m_core, &SoundCore::bufferingProgress, this, &SkinnedTextScroller::setProgress);
        break;
    }
    case Qmmp::Playing:
    {
        disconnect(m_core, &SoundCore::bufferingProgress, this, &SkinnedTextScroller::setProgress);
        m_bufferText.clear();
        updateText();
        break;
    }
    case Qmmp::Stopped:
    {
        disconnect(m_core, &SoundCore::bufferingProgress, this, nullptr);
        break;
    }
    default:
        break;
    }
}

void SkinnedTextScroller::processMetaData()
{
    if(m_core->state() == Qmmp::Playing)
    {
        m_titleText = m_formater.format(m_core->trackInfo());
        updateText();
    }
}

void SkinnedTextScroller::preparePixmap(const QString &text, bool scrollable)
{
    m_scroll = scrollable;
    bool bitmap = m_bitmap && (text.toLatin1() == text.toLocal8Bit()); //use bitmap font if possible
    if(scrollable)
    {
         int textWidth = m_bitmap ? QString(text + SCROLL_SEP).size() * 5
                                  : m_metrics->horizontalAdvance(text + SCROLL_SEP);
         int count = 154 * m_ratio / textWidth + 1;
         int width = count * textWidth;
         QString fullText;
         for(int i = 0; i < count; ++i)
         {
             fullText.append(text + SCROLL_SEP);
         }
         m_pixmap = QPixmap(width, 14 * m_ratio);
         if(m_transparencyAction->isChecked())
             m_pixmap.fill(Qt::transparent);
         else
             m_pixmap.fill(m_skin->getMainColor(Skin::MW_BACKGROUND));
         QPainter painter(&m_pixmap);
         painter.setPen(m_color);
         painter.setFont(m_font);
         if(bitmap)
             drawBitmapText (0, 10 * m_ratio, fullText, &painter, m_skin);
         else
             painter.drawText (0, qMin(m_pixmap.height() / 2  + m_metrics->ascent() / 2, m_pixmap.height() - 2), fullText);
         m_x1 = 0;
         m_x2 = m_pixmap.width();
    }
    else
    {
        m_pixmap = QPixmap(154 * m_ratio,15 * m_ratio);
        if(m_transparencyAction->isChecked())
            m_pixmap.fill(Qt::transparent);
        else
            m_pixmap.fill(m_skin->getMainColor(Skin::MW_BACKGROUND));
        QPainter painter(&m_pixmap);
        painter.setPen(m_color);
        painter.setFont(m_font);
        if(bitmap)
            drawBitmapText (0, 10 * m_ratio, text, &painter, m_skin);
        else
            painter.drawText (0, qMin(m_pixmap.height() / 2  + m_metrics->ascent() / 2, m_pixmap.height() - 2), text);
    }
}

void SkinnedTextScroller::updateText() //draw text according priority
{
    if(!m_sliderText.isEmpty())
    {
        preparePixmap(m_sliderText);
        m_timer->stop();
    }
    else if(!m_bufferText.isEmpty())
    {
        preparePixmap(m_bufferText);
        m_timer->stop();
    }
    else if (!m_titleText.isEmpty())
    {
        preparePixmap(m_titleText, m_scrollAction->isChecked());
        m_timer->start();
    }
    else if(!m_defautText.isEmpty())
    {
        preparePixmap(m_defautText);
        m_timer->stop();
    }
    else
    {
        m_timer->stop();
        m_pixmap = QPixmap(154 * m_ratio, 14 * m_ratio);
        m_pixmap.fill(Qt::transparent);
        m_scroll = false;
    }
    update();
}

void SkinnedTextScroller::clearText()
{
    m_bufferText.clear();
    m_titleText.clear();
    updateText();
}
