/***************************************************************************
 *   Copyright (C) 2009-2026 by Ilya Kotov                                 *
 *   forkotov02@ya.ru                                                      *
 *                                                                         *
 *   Copyright (C) 2003-2007 by Justin Karneges and Michail Pishchagin     *
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

#include <QtGlobal>
#ifdef QMMP_WS_X11
#include <QSettings>
#include <QEvent>
#include <QKeyEvent>
#include <QCoreApplication>
#include <QApplication>
#include <QAbstractEventDispatcher>
#include <QHash>
#define Visual XVisual
extern "C" {
#include <X11/X.h>
#include <X11/keysym.h>
#include <X11/XF86keysym.h>
#include <X11/XKBlib.h>
#include <xcb/xcb.h>
}
#undef CursorShape
#undef Status
#undef Bool
#undef None
#undef KeyPress
#undef Visual

#include <qmmp/qmmp.h>
#include <qmmp/soundcore.h>
#include <qmmpui/mediaplayer.h>
#include <qmmpui/uihelper.h>
#include "hotkeymanager.h"

quint32 Hotkey::defaultKey()
{
    return defaultKey(action);
}

quint32 Hotkey::defaultKey(int act)
{
    //default key bindings
    static const QHash<int, quint32> keyMap = {
        { PLAY, 0 },
        { STOP, XF86XK_AudioStop },
        { PAUSE, XF86XK_AudioPause },
        { PLAY_PAUSE, XF86XK_AudioPlay },
        { NEXT, XF86XK_AudioNext },
        { PREVIOUS, XF86XK_AudioPrev },
        { SHOW_HIDE, 0 },
        { VOLUME_UP, XF86XK_AudioRaiseVolume },
        { VOLUME_DOWN, XF86XK_AudioLowerVolume },
        { FORWARD, 0 },
        { REWIND, 0 },
        { JUMP_TO_TRACK, 0},
        { VOLUME_MUTE, XF86XK_AudioMute }
    };
    return keyMap.value(act);
}

HotkeyManager::HotkeyManager(QObject *parent) : QObject(parent)
{
    if(!HotkeyManager::isPlatformX11())
    {
        qCWarning(plugin, "X11 not found. Plugin disabled");
        return;
    }

    QCoreApplication::instance()->installEventFilter(this);
    WId rootWindow = DefaultRootWindow(HotkeyManager::display());
    QSettings settings; //load settings
    settings.beginGroup(u"Hotkey"_s);
    for(int i = Hotkey::PLAY, j = 0; i <= Hotkey::VOLUME_MUTE; ++i, ++j)
    {
        quint32 key = settings.value(QStringLiteral("key_%1").arg(i), Hotkey::defaultKey(i)).toUInt();
        quint32 mod = settings.value(QStringLiteral("modifiers_%1").arg(i), 0).toUInt();

        if(key)
        {
            KeyCode code = XKeysymToKeycode(HotkeyManager::display(), key);
            if(!code)
                continue;

            Hotkey *hotkey = new Hotkey;
            hotkey->action = i;
            hotkey->key = key;
            hotkey->code = XKeysymToKeycode(HotkeyManager::display(), hotkey->key);
            hotkey->mod = mod;
            hotkey->code = code;
            for(long mask_mod : ignModifiersList())
            {
                XGrabKey(HotkeyManager::display(),  hotkey->code, mod | mask_mod, rootWindow, True, GrabModeAsync, GrabModeAsync);
            }
            m_grabbedKeys << hotkey;
        }
    }
    settings.endGroup();
    XSync(HotkeyManager::display(), False);
    qApp->installNativeEventFilter(this);
}

HotkeyManager::~HotkeyManager()
{
    qApp->removeNativeEventFilter(this);
    while(!m_grabbedKeys.isEmpty())
    {
        Hotkey *key = m_grabbedKeys.takeFirst();
        if(key->code)
        {
            for(long mask_mod : ignModifiersList())
                XUngrabKey(HotkeyManager::display(), key->code, key->mod | mask_mod, HotkeyManager::appRootWindow());
        }
        delete key;
    }
}

const QString HotkeyManager::getKeyString(quint32 key, quint32 modifiers)
{
    static const QHash<quint32, QString> modList = {
        { ControlMask, u"Control"_s },
        { ShiftMask, u"Shift"_s },
        { Mod1Mask, u"Alt"_s },
        { Mod2Mask, u"Mod2"_s },
        { Mod3Mask, u"Mod3"_s },
        { Mod4Mask, u"Super"_s },
        { Mod5Mask, u"Mod5"_s }
    };

    QString keyStr;
    for(auto it = modList.cbegin(); it != modList.cend(); ++it)
    {
        if(modifiers & it.key())
            keyStr.append(it.value() + QLatin1Char('+'));
    }
    keyStr.append(QString::fromLatin1(XKeysymToString(key)));
    return keyStr;
}

bool HotkeyManager::nativeEventFilter(const QByteArray &eventType, void *message, qintptr *result)
{
    Q_UNUSED(eventType);
    Q_UNUSED(result);
    xcb_generic_event_t *e = static_cast<xcb_generic_event_t*>(message);

    if(e->response_type == XCB_KEY_PRESS)
    {
        xcb_key_press_event_t *ke = static_cast<xcb_key_press_event_t*>(message);
        quint32 key = keycodeToKeysym(ke->detail);
        quint32 mod = ke->state;
        SoundCore *core = SoundCore::instance();
        MediaPlayer *player = MediaPlayer::instance();
        long ignoredMask = 0;
        for(long mask : ignModifiersList())
            ignoredMask |= mask;

        for(const Hotkey *hotkey : std::as_const(m_grabbedKeys))
        {
            if(hotkey->key != key || (hotkey->mod | ignoredMask) != (mod | ignoredMask))
                continue;
            qCDebug(plugin, "[%s] pressed", qPrintable(getKeyString(key, mod & ~ignoredMask)));

            switch (hotkey->action)
            {
            case Hotkey::PLAY:
                player->play();
                break;
            case Hotkey::STOP:
                player->stop();
                break;
            case Hotkey::PAUSE:
                player->pause();
                break;
            case Hotkey::PLAY_PAUSE:
                if (core->state() == Qmmp::Stopped)
                    player->play();
                else if (core->state() != Qmmp::FatalError)
                    player->pause();
                break;
            case Hotkey::NEXT:
                player->next();
                break;
            case Hotkey::PREVIOUS:
                player->previous();
                break;
            case Hotkey::SHOW_HIDE:
                UiHelper::instance()->toggleVisibility();
                break;
            case Hotkey::VOLUME_UP:
                core->volumeUp();
                break;
            case Hotkey::VOLUME_DOWN:
                core->volumeDown();
                break;
            case Hotkey::FORWARD:
                core->seekRelative(5000);
                break;
            case Hotkey::REWIND:
                core->seekRelative(-5000);
                break;
            case Hotkey::JUMP_TO_TRACK:
                UiHelper::instance()->jumpToTrack();
                break;
            case Hotkey::VOLUME_MUTE:
                SoundCore::instance()->setMuted(!SoundCore::instance()->isMuted());
                break;
            }
        }
    }
    return false;
}

QList<long> HotkeyManager::ignModifiersList()
{
    static const QList<long> ret = { 0, Mod2Mask, LockMask, (Mod2Mask | LockMask) };
    return ret;
}

quint32 HotkeyManager::keycodeToKeysym(quint32 keycode)
{
    return XkbKeycodeToKeysym(HotkeyManager::display(), keycode, 0, 0);
}

Display *HotkeyManager::display()
{
    if(!qApp)
        return nullptr;
    QNativeInterface::QX11Application *app = qApp->nativeInterface<QNativeInterface::QX11Application>();
    if(!app)
        return nullptr;

    return app->display();
}

bool HotkeyManager::isPlatformX11()
{
    return QGuiApplication::platformName() == QLatin1String("xcb");
}

quint32 HotkeyManager::appRootWindow()
{
    if(!qApp)
        return 0;
    QNativeInterface::QX11Application *app = qApp->nativeInterface<QNativeInterface::QX11Application>();
    if(!app)
        return 0;

    xcb_connection_t *conn = app->connection();

    if(!conn)
        return 0;

    xcb_screen_t *scr = HotkeyManager::screenOfDisplay(conn, 0);

    return scr ? scr->root : 0;
}

xcb_screen_t *HotkeyManager::screenOfDisplay(xcb_connection_t *conn, int screen)
{
    xcb_screen_iterator_t iter = xcb_setup_roots_iterator(xcb_get_setup(conn));
    for (; iter.rem; --screen, xcb_screen_next (&iter))
        if (screen == 0)
            return iter.data;

    return nullptr;
}

#include "moc_hotkeymanager.cpp"

#endif
