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

#include <QWidget>

class QCheckBox;

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

private:
    QCheckBox *m_showEqualizer;
    QCheckBox *m_showPlaylist;
    QCheckBox *m_hideOnClose;
};

#endif
