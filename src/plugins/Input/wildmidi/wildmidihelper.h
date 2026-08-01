/***************************************************************************
 *   Copyright (C) 2010-2026 by Ilya Kotov                                 *
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

#ifndef WILDMIDIHELPER_H
#define WILDMIDIHELPER_H

#include <QMutex>
#include <QStringList>

class WildMidiHelper
{
public:
    bool initialize();
    void readSettings();
    quint32 sampleRate() const;
    void addPtr(void *t);
    void removePtr(void *t);
    QStringList configFiles() const;
    bool validateConfigFile(const QString &path) const;
    static WildMidiHelper *instance();

private:
    WildMidiHelper();
    ~WildMidiHelper();
    static void destroy();
    static WildMidiHelper *m_instance;
    bool m_inited = false;
    bool m_updateSettings = false;
    QMutex m_mutex;
    QList<void *> m_ptrs;
    quint32 m_sampleRate = 0;
};

#endif // WILDMIDIHELPER_H
