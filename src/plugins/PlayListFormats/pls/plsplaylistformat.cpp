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

#include <QtPlugin>
#include <QRegularExpression>
#include <qmmpui/metadataformatter.h>
#include "plsplaylistformat.h"

PlayListFormatProperties PLSPlaylistFormat::properties() const
{
    PlayListFormatProperties p;
    p.filters = QStringList { u"*.pls"_s };
    p.contentTypes = QStringList { u"audio/x-scpls"_s };
    p.shortName = "pls"_L1;
    return p;
}

QList<PlayListTrack *> PLSPlaylistFormat::decode(const QByteArray &contents)
{
    QList<PlayListTrack *> out;
    QStringList splitted = QString::fromUtf8(contents).split(QChar::LineFeed);

    if(splitted.isEmpty())
    {
        qCWarning(plugin, "error parsing PLS format");
        return out;
    }

    if(!splitted.takeFirst().toLower().startsWith(u"[playlist]"_s))
    {
        qCWarning(plugin, "unknown playlist format");
        return out;
    }

    static const QRegularExpression fileRegExp(u"^File(\\d+)=(.+)"_s);
    static const QRegularExpression fullTitleRegExp(u"^Title(\\d+)=(.+) - (.+)"_s);
    static const QRegularExpression titleRegExp(u"^Title(\\d+)=(.+)"_s);
    static const QRegularExpression lengthRegExp(u"^Length(\\d+)=(-{0,1}\\d+)"_s);

    int number = 0;
    bool error = false;

    for(const QString &line : std::as_const(splitted))
    {
        QRegularExpressionMatch match;

        if((match = fileRegExp.match(line)).hasMatch())
        {
            if((number = match.captured(1).toInt()) > 0)
            {
                while(number > out.count())
                    out << new PlayListTrack();
                out[number - 1]->setPath(match.captured(2));
            }
            else
                error = true;
        }
        else if((match = fullTitleRegExp.match(line)).hasMatch())
        {
            if((number = match.captured(1).toInt()) > 0)
            {
                while(number > out.count())
                    out << new PlayListTrack();
                out[number - 1]->setValue(Qmmp::ARTIST, match.captured(2));
                out[number - 1]->setValue(Qmmp::TITLE, match.captured(3));
            }
            else
                error = true;
        }
        else if((match = titleRegExp.match(line)).hasMatch())
        {
            if((number = match.captured(1).toInt()) > 0)
            {
                while(number > out.count())
                    out << new PlayListTrack();
                out[number - 1]->setValue(Qmmp::TITLE, match.captured(2));
            }
            else
                error = true;
        }
        else if((match = lengthRegExp.match(line)).hasMatch())
        {
            if((number = match.captured(1).toInt()) > 0)
            {
                while(number > out.count())
                    out << new PlayListTrack();
                out[number - 1]->setDuration(match.captured(2).toInt() * 1000);
            }
            else
                error = true;
        }

        if(error)
        {
            qCWarning(plugin, "error while parsing line: '%s'", qPrintable(line));
            qDeleteAll(out);
            out.clear();
            break;
        }
    }

    return out;
}

QByteArray PLSPlaylistFormat::encode(const QList<PlayListTrack *> &contents, const QString &path)
{
    Q_UNUSED(path);
    MetaDataFormatter formatter(u"%if(%p,%p - %t,%t)%if(%p|%t,,%f)"_s);
    QStringList out = { QStringLiteral("[playlist]") };
    int counter = 1;
    for(const PlayListTrack *f : std::as_const(contents))
    {
        out.append(QStringLiteral("File%1=%2").arg(counter).arg(f->path()));
        out.append(QStringLiteral("Title%1=%2").arg(counter).arg(formatter.format(f)));
        out.append(QStringLiteral("Length%1=%2").arg(counter).arg(f->duration() / 1000));
        counter++;
    }
    out << QStringLiteral("NumberOfEntries=%1").arg(contents.count());
    out << u"Version=2"_s;
    return out.join(QChar::LineFeed).toUtf8();
}
