/***************************************************************************
 *   Copyright (C) 2013-2026 by Ilya Kotov                                 *
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

#include <QtGlobal>
#ifdef Q_OS_WIN
#include <QSettings>
#include <QApplication>
#include <windows.h>
#include <winuser.h>
#include <commctrl.h>
#include <qmmp/qmmp.h>
#include <qmmp/soundcore.h>
#include <qmmpui/mediaplayer.h>
#include <qmmpui/uihelper.h>
#include "hotkeymanager.h"

static struct
{
    unsigned int key; //virtual key
    unsigned long code; //scan code
    QString name;
} keyMap [] = {
    { VK_LEFT,    0x14b, QString()},
    { VK_UP,      0x148, QString() },
    { VK_RIGHT,   0x14d, QString() },
    { VK_DOWN,    0x150, QString() },
    { VK_PRIOR,   0x149, QString() },
    { VK_NEXT,    0x151, QString() },
    { VK_END,     0x14F, QString() },
    { VK_HOME,    0x147, QString() },
    { VK_INSERT,  0x152, QString() },
    { VK_DELETE,  0x153, QString() },
    { VK_DIVIDE,  0x135, QString() },
    { VK_NUMLOCK, 0x145, QString() },
    { VK_NUMPAD0, 0x52,  QString() },
    { VK_NUMPAD1, 0x4F,  QString() },
    { VK_NUMPAD2, 0x50,  QString() },
    { VK_NUMPAD3, 0x51,  QString() },
    { VK_NUMPAD4, 0x4b,  QString() },
    { VK_NUMPAD6, 0x4d,  QString() },
    { VK_NUMPAD7, 0x47,  QString() },
    { VK_NUMPAD8, 0x48,  QString() },
    { VK_NUMPAD9, 0x49,  QString() },
    { VK_VOLUME_MUTE, 0x120, u"Volume Mute"_s},
    { VK_VOLUME_DOWN, 0x12e, u"Volume Down"_s},
    { VK_VOLUME_UP, 0x130,  u"Volume Up"_s},
    { VK_MEDIA_NEXT_TRACK, 0x119, u"Media Next Track"_s},
    { VK_MEDIA_PREV_TRACK, 0x110, u"Media Previous Track"_s},
    { VK_MEDIA_STOP, 0x124, u"Media Stop"_s},
    { VK_MEDIA_PLAY_PAUSE, 0x122, u"Media Play/Pause"_s}
};

quint32 Hotkey::defaultKey()
{
    return defaultKey(action);
}

quint32 Hotkey::defaultKey(int act)
{
    //default key bindings
    static const QHash<int, quint32> defaultKeys = {
        { PLAY, 0 },
        { STOP, VK_MEDIA_STOP },
        { PAUSE, 0 },
        { PLAY_PAUSE, VK_MEDIA_PLAY_PAUSE },
        { NEXT, VK_MEDIA_NEXT_TRACK },
        { PREVIOUS, VK_MEDIA_PREV_TRACK },
        { SHOW_HIDE, 0 },
        { VOLUME_UP, VK_VOLUME_UP },
        { VOLUME_DOWN, VK_VOLUME_DOWN },
        { FORWARD, 0 },
        { REWIND, 0 },
        { JUMP_TO_TRACK, 0 },
        { VOLUME_MUTE, VK_VOLUME_MUTE },
    };
    return defaultKeys.value(act);
}

HotkeyManager::HotkeyManager(QObject *parent) : QObject(parent)
{
    qApp->installNativeEventFilter(this);
    QSettings settings; //load settings
    settings.beginGroup(u"Hotkey"_s);
    for (int i = Hotkey::PLAY; i <= Hotkey::JUMP_TO_TRACK; ++i)
    {
        quint32 key = settings.value(QStringLiteral("key_%1").arg(i), Hotkey::defaultKey(i)).toUInt();
        quint32 mod = settings.value(QStringLiteral("modifiers_%1").arg(i), 0).toUInt();

        if (key)
        {
            Hotkey *hotkey = new Hotkey;
            hotkey->action = i;
            hotkey->key = key;
            hotkey->code = MapVirtualKey(key, 0);
            hotkey->mod = mod;
            hotkey->id = hotkey->mod^hotkey->key;

            if(RegisterHotKey(nullptr, hotkey->id,  hotkey->mod, hotkey->key))
            {
                qCDebug(plugin, "registered key=0x%x, mod=0x%x", hotkey->key, hotkey->mod);
            }
            else
            {
                hotkey->id = 0;
                qCWarning(plugin, "unable to register key=0x%x, mod=0x%x", hotkey->key, hotkey->mod);
            }

            m_grabbedKeys << hotkey;
        }
    }
    settings.endGroup();
}

HotkeyManager::~HotkeyManager()
{
    qApp->removeNativeEventFilter(this);
    while(!m_grabbedKeys.isEmpty())
    {
        Hotkey *key = m_grabbedKeys.takeFirst ();
        if(key->id)
             UnregisterHotKey(nullptr, key->id);
        delete key;
    }
}

const QString HotkeyManager::getKeyString(quint32 key, quint32 modifiers)
{
    static const QHash<quint32, QString> modList = {
        { MOD_SHIFT,    u"Shift"_s },
        { MOD_CONTROL,  u"Ctrl"_s },
        { MOD_ALT,      u"Alt"_s },
        { MOD_WIN,      u"Win"_s }
    };

    QString keyStr;
    for(auto it = modList.cbegin(); it != modList.cend(); ++it)
    {
        if(modifiers & it.key())
            keyStr.append(it.value() + QLatin1Char('+'));
    }

    if(key == VK_SHIFT || key == VK_CONTROL || key == VK_LWIN || key == VK_RWIN || key == VK_MENU)
        return keyStr;

    LONG lScan = MapVirtualKey(key, 0) << 16;

    for(unsigned int i = 0; i < sizeof(keyMap)/sizeof(keyMap[0]); ++i)
    {
        if(keyMap[i].key == key)
        {
            if(!keyMap[i].name.isEmpty())
            {
                keyStr += keyMap[i].name;
                return keyStr;
            }
            else if(keyMap[i].code & 0x100)
            {
                lScan |= 0x1000000; // set extended bit
                break;
            }
        }
    }

    int nBufferLen = 64;
    std::wstring str;
    int nLen;
    do
    {
        nBufferLen *= 2;
        str.resize(nBufferLen);
        nLen = ::GetKeyNameTextW(lScan, &str[0], nBufferLen);
    }
    while (nLen == nBufferLen);

    keyStr += QString::fromWCharArray(str.c_str());

    return keyStr;
}

QList<long> HotkeyManager::ignModifiersList()
{
    return QList<long>();
}

quint32 HotkeyManager::keycodeToKeysym(quint32 keycode)
{
    //MapVirtualKey does not work with some scan codes
    //using hardcoded key map instead
    for(unsigned int i = 0; i < sizeof(keyMap)/sizeof(keyMap[0]); ++i)
    {
        if(keyMap[i].code == keycode)
            return keyMap[i].key;
    }
    return MapVirtualKey(keycode, 1);
}

bool HotkeyManager::nativeEventFilter(const QByteArray &eventType, void *message, qintptr *result)
{
    Q_UNUSED(eventType);
    Q_UNUSED(result);
    MSG *m = static_cast<MSG*>(message);
    if (m->message == WM_HOTKEY)
    {
        SoundCore *core = SoundCore::instance();
        MediaPlayer *player = MediaPlayer::instance();
        for(const Hotkey *hotkey : std::as_const(m_grabbedKeys))
        {
            if(hotkey->id != m->wParam)
                continue;

            qCDebug(plugin, "[%s] pressed",
                   qPrintable(HotkeyManager::getKeyString(hotkey->key, hotkey->mod)));

            switch (hotkey->action)
            {
            case Hotkey::PLAY:
                player->play();
                break;
            case Hotkey::STOP:
                player->stop();
                break;
            case Hotkey::PAUSE:
                core->pause();
                break;
            case Hotkey::PLAY_PAUSE:
                if (core->state() == Qmmp::Stopped)
                    player->play();
                else if (core->state() != Qmmp::FatalError)
                    core->pause();
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
                core->seek(core->elapsed() + 5000);
                break;
            case Hotkey::REWIND:
                core->seek(qMax(qint64(0), core->elapsed() - 5000));
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

#include "moc_hotkeymanager.cpp"

#endif

