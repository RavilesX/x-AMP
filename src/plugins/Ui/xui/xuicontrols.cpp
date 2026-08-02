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

#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QRadialGradient>
#include "xuitheme.h"
#include "xuicontrols.h"

// ---------------------------------------------------------------- icon button

XUiIconButton::XUiIconButton(XUiIcons::Icon icon, QWidget *parent)
    : QWidget(parent), m_icon(icon)
{
    setCursor(Qt::PointingHandCursor);
    setFixedSize(m_iconSize + 16, m_iconSize + 16);
}

void XUiIconButton::setIcon(XUiIcons::Icon icon)
{
    m_icon = icon;
    update();
}

void XUiIconButton::setIconSize(int size)
{
    m_iconSize = size;
    setFixedSize(size + 16, size + 16);
    update();
}

void XUiIconButton::setCheckable(bool checkable)
{
    m_checkable = checkable;
}

void XUiIconButton::setChecked(bool checked)
{
    if(m_checked == checked)
        return;
    m_checked = checked;
    update();
}

void XUiIconButton::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);

    if(m_hovered || m_pressed)
    {
        p.setPen(Qt::NoPen);
        p.setBrush(m_pressed ? XUi::Border : XUi::Hover);
        p.drawRoundedRect(rect().adjusted(1, 1, -1, -1), 9, 9);
    }

    const QColor color = m_checked ? XUi::Accent : (m_hovered ? XUi::Text : XUi::TextDim);
    const QRectF box((width() - m_iconSize) / 2.0, (height() - m_iconSize) / 2.0,
                     m_iconSize, m_iconSize);
    XUiIcons::paint(&p, m_icon, box, color);
}

void XUiIconButton::enterEvent(QEnterEvent *)
{
    m_hovered = true;
    update();
}

void XUiIconButton::leaveEvent(QEvent *)
{
    m_hovered = false;
    m_pressed = false;
    update();
}

void XUiIconButton::mousePressEvent(QMouseEvent *e)
{
    if(e->button() != Qt::LeftButton)
        return;
    m_pressed = true;
    update();
}

void XUiIconButton::mouseReleaseEvent(QMouseEvent *e)
{
    if(e->button() != Qt::LeftButton || !m_pressed)
        return;
    m_pressed = false;
    if(rect().contains(e->pos()))
    {
        if(m_checkable)
        {
            m_checked = !m_checked;
            emit toggled(m_checked);
        }
        emit clicked();
    }
    update();
}

// ---------------------------------------------------------------- play button

XUiPlayButton::XUiPlayButton(QWidget *parent) : QWidget(parent)
{
    setCursor(Qt::PointingHandCursor);
    setFixedSize(78, 78);
}

void XUiPlayButton::setPlaying(bool playing)
{
    if(m_playing == playing)
        return;
    m_playing = playing;
    update();
}

void XUiPlayButton::setProgress(qreal progress)
{
    progress = qBound(0.0, progress, 1.0);
    //repaint only on a visible change; this is driven by the elapsed signal
    if(qAbs(progress - m_progress) < 0.001)
        return;
    m_progress = progress;
    update();
}

void XUiPlayButton::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);

    const QRectF box = rect().adjusted(3, 3, -3, -3);
    const QPointF centre = box.center();
    const qreal outer = box.width() / 2.0;

    //soft glow behind the button
    QRadialGradient glow(centre, outer + 6);
    glow.setColorAt(0.55, QColor(XUi::Accent.red(), XUi::Accent.green(), XUi::Accent.blue(), 60));
    glow.setColorAt(1.0, QColor(XUi::Accent.red(), XUi::Accent.green(), XUi::Accent.blue(), 0));
    p.setPen(Qt::NoPen);
    p.setBrush(glow);
    p.drawEllipse(centre, outer + 6, outer + 6);

    //body
    QLinearGradient body(box.topLeft(), box.bottomLeft());
    body.setColorAt(0.0, XUi::Elevated);
    body.setColorAt(1.0, XUi::Card);
    p.setBrush(body);
    p.drawEllipse(centre, outer - 4, outer - 4);

    //track and progress ring
    const QRectF ring = box.adjusted(1, 1, -1, -1);
    p.setBrush(Qt::NoBrush);
    p.setPen(QPen(XUi::Border, 3));
    p.drawEllipse(ring);
    if(m_progress > 0.0)
    {
        QPen pen(m_hovered ? XUi::AccentBright : XUi::Accent, 3);
        pen.setCapStyle(Qt::RoundCap);
        p.setPen(pen);
        //Qt angles are 1/16th of a degree, counter-clockwise from 3 o'clock;
        //start at the top and sweep clockwise
        p.drawArc(ring, 90 * 16, -int(m_progress * 360.0 * 16));
    }

    const qreal glyph = 26.0;
    QRectF g(centre.x() - glyph / 2.0, centre.y() - glyph / 2.0, glyph, glyph);
    if(!m_playing)
        g.translate(1.5, 0); //optical centring for the triangle
    XUiIcons::paint(&p, m_playing ? XUiIcons::Pause : XUiIcons::Play, g,
                    m_pressed ? XUi::TextDim : XUi::Text);
}

void XUiPlayButton::enterEvent(QEnterEvent *)
{
    m_hovered = true;
    update();
}

void XUiPlayButton::leaveEvent(QEvent *)
{
    m_hovered = false;
    m_pressed = false;
    update();
}

void XUiPlayButton::mousePressEvent(QMouseEvent *e)
{
    if(e->button() != Qt::LeftButton)
        return;
    m_pressed = true;
    update();
}

void XUiPlayButton::mouseReleaseEvent(QMouseEvent *e)
{
    if(e->button() != Qt::LeftButton || !m_pressed)
        return;
    m_pressed = false;
    if(rect().contains(e->pos()))
        emit clicked();
    update();
}

// -------------------------------------------------------------------- slider

XUiSlider::XUiSlider(QWidget *parent) : QWidget(parent)
{
    setCursor(Qt::PointingHandCursor);
    setFixedHeight(18);
    setMouseTracking(true);
}

void XUiSlider::setMaximum(qint64 maximum)
{
    m_maximum = qMax(qint64(0), maximum);
    m_value = qBound(qint64(0), m_value, m_maximum);
    update();
}

void XUiSlider::setValue(qint64 value)
{
    if(m_dragging)
        return; //the pointer owns the position while dragging
    value = qBound(qint64(0), value, m_maximum);
    if(m_value == value)
        return;
    m_value = value;
    update();
}

void XUiSlider::setGripAlwaysVisible(bool visible)
{
    m_gripAlwaysVisible = visible;
    update();
}

qint64 XUiSlider::valueAt(int x) const
{
    if(width() <= 1 || m_maximum <= 0)
        return 0;
    const qreal ratio = qBound(0.0, qreal(x) / qreal(width()), 1.0);
    return qint64(ratio * m_maximum);
}

void XUiSlider::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);

    const qreal y = height() / 2.0;
    const qreal thickness = 4.0;
    const qreal filled = m_maximum > 0 ? width() * (qreal(m_value) / qreal(m_maximum)) : 0.0;

    QRectF track(0, y - thickness / 2.0, width(), thickness);
    p.setPen(Qt::NoPen);
    p.setBrush(XUi::Border);
    p.drawRoundedRect(track, thickness / 2.0, thickness / 2.0);

    if(filled > 0.0)
    {
        QLinearGradient g(0, 0, filled, 0);
        g.setColorAt(0.0, XUi::AccentDeep);
        g.setColorAt(1.0, XUi::Accent);
        p.setBrush(g);
        p.drawRoundedRect(QRectF(0, y - thickness / 2.0, filled, thickness),
                          thickness / 2.0, thickness / 2.0);
    }

    if(m_gripAlwaysVisible || m_hovered || m_dragging)
    {
        p.setBrush(XUi::Text);
        p.setPen(QPen(XUi::Accent, 2));
        p.drawEllipse(QPointF(filled, y), 5, 5);
    }
}

void XUiSlider::mousePressEvent(QMouseEvent *e)
{
    if(e->button() != Qt::LeftButton || m_maximum <= 0)
        return;
    m_dragging = true;
    m_value = valueAt(e->position().x());
    update();
    emit moved(m_value);
}

void XUiSlider::mouseMoveEvent(QMouseEvent *e)
{
    if(!m_dragging)
        return;
    m_value = valueAt(e->position().x());
    update();
    emit moved(m_value);
}

void XUiSlider::mouseReleaseEvent(QMouseEvent *e)
{
    if(e->button() != Qt::LeftButton || !m_dragging)
        return;
    m_dragging = false;
    m_value = valueAt(e->position().x());
    update();
    emit released(m_value);
}

void XUiSlider::enterEvent(QEnterEvent *)
{
    m_hovered = true;
    update();
}

void XUiSlider::leaveEvent(QEvent *)
{
    m_hovered = false;
    update();
}

// ----------------------------------------------------------------- cover art

XUiCoverArt::XUiCoverArt(QWidget *parent) : QWidget(parent)
{
    setFixedSize(132, 132);
}

void XUiCoverArt::setCover(const QPixmap &cover)
{
    m_cover = cover;
    update();
}

void XUiCoverArt::clear()
{
    m_cover = QPixmap();
    update();
}

void XUiCoverArt::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);
    p.setRenderHint(QPainter::SmoothPixmapTransform, true);

    QPainterPath clip;
    clip.addRoundedRect(QRectF(rect()), 12, 12);
    p.setClipPath(clip);

    if(!m_cover.isNull())
    {
        //cover the square without distorting the artwork
        const QPixmap scaled = m_cover.scaled(size() * devicePixelRatioF(),
                                              Qt::KeepAspectRatioByExpanding,
                                              Qt::SmoothTransformation);
        QPixmap shown = scaled;
        shown.setDevicePixelRatio(devicePixelRatioF());
        const QPointF offset((width() - shown.width() / devicePixelRatioF()) / 2.0,
                             (height() - shown.height() / devicePixelRatioF()) / 2.0);
        p.drawPixmap(offset, shown);
        return;
    }

    QLinearGradient g(rect().topLeft(), rect().bottomRight());
    g.setColorAt(0.0, XUi::Elevated);
    g.setColorAt(1.0, XUi::Card);
    p.setPen(Qt::NoPen);
    p.setBrush(g);
    p.drawRect(rect());

    //placeholder wordmark: an X in the accent colour
    QFont f = font();
    f.setPointSizeF(height() * 0.42);
    f.setBold(true);
    p.setFont(f);
    p.setPen(XUi::Accent);
    p.drawText(rect(), Qt::AlignCenter, QStringLiteral("X"));

    p.setClipping(false);
    p.setPen(QPen(XUi::Border, 1));
    p.setBrush(Qt::NoBrush);
    p.drawRoundedRect(QRectF(rect()).adjusted(0.5, 0.5, -0.5, -0.5), 12, 12);
}

// ---------------------------------------------------------------------- chip

XUiChip::XUiChip(QWidget *parent) : QWidget(parent)
{
    QFont f = font();
    f.setPointSizeF(f.pointSizeF() * 0.85);
    setFont(f);
    setFixedHeight(22);
}

void XUiChip::setText(const QString &text)
{
    if(m_text == text)
        return;
    m_text = text;
    updateGeometry();
    update();
}

QSize XUiChip::sizeHint() const
{
    return QSize(QFontMetrics(font()).horizontalAdvance(m_text) + 18, 22);
}

void XUiChip::paintEvent(QPaintEvent *)
{
    if(m_text.isEmpty())
        return;
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);
    p.setPen(Qt::NoPen);
    p.setBrush(XUi::Elevated);
    p.drawRoundedRect(rect(), 6, 6);
    p.setPen(XUi::TextDim);
    p.drawText(rect(), Qt::AlignCenter, m_text);
}
