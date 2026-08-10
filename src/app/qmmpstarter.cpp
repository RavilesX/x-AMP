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
#include <QDir>
#include <QLocalServer>
#include <QLocalSocket>
#include <QSettings>
#include <QIcon>
#include <QProcess>
#include <QTranslator>
#include <QLibraryInfo>
#include <cstdlib>
#include <iostream>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <string.h>
#ifdef Q_OS_UNIX
#include <QSocketNotifier>
#include <sys/socket.h>
#endif
#include <qmmp/qmmp.h>
#include <qmmpui/commandlinemanager.h>
#include <qmmpui/mediaplayer.h>
#include <qmmpui/playlistparser.h>
#include <qmmpui/uihelper.h>
#include <qmmpui/uiloader.h>
#include <qmmpui/qmmpuisettings.h>
#include "qmmpstarter.h"
#include "builtincommandlineoption.h"

#ifdef Q_OS_WIN
#include <sstream>
#include <QMessageBox>
#else
#include <sys/stat.h>
#endif

//x-AMP: distinct from upstream qmmp, otherwise launching one hands its
//command line to the other's running instance
#ifdef Q_OS_WIN
#define UDS_PATH QStringLiteral("xamp")
#else
#define UDS_PATH QStringLiteral("/tmp/xamp.sock.%1").arg(getuid())
#endif

#ifdef Q_OS_UNIX
int QMMPStarter::m_sigtermFd[2];
#endif

#ifdef Q_OS_WIN
#undef qPrintable 
#define qPrintable qUtf8Printable
#endif

using namespace std;

QMMPStarter::QMMPStarter() : QObject()
{
    if(qApp->arguments().contains(u"--debug"_s))
    {
        qSetMessagePattern(u"[%{type}]: %{function}: %{message}"_s);
        QLoggingCategory::setFilterRules(u"qmmp.core.debug=true\nqmmp.plugin.debug=true"_s);
    }
    else
    {
        qSetMessagePattern(u"%{function}: %{message}"_s);
    }

#ifndef QT_NO_SESSIONMANAGER
    connect(qApp, &QApplication::commitDataRequest, this, &QMMPStarter::commitData, Qt::DirectConnection);
#endif
    createPaths();
#ifdef Q_OS_WIN
    QString defaultConfig = Qmmp::dataPath() + QStringLiteral("/xamp-default.ini");
    QString userConfig = Qmmp::configDir() + QStringLiteral("/xamp.ini");
    if(!QFile::exists(userConfig) && QFile::exists(defaultConfig))
    {
        qCDebug(core) << "creating initial config";
        QFile::copy(defaultConfig, userConfig);
    }

    QFileInfo configDirInfo(Qmmp::configDir());
    QCoreApplication::setOrganizationName(configDirInfo.fileName());
    QSettings::setDefaultFormat(QSettings::IniFormat);
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, configDirInfo.canonicalPath());
    SetConsoleOutputCP(CP_UTF8);
#endif

    QTranslator *translator = new QTranslator(qApp);
    QString locale = Qmmp::systemLanguageID();
    if(translator->load((u":/qmmp_"_s) + locale))
        qApp->installTranslator(translator);

    QTranslator *qt_translator = new QTranslator(qApp);
    if(qt_translator->load(QLibraryInfo::path(QLibraryInfo::TranslationsPath) + u"/qtbase_"_s + locale))
        qApp->installTranslator(qt_translator);

    m_option_manager = new BuiltinCommandLineOption(this);
    m_option_manager->registerOprions();
    QStringList tmp = qApp->arguments().mid(1);

    argString = tmp.join(u"|||"_s);
    QHash<QString, QStringList> commands = m_option_manager->splitArgs(tmp);

    if(commands.contains(u"--help"_s) || commands.contains(u"-h"_s))
    {
        printUsage();
        m_finished = true;
        return;
    }
    if(commands.contains(u"--version"_s) || commands.contains(u"-v"_s))
    {
        printVersion();
        m_finished = true;
        return;
    }
    if(commands.contains(u"--ui-list"_s))
    {
        printUserInterfaces();
        m_finished = true;
        return;
    }
    if(commands.contains(u"--ui"_s))
    {
        QStringList args = commands.value(u"--ui"_s);
        if(args.size() == 1)
            UiLoader::select(args.constFirst());
    }


    for(auto it = commands.cbegin(); it != commands.cend(); ++it)
    {
        CommandLineHandler::OptionFlags flags;
        if(m_option_manager->identify(it.key()) < 0 && !CommandLineManager::hasOption(it.key(), &flags) &&
                it.key() != u"--no-start"_s && it.key() != u"--ui"_s && it.key() != u"--debug"_s)
        {
            cout << qPrintable(tr("Unknown command")) << endl;
            m_exit_code = EXIT_FAILURE;
            m_finished = true;
            return;
        }

        if(flags & CommandLineHandler::NoStart)
        {
            m_exit_code = EXIT_SUCCESS;
            m_finished = true;
            QString out = CommandLineManager::executeCommand(it.key(), it.value(), QDir::currentPath());
            while(out.endsWith(QChar::LineFeed))
#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
                out.removeLast();
#else
                out.truncate(out.size() - 1);
#endif

            if(!out.isEmpty())
            {
                //show dialog with command line documentation under ms windows
#ifdef QMMP_NO_CLI
                stringstream tmp_stream;
                tmp_stream.copyfmt(cout);
                streambuf *old_stream = cout.rdbuf(tmp_stream.rdbuf());
#endif
                cout << qPrintable(out) << endl;
#ifdef QMMP_NO_CLI
                string text = tmp_stream.str();
                QMessageBox::information(nullptr, tr("Command Line Help"), QString::fromStdString(text));
                cout.rdbuf(old_stream); //restore old stream buffer
#endif
            }
            return;
        }
    }

    m_server = new QLocalServer(this);
    m_socket = new QLocalSocket(this);
    bool noStart = commands.contains(u"--no-start"_s) || commands.contains(u"--quit"_s);

#ifdef Q_OS_WIN
    //Windows IPC implementation (named mutex and named pipe)
    //
    //x-AMP: its own GUID. This mutex is how a second launch recognises that
    //the player is already running, and upstream's name is what x-AMP used to
    //carry -- so with Qmmp open, x-AMP would decide it was its own second
    //instance, hand its command line to a player that is not it, and quit. The
    //named pipe beside it was given a fork-specific name (see UDS_PATH); this
    //was missed.
    m_named_mutex = CreateMutexA(nullptr, TRUE, "XAMP-8f2bd391-6f52-4d82-8b35-b123e8ffacad");
    if(GetLastError() == NO_ERROR && !noStart)
    {
        m_server->listen (UDS_PATH);
        startPlayer();
    }
    else
    {
        m_socket->connectToServer(UDS_PATH); //connecting
        m_socket->waitForConnected();
        if(!m_socket->isValid()) //invalid connection
        {
            qCWarning(core, "unable to connect to server");
            m_exit_code = EXIT_FAILURE;
            m_finished = true;
            return;
        }
        writeCommand();
    }
#else
    if(!noStart && m_server->listen(UDS_PATH)) //trying to create server
    {
#ifndef Q_OS_WIN
        chmod(UDS_PATH.toLocal8Bit().constData(), S_IRUSR | S_IWUSR);
#endif
        startPlayer();
    }
    else if(QFile::exists(UDS_PATH))
    {
        m_socket->connectToServer(UDS_PATH); //connecting
        m_socket->waitForConnected();
        if(!m_socket->isValid()) //invalid connection
        {
            if(!QLocalServer::removeServer(UDS_PATH))
            {
                qCWarning(core, "unable to remove invalid socket file");
                m_exit_code = EXIT_FAILURE;
                m_finished = true;
                return;
            }
            qCWarning(core, "removed invalid socket file");
            if(noStart)
            {
                m_exit_code = EXIT_FAILURE;
                m_finished = true;
                return;
            }

            if(m_server->listen(UDS_PATH))
            {
#ifndef Q_OS_WIN
                chmod(UDS_PATH.toLocal8Bit().constData(), S_IRUSR | S_IWUSR);
#endif
                startPlayer();
            }
            else
            {
                qCWarning(core, "server error: %s", qPrintable(m_server->errorString()));
                m_exit_code = EXIT_FAILURE;
                m_finished = true;
                return;
            }
        }
        else
            writeCommand();
    }
    else
        m_finished = true;
#endif
}

QMMPStarter::~QMMPStarter()
{
    delete m_ui;
#ifdef Q_OS_WIN
    if(m_named_mutex)
        ReleaseMutex(m_named_mutex);
#endif
}

bool QMMPStarter::isFinished() const
{
    return m_finished;
}

int QMMPStarter::exitCode() const
{
    return m_exit_code;
}

#ifdef Q_OS_UNIX
void QMMPStarter::termSignalHandler(int)
{
    char a = 1;
    size_t len = ::write(m_sigtermFd[0], &a, sizeof(a));
    Q_UNUSED(len);
}
#endif

void QMMPStarter::startPlayer()
{
    connect(m_server, &QLocalServer::newConnection, this, &QMMPStarter::readCommand);
    QStringList args = argString.split(u"|||"_s, Qt::SkipEmptyParts);

#ifdef Q_OS_WIN
    QIcon::setThemeSearchPaths(QStringList{ qApp->applicationDirPath() + u"/../share/themes/"_s });
    QIcon::setThemeName(u"oxygen"_s);
#else
    //add extra theme path;
    QStringList theme_paths = QIcon::themeSearchPaths();
    QString share_path = QString::fromLatin1(qgetenv("XDG_DATA_HOME"));
    if(share_path.isEmpty())
        share_path = QDir::homePath() + u"/.local/share"_s;
    theme_paths << share_path + u"/icons"_s;
    theme_paths.removeDuplicates();
    QIcon::setThemeSearchPaths(theme_paths);

    //copy config from previous version
    //x-AMP: QSettings derives the file name from the application name, so this
    //is xamp.conf now. Left as qmmp.conf it would never exist and the import
    //below would rerun on every start.
    QString configFile = Qmmp::configDir() + u"/xamp.conf"_s;
    if(!QFile::exists(configFile))
    {
        QString oldConfigFile = QDir::homePath() + u"/.qmmp/qmmp2rc"_s;
        if(!QFile::exists(oldConfigFile))
            oldConfigFile = QDir::homePath() + u"/.qmmp/qmmprc"_s;

        if(QFile::exists(oldConfigFile))
        {
            QFile::copy(oldConfigFile, configFile);
            static const QStringList filesToCopy = {
                u"converterrc"_s,  u"eq.auto_preset"_s, u"history.sqlite"_s, u"library.sqlite"_s,
                u"playlist.txt"_s, u"Songlengths.txt"_s, u"winamp_presets"_s
            };

            for(const QString &name : std::as_const(filesToCopy))
                QFile::copy(QDir::homePath() + u"/.qmmp/"_s + name, Qmmp::configDir() + QLatin1Char('/') + name);

            QProcess::execute(QStringLiteral("cp"), { u"-r"_s, QDir::homePath() + u"/.qmmp/skins"_s, Qmmp::configDir() });
            if(qApp->platformName() == QLatin1String("wayland"))
            {
                //force qsui by default for wayland
                //x-AMP: must target our own settings; with qmmp/qmmp this
                //wrote into the system Qmmp's config
                QSettings settings(QStringLiteral("xamp"), QStringLiteral("xamp"));
                settings.remove("Ui/current_plugin"_L1);
            }
        }
    }
#endif

    //prepare libqmmp and libqmmpui libraries for usage
    m_player = new MediaPlayer(this);
    m_core = SoundCore::instance();

    //additional featuries
    new UiHelper(this);

    //interface
    UiFactory *factory = UiLoader::selected();
    if(factory)
        m_ui = factory->create();
    else
    {
        qCWarning(core, "no user interface found");
        m_finished = true;
        m_exit_code = EXIT_FAILURE;
        return;
    }

#ifdef Q_OS_UNIX
    if(::socketpair(AF_UNIX, SOCK_STREAM, 0, m_sigtermFd))
    {
        qCWarning(core, "couldn't create TERM socketpair");
        m_finished = true;
        m_exit_code = EXIT_FAILURE;
        return;
    }
    m_snTerm = new QSocketNotifier(m_sigtermFd[1], QSocketNotifier::Read, this);
    connect(m_snTerm, &QSocketNotifier::activated, this, &QMMPStarter::handleSigTerm);
#endif

    connect(qApp, &QApplication::aboutToQuit, this, &QMMPStarter::savePosition);
    processCommandArgs(args, QDir::currentPath());
    if(args.isEmpty())
    {
        QSettings settings;
        settings.beginGroup(u"General"_s);
        if(settings.value(u"resume_playback"_s, false).toBool())
        {
            qint64 pos =  settings.value(u"resume_playback_time"_s).toLongLong();
            m_player->playFromPosition(pos);
        }
    }
}

void QMMPStarter::createPaths()
{
    QDir(u"/"_s).mkpath(Qmmp::configDir());
    QDir(u"/"_s).mkpath(Qmmp::cacheDir());
}

void QMMPStarter::savePosition()
{
    QSettings settings;
    settings.beginGroup(u"General"_s);
    settings.setValue(u"resume_playback"_s, m_core->state() == Qmmp::Playing &&
                      QmmpUiSettings::instance()->resumeOnStartup());
    settings.setValue(u"resume_playback_time"_s, m_core->duration() > 0 ? m_core->elapsed() : 0);
    settings.endGroup();
    m_player->stop();
}

void QMMPStarter::commitData(QSessionManager &manager)
{
    if(UiHelper::instance())
        UiHelper::instance()->exit();
#ifndef QT_NO_SESSIONMANAGER
    manager.release();
#endif
}

#ifdef Q_OS_UNIX
void QMMPStarter::handleSigTerm()
{
    qCWarning(core, "processing SIGTERM signal...");
    m_snTerm->setEnabled(false);
    char tmp;
    size_t len = ::read(m_sigtermFd[1], &tmp, sizeof(tmp));
    Q_UNUSED(len);
    UiHelper::instance()->exit();
    m_snTerm->setEnabled(true);
}
#endif

void QMMPStarter::writeCommand()
{
    QString workingDir = QDir::currentPath() + u"|||"_s;
    QByteArray barray;
    barray.append(workingDir.toUtf8 ());
    barray.append(argString.isEmpty() ? "--show-mw" : argString.toUtf8());
    while(!barray.isEmpty())
    {
        qint64 size = m_socket->write(barray);
        barray.remove(0, size);
    }
    m_socket->flush();
    //reading answer
    while(m_socket->waitForReadyRead(1500))
    {
        QString answer = QString::fromUtf8(m_socket->readAll());
        while(answer.endsWith(QChar::LineFeed))
#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
            answer.removeLast();
#else
            answer.truncate(answer.size() - 1);
#endif

        if(!answer.isEmpty())
            cout << qPrintable(answer) << endl;
    }

#ifndef QMMP_NO_CLI
    if (argString.isEmpty())
        printUsage();
#endif

    m_finished = true;
}

void QMMPStarter::readCommand()
{
    QLocalSocket *socket = m_server->nextPendingConnection();
    socket->waitForReadyRead();
    QByteArray inputArray = socket->readAll();
    if(inputArray.isEmpty())
    {
        socket->deleteLater();
        return;
    }
    QStringList slist = QString::fromUtf8(inputArray.data()).split(u"|||"_s, Qt::SkipEmptyParts);
    QString cwd = slist.takeAt(0);
    QString out = processCommandArgs(slist, cwd);
    if(!out.isEmpty())
    {
        //writing answer
        socket->write(out.toUtf8());
        while(socket->waitForBytesWritten())
            socket->flush();
    }
    socket->deleteLater();
}

QString QMMPStarter::processCommandArgs(const QStringList &slist, const QString& cwd)
{
    if(slist.isEmpty())
        return QString();
    QStringList paths;
    for(const QString &arg : std::as_const(slist)) //detect file/directory paths
    {
        if(arg.startsWith(QLatin1Char('-')))
            break;
        paths.append(arg);
    }
    if(!paths.isEmpty())
    {
        return m_option_manager->executeCommand(BuiltinCommandLineOption::OPEN, paths, cwd); //add paths only
    }
    QHash<QString, QStringList> commands = m_option_manager->splitArgs(slist);
    if(commands.isEmpty())
        return QString();

    QString out;
    for(auto it = commands.cbegin(); it != commands.cend(); ++it)
    {
        if(it.key() == "--no-start"_L1 || it.key() == "--ui"_L1)
            continue;

        if (CommandLineManager::hasOption(it.key()))
            return CommandLineManager::executeCommand(it.key(), it.value(), cwd);

        int id = m_option_manager->identify(it.key());
        if(id >= 0)
            out += m_option_manager->executeCommand(id, it.value(), cwd);
        else
            return QString();
    }
    return out;
}

void QMMPStarter::printUsage()
{
//show dialog with command line documentation under ms windows
#ifdef QMMP_NO_CLI
    stringstream tmp_stream;
    tmp_stream.copyfmt(cout);
    streambuf* old_stream = cout.rdbuf(tmp_stream.rdbuf());
#endif
    cout << qPrintable(tr("Usage: xamp [options] [files]")) << endl;
    cout << qPrintable(tr("Options:")) << endl;
    cout << "--------" << endl;
    for(const QString &line : m_option_manager->helpString())
        cout << qPrintable(CommandLineManager::formatHelpString(line)) << endl;
    CommandLineManager::printUsage();
    QStringList extraHelp;
    extraHelp << QStringLiteral("--ui <name>||") + tr("Start x-AMP with the specified user interface");
    extraHelp << QStringLiteral("--ui-list||") + tr("List all available user interfaces");
    extraHelp << QStringLiteral("--no-start||") + tr("Don't start the application");
    extraHelp << QStringLiteral("--debug||") + tr("Print debugging messages");
    extraHelp << QStringLiteral("-h, --help||") + tr("Display this text and exit");
    extraHelp << QStringLiteral("-v, --version||") + tr("Print version number and exit");
    extraHelp << QString();
    //x-AMP: these must not point at the upstream project, or bug reports for
    //this fork end up in Qmmp's tracker
    extraHelp << tr("Home page: %1").arg(u"https://github.com/RavilesX/x-AMP"_s);
    extraHelp << tr("Bug tracker: %1").arg(u"https://github.com/RavilesX/x-AMP/issues"_s);
    extraHelp << tr("Based on Qmmp: %1").arg(u"https://qmmp.ylsoftware.com"_s);
    for(const QString &line : std::as_const(extraHelp))
        cout << qPrintable(CommandLineManager::formatHelpString(line)) << endl;
#ifdef QMMP_NO_CLI
    string text = tmp_stream.str();
    QMessageBox::information(nullptr, tr("Command Line Help"), QString::fromStdString(text));
    cout.rdbuf(old_stream); //restore old stream buffer
#endif
}

void QMMPStarter::printVersion()
{
    //show dialog with qmmp version under ms windows
#ifdef QMMP_NO_CLI
    stringstream tmp_stream;
    tmp_stream.copyfmt(cout);
    streambuf* old_stream = cout.rdbuf(tmp_stream.rdbuf());
#endif
    cout << qPrintable(tr("x-AMP version: %1").arg(Qmmp::strVersion())) << endl;
    cout << qPrintable(tr("Compiled with Qt version: %1").arg(QLatin1StringView(QT_VERSION_STR))) << endl;
    cout << qPrintable(tr("Using Qt version: %1").arg(QString::fromLatin1(qVersion()))) << endl;
#ifdef QMMP_NO_CLI
    string text = tmp_stream.str();
    QMessageBox::information(nullptr, tr("x-AMP Version"), QString::fromStdString(text));
    cout.rdbuf(old_stream); //restore old stream buffer
#endif
}

void QMMPStarter::printUserInterfaces()
{
    //show dialog with qmmp version under ms windows
#ifdef QMMP_NO_CLI
    stringstream tmp_stream;
    tmp_stream.copyfmt(cout);
    streambuf* old_stream = cout.rdbuf(tmp_stream.rdbuf());
#endif
    for(const QString &name : UiLoader::names())
        cout << qPrintable(name) << endl;
#ifdef QMMP_NO_CLI
    string text = tmp_stream.str();
    QMessageBox::information(nullptr, tr("User Interfaces"), QString::fromStdString(text));
    cout.rdbuf(old_stream); //restore old stream buffer
#endif
}
