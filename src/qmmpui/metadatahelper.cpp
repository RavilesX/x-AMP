/***************************************************************************
 *   Copyright (C) 2015-2026 by Ilya Kotov                                 *
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

#include "metadatahelper_p.h"

MetaDataHelper *MetaDataHelper::m_instance = nullptr;

MetaDataHelper::MetaDataHelper()
{
    m_instance = this;
    m_group_formatter0 = new MetaDataFormatter();
    m_group_formatter1 = new MetaDataFormatter();
    m_title_formatters << new MetaDataFormatter();
}

MetaDataHelper::~MetaDataHelper()
{
    m_instance = nullptr;
    delete m_group_formatter0;
    delete m_group_formatter1;
    qDeleteAll(m_title_formatters);
    m_title_formatters.clear();
}

void MetaDataHelper::setTitleFormats(const QStringList &titleFormats)
{
    while(m_title_formatters.count() > titleFormats.count())
        delete m_title_formatters.takeLast();

    while(m_title_formatters.count() < titleFormats.count())
        m_title_formatters << new MetaDataFormatter();

    for(int i = 0; i < m_title_formatters.count(); ++i)
        m_title_formatters.at(i)->setPattern(titleFormats.at(i));
}

void MetaDataHelper::setGroupFormat0(const QString &groupFormat)
{
    m_group_formatter0->setPattern(groupFormat);
}

void MetaDataHelper::setGroupFormat1(const QString &groupFormat)
{
    m_group_formatter1->setPattern(groupFormat);
}

int MetaDataHelper::columnCount() const
{
    return m_title_formatters.count();
}

const MetaDataFormatter *MetaDataHelper::titleFormatter(int index) const
{
    return m_title_formatters.at(index);
}

const MetaDataFormatter *MetaDataHelper::groupFormatter0() const
{
    return m_group_formatter0;
}

const MetaDataFormatter *MetaDataHelper::groupFormatter1() const
{
    return m_group_formatter1;
}

MetaDataHelper *MetaDataHelper::instance()
{
    return m_instance;
}
