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

#include <QTimer>
#include <QFile>
#include <QDir>
#include <QSettings>
#include <QGuiApplication>
#define Visual VisualQmmp
#include <qmmp/soundcore.h>
#undef Visual

#ifdef X11_FOUND
#include <X11/X.h>
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#undef CursorShape
#elif defined(Q_OS_WIN)
#include <windows.h>
#endif

#include "popupwidget.h"
#include "notifier.h"

Notifier::Notifier(QObject *parent) : QObject(parent)
{
    QSettings settings;
    settings.beginGroup(u"Notifier"_s);
    m_desktop = settings.value(u"song_notification"_s, true).toBool();
    m_resumeNotification = settings.value(u"resume_notification"_s, false).toBool();
    m_showVolume = settings.value(u"volume_notification"_s, true).toBool();
    m_psi = settings.value(u"psi_notification"_s, false).toBool();
    m_disableForFullScreen = settings.value(u"disable_fullscreen"_s, false).toBool();
    settings.endGroup();
    m_core = SoundCore::instance();
    connect(m_core, &SoundCore::trackInfoChanged, this, &Notifier::showMetaData);
    connect(m_core, &SoundCore::stateChanged, this, &Notifier::setState);
    connect(m_core, &SoundCore::volumeChanged, this, &Notifier::showVolume);

    //psi tune files (thousands of them!)
    QString psi_data_dir = QString::fromLocal8Bit(qgetenv("PSIDATADIR"));
    QString xdg_cache_home = QString::fromLocal8Bit(qgetenv("XDG_CACHE_HOME"));
    if(!psi_data_dir.isEmpty())
        m_psiTuneFiles << psi_data_dir + u"/tune"_s;
    else if(!xdg_cache_home.isEmpty())
    {
        m_psiTuneFiles << xdg_cache_home + u"/psi/tune"_s;
        m_psiTuneFiles << xdg_cache_home + u"/psi+/tune"_s;
    }
    else
    {
        m_psiTuneFiles << QDir::homePath() + u"/.cache/psi/tune"_s;
        m_psiTuneFiles << QDir::homePath() + u"/.cache/psi+/tune"_s;
    }
    //legacy psi support
    m_psiTuneFiles << QDir::homePath() + u"/.psi/tune"_s;
    m_psiTuneFiles << QDir::homePath() + u"/.psi-plus/tune"_s;
    m_psiTuneFiles << QDir::homePath() + u"/.cache/Psi+/tune"_s;

    if (m_core->state() == Qmmp::Playing) //test message
        showMetaData();
}

Notifier::~Notifier()
{
    removePsiTuneFiles();
    delete m_popupWidget;
}

void Notifier::setState(Qmmp::State state)
{
    switch ((uint) state)
    {
    case Qmmp::Playing:
    {
        if (m_isPaused)
        {
            showMetaData();
            m_isPaused = false;
        }
        break;
    }
    case Qmmp::Paused:
    {
        if(m_resumeNotification)
            m_isPaused = true;
        break;
    }
    case Qmmp::Stopped:
    {
        m_isPaused = false;
        removePsiTuneFiles();
        break;
    }
    default:
        m_isPaused = false;
    }
}

void Notifier::showMetaData()
{
    if (m_desktop && !hasFullscreenWindow())
    {
        if (!m_popupWidget)
            m_popupWidget = new PopupWidget();
        m_popupWidget->showMetaData();
    }

    if (!m_psi)
        return;

    QByteArray data;
    data.append(m_core->metaData(Qmmp::TITLE).toUtf8() + "\n");
    data.append(m_core->metaData(Qmmp::ARTIST).toUtf8() + "\n");
    data.append(m_core->metaData(Qmmp::ALBUM).toUtf8() + "\n");
    data.append(m_core->metaData(Qmmp::TRACK).toUtf8() + "\n");
    data.append(QString::number(m_core->duration() / 1000).toUtf8() + "\n");

    for(const QString &path : std::as_const(m_psiTuneFiles))
    {
        QDir tuneDir = QFileInfo(path).absoluteDir();
        if(!tuneDir.exists())
            continue;

        QFile file(path);
        if(file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate))
            file.write(data);
    }
}

void Notifier::showVolume(int v)
{
    if((m_volume != v) && m_showVolume)
    {
        if (m_volume >= 0 && !hasFullscreenWindow())
        {
            if (!m_popupWidget)
                m_popupWidget = new PopupWidget();
            m_popupWidget->showVolume(v);
        }
        m_volume = v;
    }
}

void Notifier::removePsiTuneFiles()
{
    if(m_psi) //clear psi notification
    {
        for(const QString &path : std::as_const(m_psiTuneFiles))
            QFile::remove(path);
    }
}

#ifdef X11_FOUND
bool Notifier::hasFullscreenWindow() const
{
    if(!m_disableForFullScreen || !Notifier::isPlatformX11())
        return false;
    Atom type = None;
    int format = 0;
    unsigned long nitems = 0, bytes_after = 0;
    unsigned char *prop;

    Display *display = Notifier::display();

    Atom filter = XInternAtom(display, "_NET_WM_STATE_FULLSCREEN", False);
    Atom net_wm_state = XInternAtom(display, "_NET_WM_STATE", False);

    Window window;
    int ret;
    XGetInputFocus(display, &window, &ret);


    int status = XGetWindowProperty(display, window, net_wm_state, 0, 256,
                                    False, XA_ATOM, &type, &format, &nitems,
                                    &bytes_after, &prop);
    if(status != Success || type == None)
        return false;

    Atom *atoms = (Atom *)prop;

    for (unsigned long i = 0; i < nitems; i++)
    {
        if (atoms[i] == filter)
        {
            XFree(prop);
            return true;
        }
    }
    XFree(prop);
    return false;
}

Display *Notifier::display()
{
    if(!qApp)
        return nullptr;
    QNativeInterface::QX11Application *app = qApp->nativeInterface<QNativeInterface::QX11Application>();
    if(!app)
        return nullptr;

    return app->display();
}

bool Notifier::isPlatformX11()
{
    return QGuiApplication::platformName() == QLatin1String("xcb");
}
#elif defined(Q_OS_WIN)
bool Notifier::hasFullscreenWindow() const
{
    if(!m_disableForFullScreen)
        return false;
    int width = GetSystemMetrics(SM_CXSCREEN);
    int height = GetSystemMetrics(SM_CYSCREEN);

    RECT windowRect;
    GetWindowRect(GetForegroundWindow(), &windowRect);

    return (width == windowRect.right - windowRect.left) && (height == windowRect.bottom - windowRect.top);
}
#else
bool Notifier::hasFullscreenWindow() const
{
    return false;
}
#endif
