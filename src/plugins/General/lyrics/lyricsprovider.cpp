/***************************************************************************
 *   Copyright (C) 2019-2026 by Ilya Kotov                                 *
 *   forkotov02@ya.ru                                                      *
 *                                                                         *
 *   Based on Amarok 2 Ultimate Lyrics script                              *
 *   Copyright (C) 2009-2010 Vladimir Brkic <vladimir_brkic@yahoo.com>     *
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

#include <QRegularExpression>
#include <qmmp/trackinfo.h>
#include "lyricsprovider.h"

LyricsProvider::LyricsProvider()
{}

void LyricsProvider::setName(const QString &name)
{
    m_name = name;
}

void LyricsProvider::setTitle(const QString &title)
{
    m_title = title;
}

void LyricsProvider::setUrl(const QString &url)
{
    m_url = url;
}

void LyricsProvider::addUrlFormat(const QString &replace, const QString &with)
{
    m_urlFormats << UrlFormat{ .replace = replace, .with = with };
}

void LyricsProvider::addRule(const QList<QPair<QString, QString> > &args, bool exclude)
{
    Rule rule;
    for(const QPair<QString, QString> &i : std::as_const(args))
    {
        Item item;
        if(!i.first.isEmpty() && !i.second.isEmpty())
        {
            item.begin = i.first;
            item.end = i.second;
        }
        else if(i.first.contains(u"://"_s)) //url
        {
            item.url = i.first;
        }
        else
        {
            item.tag = i.first;
        }
        rule << item;
    }
    if(exclude)
        m_excludeRules << rule;
    else
        m_extractRules << rule;
}

void LyricsProvider::addInvalidIndicator(const QString &indicator)
{
    m_invalidIndicators << indicator;
}

QString LyricsProvider::getUrl(const TrackInfo &track) const
{
    QString url = m_url;
    const QHash<QString, QString> replaceHash = generateReplaceHash(track);
    QHash<QString, QString>::const_iterator it = replaceHash.constBegin();
    while(it != replaceHash.constEnd())
    {
        QString value = it.value();

        for(const UrlFormat &format: m_urlFormats)
            value.replace(QRegularExpression(QStringLiteral("[%1]").arg(QRegularExpression::escape(format.replace))), format.with);

        url.replace(it.key(), value);

        ++it;
    }

    return url;
}

QString LyricsProvider::format(const QByteArray &data, const TrackInfo &track) const
{
    const QString content = QString::fromUtf8(data);
    QString out;

    for(const QString &indicator : std::as_const(m_invalidIndicators))
    {
        if(content.contains(indicator))
            return QString();
    }

    if(m_skipRules)
        return content;

    const QHash<QString, QString> replaceHash = generateReplaceHash(track);

    for(const Rule &rule : std::as_const(m_extractRules))
    {
        Rule tmpRule = rule;
        QString tmpContent = content;

        for(Item &item : tmpRule)
        {
            QHash<QString, QString>::const_iterator it = replaceHash.constBegin();
            while(it != replaceHash.constEnd())
            {
                item.begin.replace(it.key(), it.value());
                item.url.replace(it.key(), it.value());
                ++it;
            }
        }

        tmpContent = extract(tmpContent, tmpRule);

        if(!tmpContent.isEmpty())
        {
            for(const Rule &excludeRule : std::as_const(m_excludeRules))
                tmpContent = exclude(tmpContent, excludeRule);
        }
        if(tmpContent.isEmpty())
        {
            tmpContent = content;
        }
        else
        {
            out = tmpContent;
            break;
        }
    }

    while (out.endsWith(u"<br />"_s))
    {
        out.chop(6);
        out = out.trimmed();
    }

    while (out.endsWith(u"<br>"_s))
    {
        out.chop(4);
        out = out.trimmed();
    }

    //plain text support
    if(out.contains(QChar::LineFeed) && !out.contains(QLatin1Char('<')))
        out.replace(QChar::LineFeed, QStringLiteral("<br>"));

    return out;
}

const QString &LyricsProvider::name() const
{
    return m_name;
}

void LyricsProvider::skipRules(bool skip)
{
    m_skipRules = skip;
}

QString LyricsProvider::fixCase(const QString &title) const
{
    QString out;
    QString::const_iterator it = title.constBegin();
    while (it != title.constEnd())
    {
        if(it == title.constBegin() || (it - 1)->isSpace())
            out.append(it->toUpper());
        else
            out.append(*it);

        ++it;
    }

    return out;
}

QHash<QString, QString> LyricsProvider::generateReplaceHash(const TrackInfo &track) const
{
    const QHash<QString, QString> replaceMap = {
        { u"{artist}"_s, track.value(Qmmp::ARTIST).toLower() },
        { u"{artist2}"_s, track.value(Qmmp::ARTIST).toLower().remove(QChar::Space) },
        { u"{Artist}"_s, track.value(Qmmp::ARTIST) },
        { u"{ARTIST}"_s, track.value(Qmmp::ARTIST).toUpper() },
        { u"{a}"_s, track.value(Qmmp::ARTIST).left(1).toLower() },
        { u"{album}"_s, track.value(Qmmp::ALBUM).toLower() },
        { u"{album2}"_s, track.value(Qmmp::ALBUM).toLower().remove(QChar::Space) },
        { u"{Album}"_s, track.value(Qmmp::ALBUM) },
        { u"{title}"_s,  track.value(Qmmp::TITLE).toLower() },
        { u"{Title}"_s, track.value(Qmmp::TITLE) },
        { u"{Title2}"_s, fixCase(track.value(Qmmp::TITLE)) },
        { u"{track}"_s, track.value(Qmmp::TRACK) },
        { u"{year}"_s,  track.value(Qmmp::YEAR) }
    };

    return replaceMap;
}

QString LyricsProvider::extract(const QString &content, const Rule &rule) const
{
    QString out = content;

    for(const Item &item : std::as_const(rule))
    {
        if(!item.url.isEmpty())
        {
            QString url = item.url;
            QString id = rule.count() >= 2 ? out.section(rule[1].begin, 1).section(rule[1].end, 0, 0) : QString();
            url.replace(u"{id}"_s, id);
            return url;
        }

        if(!item.tag.isEmpty())
        {
            static const QRegularExpression re(u"<(\\w+).*>"_s);
            QRegularExpressionMatch m = re.match(item.tag);
            out = out.section(item.tag, 1).section(QStringLiteral("</%1>").arg(m.captured(1)), 0, 0);
        }
        else
        {
            out = out.section(item.begin, 1).section(item.end, 0, 0);
        }
    }
    return out.trimmed();
}

QString LyricsProvider::exclude(const QString &content, const LyricsProvider::Rule &rule) const
{
    QString out = content;

    for(const Item &item : std::as_const(rule))
    {
        if(!item.tag.isEmpty())
        {
            static const QRegularExpression re(u"<(\\w+).*>"_s);
            QRegularExpressionMatch m = re.match(item.tag);
            out = out.section(item.tag, 0, 0) + out.section(item.tag, 1).section(QStringLiteral("</%1>").arg(m.captured(1)), 1);
        }
        else
        {
            QString prev;
            while(prev != out)
            {
                prev = out;
                out = out.section(item.begin, 0, 0) + out.section(item.begin, 1).section(item.end, 1);
            }
        }
    }
    return out.trimmed();
}
