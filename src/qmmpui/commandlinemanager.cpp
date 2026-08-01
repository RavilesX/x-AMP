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

#include <QDir>
#include <QPluginLoader>
#include <QObject>
#include <QList>
#include <QApplication>
#include <QTranslator>
#include <cstdlib>
#include <iostream>
#include <qmmp/qmmp.h>
#include <qmmp/soundcore.h>
#include "uihelper.h"
#include "mediaplayer.h"
#include "commandlinemanager.h"

#ifdef Q_OS_WIN
#undef qPrintable 
#define qPrintable qUtf8Printable
#endif

Q_GLOBAL_STATIC(QList<CommandLineHandler *>, options);

class CommandLineManagerPrivate
{
public:
    static void checkOptions()
    {
        if(!options.exists())
        {
            for(const QString &filePath : Qmmp::findPlugins(u"CommandLineOptions"_s))
            {
                QPluginLoader loader(filePath);
                QObject *plugin = loader.instance();
                if (loader.isLoaded())
                    /*qCDebug(core) << "loaded plugin" << QFileInfo(filePath).filePath();*/;
                else
                    qCWarning(core) << loader.errorString();

                CommandLineHandler *option = nullptr;
                if(plugin)
                    option = qobject_cast<CommandLineHandler *>(plugin);

                if(option)
                {
                    options->append(option);
                    if(!option->translation().isEmpty())
                    {
                        QTranslator *translator = new QTranslator(qApp);
                        if(translator->load(option->translation() + Qmmp::systemLanguageID()))
                            qApp->installTranslator(translator);
                        else
                            delete translator;
                    }
                    option->registerOprions();
                }
            }
        }
    }
};

QString CommandLineManager::executeCommand(const QString &name, const QStringList &args, const QString &currentWorkingDir)
{
    CommandLineManagerPrivate::checkOptions();
    bool started = UiHelper::instance() && SoundCore::instance() && MediaPlayer::instance();

    for(CommandLineHandler *opt : std::as_const(*options))
    {
        int id = opt->identify(name);
        if(id < 0)
            continue;

        if(started || (opt->flags(id) & CommandLineHandler::NoStart))
            return opt->executeCommand(id, args, currentWorkingDir);

        qCWarning(core, "player objects are not created");
        return QString();
    }
    return QString();
}

bool CommandLineManager::hasOption(const QString &opt_str, CommandLineHandler::OptionFlags *flags)
{
    CommandLineManagerPrivate::checkOptions();
    if(flags)
        *flags = CommandLineHandler::OptionFlags();
    for(const CommandLineHandler *opt : std::as_const(*options))
    {
        int id = opt->identify(opt_str);
        if(id >= 0)
        {
            if(flags)
                *flags = opt->flags(id);
            return true;
        }
    }
    return false;
}

void CommandLineManager::printUsage()
{
    CommandLineManagerPrivate::checkOptions();
    for(const CommandLineHandler *opt : std::as_const(*options))
    {
        for(const QString &line : opt->helpString())
        {
            QString str = formatHelpString(line);
            if(!str.isEmpty())
                std::cout << qPrintable(str) << std::endl;
        }
    }
}

QString CommandLineManager::formatHelpString(const QString &line)
{
    QStringList list = line.split(u"||"_s, Qt::SkipEmptyParts);
    if(list.count() == 1)
        return list.constFirst();
    if(list.count() >= 2)
        return list.constFirst().leftJustified(25) + list.at(1);
    return QString();
}
