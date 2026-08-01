/***************************************************************************
 *   Copyright (C) 2017-2026 by Ilya Kotov                                 *
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

#include <qmmp/qmmp.h>
#include "metadataformattermenu.h"

MetaDataFormatterMenu::MetaDataFormatterMenu(Type type, QWidget *parent) :
    QMenu(parent)
{
    addAction(tr("Artist"))->setData(u"%p"_s);
    addAction(tr("Album"))->setData(u"%a"_s);
    addAction(tr("Album Artist"))->setData(u"%aa"_s);
    if(type == TITLE_MENU || type == COLUMN_MENU)
    {
        addAction(tr("Title"))->setData(u"%t"_s);
        addAction(tr("Track Number"))->setData(u"%n"_s);
        addAction(tr("Two-digit Track Number"))->setData(u"%NN"_s);
    }
    if(type == COLUMN_MENU)
    {
        addAction(tr("Track Index"))->setData(u"%I"_s);
    }
    addAction(tr("Genre"))->setData(u"%g"_s);
    addAction(tr("Comment"))->setData(u"%c"_s);
    addAction(tr("Composer"))->setData(u"%C"_s);
    addAction(tr("Disc Number"))->setData(u"%D"_s);
    addAction(tr("Year"))->setData(u"%y"_s);
    addAction(tr("Duration"))->setData(u"%l"_s);
    if(type == TITLE_MENU || type == COLUMN_MENU)
    {
        addAction(tr("File Name"))->setData(u"%f"_s);
        addAction(tr("File Path"))->setData(u"%F"_s);
        addAction(tr("Artist - Title"))->setData(u"%if(%p,%p - %t,%t)"_s);
        addAction(tr("Condition"))->setData(u"%if(%p&%t,%p - %t,%f)"_s);
    }
    else if(type == GROUP_MENU)
    {
        addAction(tr("Artist - Album"))->setData(u"%if(%p,%p - %a,%a)"_s);
        addAction(tr("Artist - [Year] Album"))->setData(u"%p%if(%p&%a, - %if(%y,[%y] ,),)%a"_s);
        addAction(tr("Condition"))->setData(u"%if(%p,%p - %a,%a)"_s);
    }
    else if(type == GROUP_EXTRA_ROW_MENU)
    {
        addAction(tr("Duration | Format | Bitrate"))->setData(tr("%if(%l,%l | ,)%{format} | %{bitrate} kbps"));
        addAction(tr("Duration | Format | Bitrate | Sample rate"))->setData(tr("%if(%l,%l | ,)%{format} | %{bitrate} kbps | %{samplerate} Hz"));
        addAction(tr("Duration | Format | Sample rate"))->setData(tr("%if(%l,%l | ,)%{format} | %{samplerate} Hz"));
        addAction(tr("Year | Duration | Bitrate"))->setData(tr("%y | %if(%l,%l | ,)%{bitrate} kbps"));
        addAction(tr("Year | Duration | Sample rate"))->setData(tr("%y | %if(%l,%l | ,)%{samplerate} Hz"));
        addAction(tr("Condition"))->setData(u"%if(%p,%p - %a,%a)"_s);
    }
    addAction(tr("Parent Directory Name"))->setData(u"%dir(0)"_s);
    addAction(tr("Parent Directory Path"))->setData(u"%dir"_s);
    addSeparator();
    addAction(tr("Bitrate"))->setData(u"%{bitrate}"_s);
    addAction(tr("Sample Rate"))->setData(u"%{samplerate}"_s);
    addAction(tr("Number of Channels"))->setData(u"%{channels}"_s);
    addAction(tr("Sample Size"))->setData(u"%{samplesize}"_s);
    addAction(tr("Format"))->setData(u"%{format}"_s);
    addAction(tr("Decoder"))->setData(u"%{decoder}"_s);
    if(type == TITLE_MENU || type == COLUMN_MENU)
    {
        addAction(tr("File Size"))->setData(u"%{filesize}"_s);
    }

    connect(this, &QMenu::triggered, this, [this] (QAction *action) {
        emit patternSelected(action->data().toString());
    });
}
