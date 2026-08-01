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

#include <QApplication>
#include <QLocale>
#include <QIcon>
#include <stdlib.h>
#ifdef Q_OS_WIN
#include <QtGlobal>
#include <windows.h>
#include <winuser.h>
#endif
#include <qmmp/qmmp.h>
#include "qmmpstarter.h"

#ifdef Q_OS_UNIX
#include <QSettings>
#include <signal.h>
#endif

#ifdef Q_OS_UNIX
static int setupUnixSignalHandlers()
{
    struct sigaction term = {};

    term.sa_handler = QMMPStarter::termSignalHandler;
    sigemptyset(&term.sa_mask);
    term.sa_flags |= SA_RESTART;

    if (sigaction(SIGTERM, &term, nullptr))
        return 2;

    return 0;
}
#endif


int main(int argc, char *argv[])
{
#ifdef Q_OS_WIN
    //allows to activate main window from other instances
    AllowSetForegroundWindow(ASFW_ANY);
#endif
    QCoreApplication::setQuitLockEnabled(false);

#ifdef Q_OS_UNIX
    //using XWayland for skinned user interface
    if(qEnvironmentVariable("XDG_SESSION_TYPE") == QLatin1String("wayland") && !qEnvironmentVariableIsSet("QT_QPA_PLATFORM"))
    {
        QSettings settings(QStringLiteral("qmmp"), QStringLiteral("qmmp"));
        if(settings.value(QStringLiteral("Ui/current_plugin")).toString() == QLatin1String("skinned"))
            qputenv("QT_QPA_PLATFORM", "xcb");
    }
    setupUnixSignalHandlers();
#endif

    QApplication a(argc, argv);
    a.setApplicationName(u"qmmp"_s);
    a.setOrganizationName(u"qmmp"_s);
    QIcon icon;
    icon.addFile(u":/16x16/qmmp.png"_s);
    icon.addFile(u":/32x32/qmmp.png"_s);
    icon.addFile(u":/48x48/qmmp.png"_s);
    icon.addFile(u":/56x56/qmmp.png"_s);
    icon.addFile(u":/64x64/qmmp.png"_s);
    icon.addFile(u":/128x128/qmmp.png"_s);
    icon.addFile(u":/256x256/qmmp.png"_s);
    icon.addFile(u":/scalable/qmmp.svgz"_s);
    a.setWindowIcon(icon);

    QMMPStarter starter;

    if(starter.isFinished())
        return starter.exitCode();

    a.setQuitOnLastWindowClosed(false);
    return a.exec();
}
