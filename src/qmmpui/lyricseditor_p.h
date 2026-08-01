/***************************************************************************
 *   Copyright (C) 2021-2026 by Ilya Kotov                                 *
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

#ifndef LYRICSEDITOR_P_H
#define LYRICSEDITOR_P_H

#include <QWidget>
#include <qmmp/trackinfo.h>
#include "editorbase_p.h"

namespace Ui {
class TextEditor;
}

class MetaDataModel;

class LyricsEditor : public EditorBase
{
    Q_OBJECT
public:
    explicit LyricsEditor(MetaDataModel *model, const TrackInfo &info, QWidget *parent = nullptr);
    ~LyricsEditor();

    void save() override;
    bool isEditable() const override;

private slots:
    void on_loadButton_clicked();
    void on_deleteButton_clicked();
    void on_saveAsButton_clicked();

private:
    Ui::TextEditor *m_ui;
    MetaDataModel *m_model;
    QString m_lastDir;
    bool m_editable;
    TrackInfo m_info;
};

#endif // LYRICSEDITOR_P_H
