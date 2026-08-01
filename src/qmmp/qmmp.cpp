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
#include <QCoreApplication>
#include <QSettings>
#include <QLocale>
#include <QFile>
#include <QByteArray>
#include <QStandardPaths>

#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)
#define DEV_SUFFIX "dev"

#include "qmmp.h"

Q_GLOBAL_STATIC(QString, configDir);
Q_GLOBAL_STATIC(QString, langID);
#ifdef Q_OS_WIN
Q_GLOBAL_STATIC(QString, appDir);
#endif


#ifdef QT_DEBUG
Q_LOGGING_CATEGORY(core, "qmmp.core", QtDebugMsg)
Q_LOGGING_CATEGORY(plugin, "qmmp.plugin", QtDebugMsg)
#else
Q_LOGGING_CATEGORY(core, "qmmp.core", QtWarningMsg)
Q_LOGGING_CATEGORY(plugin, "qmmp.plugin", QtWarningMsg)
#endif

QString Qmmp::configDir()
{
#ifdef Q_OS_WIN
    if(::configDir()->isEmpty())
    {
        if(isPortable())
            return *::appDir() + u"/.qmmp"_s;

        return QDir::homePath() + u"/.qmmp"_s;
    }
#else
    if(::configDir()->isEmpty())
        return QStringLiteral("%1/%2").arg(QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation),
                                           QCoreApplication::organizationName());

#endif
    return *::configDir();
}

void Qmmp::setConfigDir(const QString &path)
{
    ::configDir()->operator = (path);
}

QString Qmmp::cacheDir()
{
#ifdef Q_OS_WIN
    return *::configDir();
#else
    if(::configDir()->isEmpty())
        return QStringLiteral("%1/%2").arg(QStandardPaths::writableLocation(QStandardPaths::GenericCacheLocation),
                                           QCoreApplication::organizationName());
    return *::configDir();
#endif
}

QString Qmmp::strVersion()
{
    QString ver = QStringLiteral("%1.%2.%3")
            .arg(QMMP_VERSION_MAJOR)
            .arg(QMMP_VERSION_MINOR)
            .arg(QMMP_VERSION_PATCH);
#if !QMMP_VERSION_STABLE
#ifdef SVN_REVISION
    ver += QStringLiteral("-svn-" SVN_REVISION);
#else
    ver += QStringLiteral("-" DEV_SUFFIX);
#endif
#endif
    return ver;
}

QString Qmmp::pluginPath()
{
    QByteArray path = qgetenv("QMMP_PLUGINS");
    if(!path.isEmpty())
        return QString::fromLocal8Bit(path);
    QString fallbackPath = QStringLiteral("%1/../lib/qmmp-" STR(QMMP_VERSION_MAJOR) "." STR(QMMP_VERSION_MINOR)).arg(qApp->applicationDirPath());

#if defined(Q_OS_WIN) && !defined(Q_OS_CYGWIN)
    QDir dir(fallbackPath);
#elif defined (QMMP_PLUGIN_DIR)
    QDir dir(QStringLiteral(QMMP_PLUGIN_DIR));
    if(!dir.exists())
        dir = QDir(fallbackPath);
#else
    QDir dir(fallbackPath);
#endif

    return dir.canonicalPath();
}

QStringList Qmmp::findPlugins(const QString &prefix)
{
    QDir pluginDir(pluginPath() + u"/"_s + prefix);
    QStringList paths;
    for(const QFileInfo &info : pluginDir.entryInfoList({ u"*.dll"_s, u"*.so"_s }, QDir::Files))
        paths << info.canonicalFilePath();
    return paths;
}

QString Qmmp::systemLanguageID()
{
    if(langID()->isEmpty())
    {
        langID()->operator = (uiLanguageID());
    }

    if(*langID() != "auto"_L1)
        return *langID();

#ifdef Q_OS_UNIX
    QByteArray v = qgetenv ("LC_ALL");
    if (v.isEmpty())
        v = qgetenv ("LC_MESSAGES");
    if (v.isEmpty())
        v = qgetenv ("LANG");
    if (!v.isEmpty())
        return QLocale(QString::fromLatin1(v)).name();
#endif
    return QLocale::system().name();
}

QString Qmmp::uiLanguageID()
{
    QSettings settings;
    QString langID = settings.value(u"General/locale"_s, u"auto"_s).toString();
    langID = langID.isEmpty() ? u"auto"_s : langID;
    return langID;
}

void Qmmp::setUiLanguageID(const QString &code)
{
    QSettings settings;
    settings.setValue(u"General/locale"_s, code);
    langID()->clear();
}

QString Qmmp::dataPath()
{
    return QDir(QStringLiteral("%1/../share/qmmp" APP_NAME_SUFFIX).arg(qApp->applicationDirPath())).absolutePath();
}

QString Qmmp::userDataPath()
{
#if defined(Q_OS_WIN) && !defined(Q_OS_CYGWIN)
    return configDir();
#else
    if(::configDir()->isEmpty())
    {
        return QStringLiteral("%1/%2").arg(QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation),
                                           QCoreApplication::organizationName());
    }
    return *::configDir();
#endif
}

#ifdef Q_OS_WIN
bool Qmmp::isPortable()
{
    if(appDir()->isEmpty())
        appDir()->operator = (QCoreApplication::applicationDirPath());
    return QFile::exists(*appDir() + QStringLiteral("/../qmmp_portable.txt"));
}
#endif
