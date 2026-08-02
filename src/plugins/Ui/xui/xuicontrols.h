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

#ifndef XUICONTROLS_H
#define XUICONTROLS_H

#include <QPixmap>
#include <QWidget>
#include "xuiicons.h"

/*!
 * Flat icon button: no frame, a soft rounded highlight on hover, accent
 * colour when checked. Used for every small control in the interface.
 */
class XUiIconButton : public QWidget
{
    Q_OBJECT
public:
    explicit XUiIconButton(XUiIcons::Icon icon, QWidget *parent = nullptr);

    void setIcon(XUiIcons::Icon icon);
    void setCheckable(bool checkable);
    void setChecked(bool checked);
    bool isChecked() const { return m_checked; }
    /*! Icon side length in pixels; the widget sizes itself around it. */
    void setIconSize(int size);

signals:
    void clicked();
    void toggled(bool checked);

protected:
    void paintEvent(QPaintEvent *) override;
    void enterEvent(QEnterEvent *) override;
    void leaveEvent(QEvent *) override;
    void mousePressEvent(QMouseEvent *) override;
    void mouseReleaseEvent(QMouseEvent *) override;

private:
    XUiIcons::Icon m_icon;
    int m_iconSize = 20;
    bool m_checkable = false;
    bool m_checked = false;
    bool m_hovered = false;
    bool m_pressed = false;
};

/*!
 * The big round play/pause button, with a progress ring around it.
 */
class XUiPlayButton : public QWidget
{
    Q_OBJECT
public:
    explicit XUiPlayButton(QWidget *parent = nullptr);

    void setPlaying(bool playing);
    /*! Ring fill, 0.0 to 1.0. */
    void setProgress(qreal progress);

signals:
    void clicked();

protected:
    void paintEvent(QPaintEvent *) override;
    void enterEvent(QEnterEvent *) override;
    void leaveEvent(QEvent *) override;
    void mousePressEvent(QMouseEvent *) override;
    void mouseReleaseEvent(QMouseEvent *) override;

private:
    bool m_playing = false;
    bool m_hovered = false;
    bool m_pressed = false;
    qreal m_progress = 0.0;
};

/*!
 * Thin horizontal slider with a round grip, used for both the seek bar and
 * the volume control. Reports positions as 0..maximum.
 */
class XUiSlider : public QWidget
{
    Q_OBJECT
public:
    explicit XUiSlider(QWidget *parent = nullptr);

    void setMaximum(qint64 maximum);
    void setValue(qint64 value);
    qint64 value() const { return m_value; }
    /*! Hides the grip until the pointer is over the slider. */
    void setGripAlwaysVisible(bool visible);

signals:
    /*! Emitted while dragging, and once on click. */
    void moved(qint64 value);
    /*! Emitted when the drag ends, so callers can seek only once. */
    void released(qint64 value);

protected:
    void paintEvent(QPaintEvent *) override;
    void mousePressEvent(QMouseEvent *) override;
    void mouseMoveEvent(QMouseEvent *) override;
    void mouseReleaseEvent(QMouseEvent *) override;
    void enterEvent(QEnterEvent *) override;
    void leaveEvent(QEvent *) override;

private:
    qint64 valueAt(int x) const;

    qint64 m_maximum = 0;
    qint64 m_value = 0;
    bool m_dragging = false;
    bool m_hovered = false;
    bool m_gripAlwaysVisible = true;
};

/*!
 * Vertical equalizer slider: a thin rail with the travelled part lit from
 * the knob down, as in the design. Values are dB, symmetric around zero.
 */
class XUiEqSlider : public QWidget
{
    Q_OBJECT
public:
    explicit XUiEqSlider(QWidget *parent = nullptr);

    void setRange(double minimum, double maximum);
    void setValue(double value);
    double value() const { return m_value; }

signals:
    void moved(double value);

protected:
    void paintEvent(QPaintEvent *) override;
    void mousePressEvent(QMouseEvent *) override;
    void mouseMoveEvent(QMouseEvent *) override;
    void mouseReleaseEvent(QMouseEvent *) override;
    void mouseDoubleClickEvent(QMouseEvent *) override;
    void wheelEvent(QWheelEvent *) override;
    void enterEvent(QEnterEvent *) override;
    void leaveEvent(QEvent *) override;

private:
    /*! Vertical centre of the knob for the current value. */
    qreal knobY() const;
    double valueAt(int y) const;

    double m_minimum = -12.0;
    double m_maximum = 12.0;
    double m_value = 0.0;
    bool m_dragging = false;
    bool m_hovered = false;
};

/*!
 * Pill-shaped on/off switch.
 */
class XUiToggle : public QWidget
{
    Q_OBJECT
public:
    explicit XUiToggle(QWidget *parent = nullptr);

    void setChecked(bool checked);
    bool isChecked() const { return m_checked; }

signals:
    void toggled(bool checked);

protected:
    void paintEvent(QPaintEvent *) override;
    void mouseReleaseEvent(QMouseEvent *) override;

private:
    bool m_checked = false;
};

/*!
 * Text button with a trailing chevron, used for the presets menu.
 */
class XUiMenuButton : public QWidget
{
    Q_OBJECT
public:
    explicit XUiMenuButton(const QString &text, QWidget *parent = nullptr);

    void setText(const QString &text);

signals:
    void clicked();

protected:
    void paintEvent(QPaintEvent *) override;
    void mouseReleaseEvent(QMouseEvent *) override;
    void enterEvent(QEnterEvent *) override;
    void leaveEvent(QEvent *) override;
    QSize sizeHint() const override;

private:
    QString m_text;
    bool m_hovered = false;
};

/*!
 * Album artwork with rounded corners. Falls back to the x-AMP wordmark on a
 * dark panel when the track has no cover.
 */
class XUiCoverArt : public QWidget
{
    Q_OBJECT
public:
    explicit XUiCoverArt(QWidget *parent = nullptr);

    void setCover(const QPixmap &cover);
    void clear();

protected:
    void paintEvent(QPaintEvent *) override;

private:
    QPixmap m_cover;
};

/*!
 * Small rounded label for the format badges (MP3, 320 kbps, 44 kHz).
 */
class XUiChip : public QWidget
{
    Q_OBJECT
public:
    explicit XUiChip(QWidget *parent = nullptr);

    void setText(const QString &text);
    QString text() const { return m_text; }

protected:
    void paintEvent(QPaintEvent *) override;
    QSize sizeHint() const override;

private:
    QString m_text;
};

#endif
