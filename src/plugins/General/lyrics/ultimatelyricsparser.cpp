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

#include <QXmlStreamReader>
#include <QFile>
#include <algorithm>
#include <qmmp/qmmp.h>
#include "ultimatelyricsparser.h"

UltimateLyricsParser::UltimateLyricsParser()
{}

UltimateLyricsParser::~UltimateLyricsParser()
{
    qDeleteAll(m_providers);
    m_providers.clear();
}

bool UltimateLyricsParser::load(const QString &path)
{
    qDeleteAll(m_providers);
    m_providers.clear();

    QFile file(path);
    if(!file.open(QIODevice::ReadOnly))
    {
        m_errorString = file.errorString();
        return false;
    }

    QXmlStreamReader reader(&file);

    QString parentElement;
    QList<QPair<QString, QString> > args;

    while(!reader.atEnd())
    {
        reader.readNext();

        if(reader.isStartElement())
        {
            if(reader.name() == QLatin1String("provider"))
            {
                LyricsProvider *provider = new LyricsProvider;
                QXmlStreamAttributes attrs = reader.attributes();
                provider->setName(attrs.value("name"_L1).toString());
                provider->setTitle(attrs.value("title"_L1).toString());
                provider->setUrl(attrs.value("url"_L1).toString());
                m_providers << provider;
            }
            else if(reader.name() == QLatin1String("urlFormat") && !m_providers.isEmpty())
            {
                m_providers.last()->addUrlFormat(reader.attributes().value("replace"_L1).toString(),
                                                 reader.attributes().value("with"_L1).toString());
            }
            else if(reader.name() == QLatin1String("extract") || reader.name() == QLatin1String("exclude"))
            {
                parentElement = reader.name().toString();
            }
            else if(reader.name() == QLatin1String("invalidIndicator") && !m_providers.isEmpty())
            {
                m_providers.last()->addInvalidIndicator(reader.attributes().value("value"_L1).toString());
            }
            else if(reader.name() == QLatin1String("item"))
            {
                QXmlStreamAttributes attrs = reader.attributes();
                QString arg1, arg2;
                if(attrs.hasAttribute("begin"_L1) && attrs.hasAttribute("end"_L1))
                {
                    arg1 = attrs.value("begin"_L1).toString();
                    arg2 = attrs.value("end"_L1).toString();
                }
                else if(attrs.hasAttribute("tag"_L1))
                {
                    arg1 = attrs.value("tag"_L1).toString();
                }
                else if(attrs.hasAttribute("url"_L1))
                {
                    arg1 = attrs.value("url"_L1).toString();
                }
                args << qMakePair(arg1, arg2);
            }
        }
        else if(reader.isEndElement())
        {
            if(reader.name() == QLatin1String("extract") || reader.name() == QLatin1String("exclude"))
            {
                parentElement.clear();
                m_providers.last()->addRule(args, reader.name() == QLatin1String("exclude"));
                args.clear();
            }
        }

        if(reader.hasError())
        {
            m_errorString = tr("%1 (line: %2)").arg(reader.errorString()).arg(reader.lineNumber());
            return false;
        }
    }
    return true;
}

QString UltimateLyricsParser::errorString() const
{
    return m_errorString;
}

QList<LyricsProvider *> UltimateLyricsParser::providers()
{
    return m_providers;
}

LyricsProvider *UltimateLyricsParser::provider(const QString &name) const
{
    auto it = std::find_if(m_providers.cbegin(), m_providers.cend(), [name](LyricsProvider *provider){ return provider->name() == name; });
    return it == m_providers.cend() ? nullptr : *it;
}

const QStringList &UltimateLyricsParser::defaultProviders()
{
    static const QStringList out = {
        u"letras.mus.br"_s,
        u"darklyrics.com"_s,
        u"lyricsmania.com"_s
    };

    return out;
}
