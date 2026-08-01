/***************************************************************************
 *   Copyright (C) 2014-2026 by Ilya Kotov                                 *
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

#include <QStringList>
#include <QHash>
#include "channelmap.h"

namespace ChannelMapPrivate {
static Qmmp::ChannelPosition internalMap[9] = {
    Qmmp::CHAN_FRONT_LEFT,
    Qmmp::CHAN_FRONT_RIGHT,
    Qmmp::CHAN_REAR_LEFT,
    Qmmp::CHAN_REAR_RIGHT,
    Qmmp::CHAN_FRONT_CENTER,
    Qmmp::CHAN_REAR_CENTER,
    Qmmp::CHAN_LFE,
    Qmmp::CHAN_SIDE_LEFT,
    Qmmp::CHAN_SIDE_RIGHT
};
}

ChannelMap::ChannelMap(int channels)
{
    int mask = 0;

    switch (channels)
    {
    case 1:
        mask = Qmmp::CHAN_FRONT_LEFT;
        break;
    case 2:
        mask = Qmmp::CHAN_FRONT_LEFT
               | Qmmp::CHAN_FRONT_RIGHT;
        break;
    case 3:
        mask = Qmmp::CHAN_FRONT_LEFT
               | Qmmp::CHAN_FRONT_RIGHT
               | Qmmp::CHAN_FRONT_CENTER;
        break;
    case 4:
        mask = Qmmp::CHAN_FRONT_LEFT
               | Qmmp::CHAN_FRONT_RIGHT
               | Qmmp::CHAN_REAR_LEFT
               | Qmmp::CHAN_REAR_RIGHT;
        break;
    case 5:
        mask = Qmmp::CHAN_FRONT_LEFT
               | Qmmp::CHAN_FRONT_RIGHT
               | Qmmp::CHAN_FRONT_CENTER
               | Qmmp::CHAN_REAR_LEFT
               | Qmmp::CHAN_REAR_RIGHT;
        break;
    case 6:
        mask = Qmmp::CHAN_FRONT_LEFT
               | Qmmp::CHAN_FRONT_RIGHT
               | Qmmp::CHAN_FRONT_CENTER
               | Qmmp::CHAN_LFE
               | Qmmp::CHAN_REAR_LEFT
               | Qmmp::CHAN_REAR_RIGHT;
        break;
    case 7:
        mask = Qmmp::CHAN_FRONT_LEFT
               | Qmmp::CHAN_FRONT_RIGHT
               | Qmmp::CHAN_FRONT_CENTER
               | Qmmp::CHAN_LFE
               | Qmmp::CHAN_REAR_CENTER
               | Qmmp::CHAN_SIDE_LEFT
               | Qmmp::CHAN_SIDE_RIGHT;
        break;
    case 8:
        mask = Qmmp::CHAN_FRONT_LEFT
               | Qmmp::CHAN_FRONT_RIGHT
               | Qmmp::CHAN_FRONT_CENTER
               | Qmmp::CHAN_LFE
               | Qmmp::CHAN_REAR_LEFT
               | Qmmp::CHAN_REAR_RIGHT
               | Qmmp::CHAN_SIDE_LEFT
               | Qmmp::CHAN_SIDE_RIGHT;
        break;
    default:
        ;
    }

    for(int i = 0; i < 9; ++i)
    {
        if(mask & ChannelMapPrivate::internalMap[i])
            append(ChannelMapPrivate::internalMap[i]);
    }
}

int ChannelMap::mask() const
{
    int mask = 0;
    for(const Qmmp::ChannelPosition channel : std::as_const(*this))
    {
        mask |= channel;
    }
    return mask;
}

ChannelMap ChannelMap::remaped() const
{
    ChannelMap map;
    for(int i = 0; i < 9; ++i)
    {
         if(contains(ChannelMapPrivate::internalMap[i]))
            map.append(ChannelMapPrivate::internalMap[i]);
    }
    while (map.count() < count())
    {
        map.append(Qmmp::CHAN_NULL);
    }
    return map;
}

QString ChannelMap::toString() const
{
    static const QHash <Qmmp::ChannelPosition, QString> names = {
        { Qmmp::CHAN_NULL, u"NA"_s },
        { Qmmp::CHAN_FRONT_LEFT, u"FL"_s },
        { Qmmp::CHAN_FRONT_RIGHT, u"FR"_s },
        { Qmmp::CHAN_REAR_LEFT, u"RL"_s },
        { Qmmp::CHAN_REAR_RIGHT, u"RR"_s },
        { Qmmp::CHAN_FRONT_CENTER, u"FC"_s },
        { Qmmp::CHAN_REAR_CENTER, u"RC"_s },
        { Qmmp::CHAN_LFE, u"LFE"_s },
        { Qmmp::CHAN_SIDE_LEFT, u"SL"_s },
        { Qmmp::CHAN_SIDE_RIGHT, u"SR"_s }
    };

    QStringList list;
    for(const Qmmp::ChannelPosition channel : std::as_const(*this))
       list << names.value(channel);

    return list.join(QLatin1Char(','));
}
