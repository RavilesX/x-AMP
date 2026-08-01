/***************************************************************************
 *   Copyright (C) 2011-2026 by Ilya Kotov                                 *
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

#ifndef QSUIHOTKEYEDITOR_H
#define QSUIHOTKEYEDITOR_H

#include <QWidget>

namespace Ui {
    class QSUiHotkeyEditor;
}

/**
    @author Ilya Kotov <forkotov02@ya.ru>
*/
class QSUiHotkeyEditor : public QWidget
{
    Q_OBJECT

public:
    explicit QSUiHotkeyEditor(QWidget *parent = nullptr);
    virtual ~QSUiHotkeyEditor();

    void applyShortcuts();

private slots:
    void on_changeShortcutButton_clicked();
    void on_restoreShortcutsButton_clicked();

private:
    void loadShortcuts();
    Ui::QSUiHotkeyEditor *m_ui;
};

#endif // QSUIHOTKEYEDITOR_H
