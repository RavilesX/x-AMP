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

#include <QXmlStreamReader>
#include <QXmlStreamWriter>
#include <QFileInfo>
#include <QUrl>
#include <QtPlugin>
#include <qmmp/qmmp.h>
#include "xspfplaylistformat.h"

// Needs more work - it's better use libSpiff there and put it as plugin.

PlayListFormatProperties XSPFPlaylistFormat::XSPFPlaylistFormat::properties() const
{
    PlayListFormatProperties p;
    p.filters = QStringList { u"*.xspf"_s };
    p.shortName = "xspf"_L1;
    p.contentTypes = QStringList { u"application/xspf+xml"_s };
    return p;
}

QList<PlayListTrack*> XSPFPlaylistFormat::decode(const QByteArray &contents)
{
    QList<PlayListTrack*> out;
    QString currentTag;
    QString contents_copy = QString::fromUtf8(contents);

    //remove control symbols to avoid xml errors
    for(int i = 0; i < contents_copy.size(); ++i)
    {
        if(contents_copy[i] <= QChar(0x1F))
        {
            contents_copy.replace(i, 1, QString());
            i--;
        }
    }

    QXmlStreamReader xml(contents_copy);
    while(!xml.atEnd() || !xml.hasError())
    {
        xml.readNext();
        if (xml.isStartElement())
        {
            currentTag = xml.name().toString();
            if(currentTag == "track"_L1)
                out << new PlayListTrack();
        }
        else if (xml.isCharacters() && !xml.isWhitespace())
        {
            if(out.isEmpty())
                continue;

            if(currentTag == "location"_L1)
            {
                QUrl url(xml.text().toString());
                if (url.scheme() == "file"_L1)  //remove scheme for local files only
                    out.last()->setPath(QUrl::fromPercentEncoding(url.toString().toLatin1()).remove(u"file://"_s));
                else
                    out.last()->setPath(QUrl::fromPercentEncoding(url.toString().toLatin1()));
            }
            else if(currentTag == "title"_L1)
            {
                out.last()->setValue(Qmmp::TITLE, xml.text().toString());
            }
            else if(currentTag == "creator"_L1)
            {
                out.last()->setValue(Qmmp::ARTIST, xml.text().toString());
            }
            else if(currentTag == "annotation"_L1)
            {
                out.last()->setValue(Qmmp::COMMENT, xml.text().toString());
            }
            else if(currentTag == "album"_L1)
            {
                out.last()->setValue(Qmmp::ALBUM, xml.text().toString());
            }
            else if(currentTag == "meta"_L1 && xml.attributes().value("rel"_L1) == QLatin1String("year"))
            {
                out.last()->setValue(Qmmp::YEAR, xml.text().toString());
            }
            else
                xml.skipCurrentElement();
        }
    }

    if(xml.hasError())
    {
        qCDebug(plugin, "parse error: %s (row:%lld, col:%lld",
               qPrintable(xml.errorString()), xml.lineNumber(), xml.columnNumber());
    }
    return out;
}

// Needs more work - it's better use libSpiff there and put it as plugin.

QByteArray XSPFPlaylistFormat::encode(const QList<PlayListTrack*> &files, const QString &path)
{
    QString xspfDir = QFileInfo(path).canonicalPath();
    QByteArray out;
    QXmlStreamWriter xml(&out);
    xml.setAutoFormatting(true);
    xml.writeStartDocument();
    xml.writeStartElement("playlist"_L1);
    xml.writeAttribute("version"_L1, "1"_L1);
    xml.writeAttribute("xmlns"_L1, u"http://xspf.org/ns/0/"_s);
    xml.writeTextElement("creator"_L1, u"qmmp-"_s + Qmmp::strVersion());
    xml.writeStartElement("trackList"_L1);

    int counter = 1;
    for(PlayListTrack *f : std::as_const(files))
    {
        xml.writeStartElement("track"_L1);

        QString url;
        if (f->path().contains(u"://"_s))
        {
            url = QString::fromLatin1(QUrl::toPercentEncoding(f->path(), ":/"));
        }
        else if(f->path().startsWith(xspfDir)) //relative path
        {
            QString p = f->path();
            p.remove(0, xspfDir.size());
            if(p.startsWith(QLatin1Char('/')))
                p.remove(0, 1);
            url = QString::fromLatin1(QUrl::toPercentEncoding(p, ":/"));
        }
        else  //absolute path
        {
            url = QString::fromLatin1(QUrl::toPercentEncoding(u"file://"_s + f->path(), ":/"));
        }

        xml.writeTextElement("location"_L1, url);
        xml.writeTextElement("title"_L1, f->value(Qmmp::TITLE));
        xml.writeTextElement("creator"_L1, f->value(Qmmp::ARTIST));
        xml.writeTextElement("annotation"_L1, f->value(Qmmp::COMMENT));
        xml.writeTextElement("album"_L1, f->value(Qmmp::ALBUM));
        xml.writeTextElement("trackNum"_L1, QString::number(counter));

        xml.writeStartElement("meta"_L1);
        xml.writeAttribute("rel"_L1, u"year"_s);
        xml.writeCharacters(f->value(Qmmp::YEAR));
        xml.writeEndElement(); // meta
        xml.writeEndElement(); // track

        counter ++;
    }
    xml.writeEndElement(); //trackList
    xml.writeEndElement(); //playlist
    xml.writeEndDocument();
    return out;
}
