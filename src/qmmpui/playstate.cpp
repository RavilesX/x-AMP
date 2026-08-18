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

#include <QRandomGenerator>
#include "qmmpuisettings.h"
#include "playstate_p.h"

PlayState::PlayState(PlayListModel *model) : m_model(model)
{
    m_ui_settings = QmmpUiSettings::instance();
}

ShufflePlayState::ShufflePlayState(PlayListModel * model) : PlayState(model)
{
    ShufflePlayState::prepare();
}

bool ShufflePlayState::next()
{
    if(m_model->isEmpty())
        return false;

    if (m_shuffled_current >= m_shuffled_indexes.count() - 1)
    {
        if (!m_ui_settings->isRepeatableList())
            return false;

        prepare();
    }
    else
        m_shuffled_current = (m_shuffled_current + 1) % m_shuffled_indexes.count();

    return m_model->setCurrent(m_shuffled_indexes.at(m_shuffled_current));

}

int ShufflePlayState::nextIndex()
{
    if(m_model->isEmpty())
        return -1;

    if (m_shuffled_current >= m_shuffled_indexes.count() - 1)
    {
        if (!m_ui_settings->isRepeatableList())
            return -1;

        prepare();
    }
    return m_shuffled_indexes.at((m_shuffled_current + 1) % m_shuffled_indexes.count());
}

bool ShufflePlayState::previous()
{
    if(m_model->isEmpty())
        return false;

    if (m_shuffled_current <= 0)
    {
        if (!m_ui_settings->isRepeatableList())
            return false;

        prepare();
        m_shuffled_current = m_shuffled_indexes.count() - 1;
    }

    if (m_model->trackCount() > 1)
        m_shuffled_current--;

    return m_model->setCurrent(m_shuffled_indexes.at(m_shuffled_current));
}

bool ShufflePlayState::isFinished() const
{
    //the scheduler asks whether the play order is exhausted, so "Repeat All"
    //is deliberately not taken into account here
    return m_model->isEmpty() || m_shuffled_current >= m_shuffled_indexes.count() - 1;
}

void ShufflePlayState::prepare()
{
    QRandomGenerator *rg = QRandomGenerator::global();

    resetState();
    for(int i = 0; i < m_model->trackCount(); i++)
    {
        if (i != m_model->currentIndex())
            m_shuffled_indexes << i;
    }

    for (int i = 0; i < m_shuffled_indexes.count(); i++)
        m_shuffled_indexes.swapItemsAt(i, rg->generate() % m_shuffled_indexes.size());

    m_shuffled_indexes.prepend(m_model->currentIndex());
}

void ShufflePlayState::resetState()
{
    m_shuffled_indexes.clear();
    m_shuffled_current = 0;
}

NormalPlayState::NormalPlayState(PlayListModel * model) : PlayState(model)
{}

bool NormalPlayState::next()
{
    if(m_model->isEmpty())
        return false;

    if(m_ui_settings->isRepeatableList() && m_model->currentIndex() == m_model->trackCount() - 1)
        return m_model->setCurrent(0);

    if(m_model->currentIndex() + 1 >= m_model->trackCount())
        return false;

    return m_model->setCurrent(m_model->currentIndex() + 1);
}

bool NormalPlayState::previous()
{
    if(m_model->isEmpty())
        return false;

    if(m_ui_settings->isRepeatableList())
    {
        if(m_model->currentIndex() == 0)
            return m_model->setCurrent(m_model->trackCount() - 1);
    }

    if(m_model->currentIndex() == 0)
        return false;

    return m_model->setCurrent(m_model->currentIndex() - 1);
}

bool NormalPlayState::isFinished() const
{
    //"Repeat All" is ignored on purpose, see ShufflePlayState::isFinished()
    return m_model->isEmpty() || m_model->currentIndex() + 1 >= m_model->trackCount();
}

int NormalPlayState::nextIndex()
{
    if(m_model->isEmpty())
        return -1;

    if (m_model->currentIndex() == m_model->trackCount() - 1)
        return m_ui_settings->isRepeatableList() ? 0 : -1;

    return m_model->currentIndex() + 1;
}
