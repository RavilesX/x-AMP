/***************************************************************************
 *   Copyright (C) 2008-2026 by Ilya Kotov                                 *
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
#include "commandlinehandler.h"

class CommandLineHandlerPrivate
{
public:
    struct CommandLineOption
    {
        QStringList names;
        QStringList values;
        QString helpString;
        CommandLineHandler::OptionFlags flags;

        inline bool operator == (const CommandLineOption &opt) const
        {
            return names == opt.names &&
                   values == opt.values &&
                   helpString == opt.helpString &&
                   flags == opt.flags;
        }
    };

    QMap<int, CommandLineOption> options;
};


CommandLineHandler::CommandLineHandler() :
    d_ptr(new CommandLineHandlerPrivate)
{}

CommandLineHandler::~CommandLineHandler()
{
    delete d_ptr;
}

QStringList CommandLineHandler::helpString() const
{
    Q_D(const CommandLineHandler);
    QStringList out;
    for(const CommandLineHandlerPrivate::CommandLineOption &opt : std::as_const(d->options))
    {
        if(opt.flags & HiddenFromHelp)
            continue;

        if(opt.values.isEmpty())
            out << QStringLiteral("%1||%2").arg(opt.names.join(u", "_s), opt.helpString);
        else
            out << QStringLiteral("%1 <%2>||%3").arg(opt.names.join(u", "_s), opt.values.join(u"> <"_s), opt.helpString);
    }
    return out;
}

QString CommandLineHandler::helpString(int id) const
{
    Q_D(const CommandLineHandler);
    if(d->options[id].values.isEmpty())
        return QStringLiteral("%1||%2").arg(d->options[id].names.join(u", "_s), d->options[id].helpString);

    return QStringLiteral("%1 <%2>||%3").arg(d->options[id].names.join(u", "_s), d->options[id].values.join(u"> <"_s), d->options[id].helpString);
}

int CommandLineHandler::identify(const QString &name) const
{
    for(const CommandLineHandlerPrivate::CommandLineOption &opt : std::as_const(d_ptr->options))
    {
        if(opt.names.contains(name))
            return d_ptr->options.key(opt);
    }
    return -1;
}

CommandLineHandler::OptionFlags CommandLineHandler::flags(int id) const
{
    return d_ptr->options.value(id).flags;
}

void CommandLineHandler::registerOption(int id, const QString &name, const QString &helpString, const QStringList &values)
{
    registerOption(id, QStringList{ name }, helpString, values);
}

void CommandLineHandler::registerOption(int id, const QStringList &names, const QString &helpString, const QStringList &values)
{
    CommandLineHandlerPrivate::CommandLineOption opt;
    opt.names = names;
    opt.values = values;
    opt.helpString = helpString;
    d_ptr->options.insert(id, opt);
}

void CommandLineHandler::setOptionFlags(int id, OptionFlags flags)
{
    d_ptr->options[id].flags = flags;
}
