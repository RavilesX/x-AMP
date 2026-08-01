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

#include <QtPlugin>
#include <QLocale>
#include <QRegularExpression>
#include <qmmp/soundcore.h>
#include "seekoption.h"

void SeekOption::registerOprions()
{
    registerOption(SEEK, u"--seek"_s, tr("Seek to position in the current track"), QStringList{ u"time"_s });
    registerOption(SEEK_FWD, u"--seek-fwd"_s, tr("Seek forward"), QStringList{ u"time"_s });
    registerOption(SEEK_BWD, u"--seek-bwd"_s, tr("Seek backwards"), QStringList{ u"time"_s });
}

QString SeekOption::shortName() const
{
    return QLatin1String("SeekOption");
}

QString SeekOption::translation() const
{
    return QLatin1String(":/seek_plugin_");
}

QString SeekOption::executeCommand(int id, const QStringList &args, const QString &cwd)
{
    Q_UNUSED(cwd);

    SoundCore *core = SoundCore::instance();
    if(args.isEmpty() || core->state() != Qmmp::Playing || core->duration() <= 0)
        return QString();

    int seek_pos = -1;

    static const QRegularExpression seek_regexp1(u"^([0-9]{1,4})$"_s);
    static const QRegularExpression seek_regexp2(u"^([0-9]{1,2}):([0-9]{1,2})$"_s);

    QRegularExpressionMatch match;
    if((match = seek_regexp1.match(args.constFirst())).hasMatch())
        seek_pos = match.captured(1).toInt();
    else if((match = seek_regexp2.match(args.constFirst())).hasMatch())
        seek_pos = match.captured(1).toInt() * 60 + match.captured(2).toInt();

    if(seek_pos < 0)
        return tr("Invalid position specified");

    switch (id) {
    case SEEK: //seek absolute
        core->seek(qMin(seek_pos * 1000, core->duration()));
        break;
    case SEEK_FWD:
        core->seekRelative(seek_pos * 1000);
        break;
    case SEEK_BWD:
        core->seekRelative(-seek_pos * 1000);
        break;
    default:
        break;
    }

    return QString();
}
