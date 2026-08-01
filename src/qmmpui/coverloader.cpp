/***************************************************************************
 *   Copyright (C) 2024 by Ilya Kotov                                      *
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

#include <qmmp/metadatamanager.h>
#include "coverloader_p.h"

CoverLoader::CoverLoader(QObject *parent)
    : QThread{parent}
{

}

void CoverLoader::add(const QStringList &paths)
{
    MetaDataManager::instance()->prepareForAnotherThread();

    m_mutex.lock();

    for(const QString &path : std::as_const(paths))
        m_paths.push(path);

    m_mutex.unlock();

    start(QThread::IdlePriority);
}

void CoverLoader::finish()
{
    m_mutex.lock();
    m_paths.clear();
    m_mutex.unlock();
    wait();
}

void CoverLoader::run()
{
    m_mutex.lock();

    while(!m_paths.isEmpty())
    {
        QString path = m_paths.pop();

        m_mutex.unlock();
        QImage img = MetaDataManager::instance()->getCover(path);
        if(!img.isNull())
            emit ready(path, img.scaled(128, 128, Qt::KeepAspectRatio, Qt::SmoothTransformation));
        m_mutex.lock();
    }

    m_mutex.unlock();
}
