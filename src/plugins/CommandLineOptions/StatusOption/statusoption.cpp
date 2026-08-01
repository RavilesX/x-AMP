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
#include <QMap>
#include <qmmp/soundcore.h>
#include <qmmpui/metadataformatter.h>
#include "statusoption.h"

void StatusOption::registerOprions()
{
    registerOption(STATUS, u"--status"_s, tr("Print playback status"));
    registerOption(NOW_PLAYING, u"--nowplaying"_s, tr("Print formatted track name (example: qmmp --nowplaying \"%t - %a\")"), { u"fmt"_s });
    registerOption(NOW_PLAYING_SYNTAX, u"--nowplaying-syntax"_s, tr("Print --nowplaying syntax"));

    setOptionFlags(NOW_PLAYING_SYNTAX, NoStart);
}

QString StatusOption::shortName() const
{
    return QLatin1String("StatusOption");
}

QString StatusOption::translation() const
{
    return QLatin1String(":/status_plugin_");
}

QString StatusOption::executeCommand(int id, const QStringList &args, const QString &cwd)
{
    Q_UNUSED(cwd);
    SoundCore *core = SoundCore::instance();
    QString out;
    switch (id)
    {
    case STATUS:
    {
        QMap<int, QString> state_names;
        state_names.insert(Qmmp::Playing, u"[playing]"_s);
        state_names.insert(Qmmp::Paused, u"[paused]"_s);
        state_names.insert(Qmmp::Stopped, u"[stopped]"_s);
        state_names.insert(Qmmp::Buffering, u"[buffering]"_s);
        state_names.insert(Qmmp::NormalError, u"[error]"_s);
        state_names.insert(Qmmp::FatalError, u"[error]"_s);
        out += state_names[core->state()];

        if(core->state() == Qmmp::Playing || core->state() == Qmmp::Paused)
        {
            out += QChar::Space;
            out += genProgressBar() + QChar::LineFeed;
            out += u"ARTIST = %p\n"_s;
            out += u"ALBUMARTIST = %aa\n"_s;
            out += u"TITLE = %t\n"_s;
            out += u"ALBUM = %a\n"_s;
            out += u"COMMENT = %c\n"_s;
            out += u"GENRE = %g\n"_s;
            out += u"YEAR = %y\n"_s;
            out += u"TRACK = %n\n"_s;
            out += u"FILE = %f"_s;
            MetaDataFormatter formatter(out);
            out = formatter.format(core->trackInfo());
        }
        out += QChar::LineFeed;
    }
        break;
    case NOW_PLAYING:
    {
        QString t = args.join(QChar::Space);
        MetaDataFormatter formatter(t);
        out = formatter.format(core->trackInfo());
        out += QChar::LineFeed;
    }
        break;
    case NOW_PLAYING_SYNTAX:
    {
        out += tr("Syntax:") + QChar::LineFeed;
        out += tr("%p - artist") + QChar::LineFeed;
        out += tr("%a - album") + QChar::LineFeed;
        out += tr("%aa - album artist") + QChar::LineFeed;
        out += tr("%t - title") + QChar::LineFeed;
        out += tr("%n - track") + QChar::LineFeed;
        out += tr("%NN - 2-digit track") + QChar::LineFeed;
        out += tr("%g - genre") + QChar::LineFeed;
        out += tr("%c - comment") + QChar::LineFeed;
        out += tr("%C - composer") + QChar::LineFeed;
        out += tr("%D - disc number") + QChar::LineFeed;
        out += tr("%f - file name") + QChar::LineFeed;
        out += tr("%F - full path") + QChar::LineFeed;
        out += tr("%y - year") + QChar::LineFeed;
        out += tr("%l - duration") + QChar::LineFeed;
        out += tr("%{bitrate} - bitrate") + QChar::LineFeed;
        out += tr("%{samplerate} - sample rate") + QChar::LineFeed;
        out += tr("%{channels} - number of channels") + QChar::LineFeed;
        out += tr("%{samplesize} - bits per sample") + QChar::LineFeed;
        out += tr("%{format} - format name") + QChar::LineFeed;
        out += tr("%{decoder} - decoder name") + QChar::LineFeed;
        out += tr("%{filesize} - file size") + QChar::LineFeed;
        out += tr("%if(A&B&C,D,E) - condition") + QChar::LineFeed;
        out += tr("%dir(n) - directory name located on n levels above");
    }
        break;
    default:
        ;
    }

    return out;
}

QString StatusOption::genProgressBar()
{
    SoundCore *core = SoundCore::instance();
    QString totalTime = QStringLiteral("%1:%2").arg(core->duration() / 60000)
            .arg(core->duration() % 60000 / 1000, 2, 10, QLatin1Char('0'));
    QString currentTime = QStringLiteral("%1:%2").arg(core->elapsed() / 60000)
            .arg(core->elapsed() % 60000 / 1000, 2, 10, QLatin1Char('0'));
    QString out = currentTime;
    if(core->duration())
    {
        out.clear();
        int played_count = 22 * (double)core->elapsed() / core->duration();
        for(int i = 0; i < played_count; ++i)
            out += QLatin1Char('=');
        out += QLatin1Char('#');
        for(int i = played_count; i < 22; ++i)
            out += QLatin1Char('-');
        out += QChar::Space + currentTime + QLatin1Char('/') + totalTime;
    }
    return out;
}
