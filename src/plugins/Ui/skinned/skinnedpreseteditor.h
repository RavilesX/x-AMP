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
#ifndef SKINNEDPRESETEDITOR_H
#define SKINNEDPRESETEDITOR_H

#include <QDialog>

namespace Ui {
class SkinnedPresetEditor;
}

/**
	@author Ilya Kotov <forkotov02@ya.ru>
*/

class SkinnedPresetEditor : public QDialog
{
Q_OBJECT
public:
    SkinnedPresetEditor(QWidget *parent = nullptr);
    ~SkinnedPresetEditor();

    void addPresets(const QStringList &names);
    void addAutoPresets(const QStringList &names);

signals:
    void presetLoaded(const QString &name, bool isAutoPreset);
    void presetRemoved(const QString &name, bool isAutoPreset);

private slots:
    void loadPreset();
    void removePreset();

private:
    Ui::SkinnedPresetEditor *m_ui;

};

#endif
