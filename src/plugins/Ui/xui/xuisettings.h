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

#ifndef XUISETTINGS_H
#define XUISETTINGS_H

#include <QColor>
#include <QWidget>

class QCheckBox;
class QLabel;
class QPushButton;

/*!
 * Preferences page for this interface, added to the shared ConfigDialog.
 *
 * The settings live under the XUi group so they stay next to the geometry
 * the main window already saves there.
 */
class XUiSettings : public QWidget
{
    Q_OBJECT
public:
    explicit XUiSettings(QWidget *parent = nullptr);

    /*! Called when the dialog is accepted. */
    void writeSettings();

    //keys, so the main window and this page cannot drift apart
    static const QString ShowEqualizerKey;
    static const QString ShowPlaylistKey;
    static const QString HideOnCloseKey;
    static const QString BackgroundKey;

signals:
    /*!
     * The accent was applied without closing the dialog, so whoever built
     * this page should repaint. Emitted by the Apply button only.
     */
    void accentApplied();

    /*! The playlist's background image was chosen or cleared. */
    void backgroundApplied();

private slots:
    void pickAccent();
    void resetAccent();
    void applyAccent();
    void pickBackground();
    void clearBackground();

private:
    /*! Paints the chosen colour onto the swatch button. */
    void showAccent();
    QColor m_accent;
    QPushButton *m_accentButton;

    /*!
     * Writes the chosen image to the settings and announces it.
     *
     * Applied the moment it is picked rather than on OK: the whole point of
     * the effect is what it looks like, and an image cannot be judged from a
     * file name. Clear puts it back.
     */
    void applyBackground();
    /*! Shows the chosen file's name, elided, next to its buttons. */
    void showBackground();
    QString m_background;
    QLabel *m_backgroundName;

    QCheckBox *m_showEqualizer;
    QCheckBox *m_showPlaylist;
    QCheckBox *m_hideOnClose;
};

#endif
