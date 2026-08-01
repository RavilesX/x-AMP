/***************************************************************************
 *   Copyright (C) 2015-2026 by Ilya Kotov                                 *
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


/*
Syntax:
%p - artist,
%a - album,
%aa - album artist,
%t - title,
%n - track number,
%NN - 2-digit track number,
%I - track index,
%g - genre,
%c - comment,
%C - composer,
%D - disc number,
%f - file name,
%F - full path,
%y - year,
%l - duration,
%I - track index,
%{bitrate} - bitrate,
%{samplerate} - sample rate,
%{channels} - number of channels,
%{samplesize} - bits per sample,
%{format} - format name,
%{decoder} - decoder name,
%{filesize} - file size,
%if(A,B,C) or %if(A&B&C,D,E) - condition,
%dir(n) - name of the directory located on n levels above,
%dir - full path of the parent directory.
*/

#include <QStringList>
#include <QUrl>
#include "metadataformatter.h"

//#define DUMP_NODES

class MetaDataFormatterPrivate
{
    Q_DECLARE_PUBLIC(MetaDataFormatter)
public:

    MetaDataFormatterPrivate(MetaDataFormatter *f) : q_ptr(f)
    {}

private:
    MetaDataFormatter *q_ptr;

    struct Param;

    struct Node
    {
        enum {
            PRINT_TEXT = 0,
            IF_KEYWORD,
            OR_OPERATOR,
            AND_OPERATOR,
            DIR_FUNCTION
        } command;

        QList<Param> params;
    };

    struct Param
    {
        enum {
            FIELD = 0,
            PROPERTY,
            TEXT,
            NUMERIC,
            NODES
        } type;

        //extra fields
        enum
        {
            PATH = Qmmp::DISCNUMBER + 1,
            TWO_DIGIT_TRACK,
            DURATION,
            FILE_NAME,
            TRACK_INDEX
        };

        int field = -1;
        QString text;
        int number = 0;
        QList<Node> children;
    };

    bool parseField(QList<Node> *nodes, QString::const_iterator *i, QString::const_iterator end)
    {
        QString fieldName;
        int field = Qmmp::UNKNOWN;

        //try to find field with 2 symbols
        if((*i) + 1 != end)
        {
            fieldName.append((**i));
            fieldName.append(*((*i)+1));
            field = fieldNames.value(fieldName, Qmmp::UNKNOWN);
        }
        //try to find field with 1 symbol
        if(field == Qmmp::UNKNOWN)
        {
            fieldName.clear();
            fieldName.append((**i));
            field = fieldNames.value(fieldName, Qmmp::UNKNOWN);
        }

        if(field != Qmmp::UNKNOWN)
        {
            Node node;
            node.command = Node::PRINT_TEXT;
            Param param;
            param.type = Param::FIELD;
            param.field = field;
            node.params.append(param);
            nodes->append(node);
            (*i) += fieldName.size() - 1;
            return true;
        }
        return false;
    }

    bool parseProperty(QList<Node> *nodes, QString::const_iterator *i, QString::const_iterator end)
    {
        if((*i) + 1 == end || (*i) + 2 == end)
            return false;

        if((**i) != QLatin1Char('{'))
            return false;

        (*i)++; //skip '{'

        QString propertyName;

        while((*i) != end && (**i) != QLatin1Char('}'))
        {
            propertyName.append((**i));
            (*i)++;
        }

        int field = propertyNames.value(propertyName, Qmmp::UNKNOWN);
        if(field != Qmmp::UNKNOWN)
        {
            Node node;
            node.command = Node::PRINT_TEXT;
            Param param;
            param.type = Param::PROPERTY;
            param.field = field;
            node.params.append(param);
            nodes->append(node);
            return true;
        }
        return false;
    }

    bool parseIf(QList<MetaDataFormatterPrivate::Node> *nodes, QString::const_iterator *i, QString::const_iterator end)
    {
        if((*i) + 1 == end || (*i) + 2 == end)
            return false;

        if((**i) != QLatin1Char('i') || *((*i)+1) != QLatin1Char('f'))
            return false;

        (*i)+=2;

        Node node;
        node.command = Node::IF_KEYWORD;

        int brackets_tracker = 0;
        QString var1, var2, var3;

        enum {
            STARTING = 0,
            READING_VAR1,
            READING_VAR2,
            READING_VAR3,
            FINISHED,

        } state = STARTING;

        bool escaped = false;

        while((*i) != end)
        {
            if((**i) == QLatin1Char('\\'))
            {
                (*i)++;
                escaped = true;
                continue;
            }

            if(escaped) //ignore escaped brackets
            {
                escaped = false;
            }
            else
            {
                if((**i) == QLatin1Char('('))
                {
                    brackets_tracker++;
                    if(state == STARTING)
                    {
                        state = READING_VAR1;
                        (*i)++;
                        continue;
                    }
                }
                else if((**i) == QLatin1Char(')'))
                    brackets_tracker--;
            }

            switch (state)
            {
            case STARTING:
            {
                break;
            }
            case READING_VAR1:
            {
                if((**i) == QLatin1Char(',') && brackets_tracker == 1)
                {
                    state = READING_VAR2;
                    break;
                }
                var1.append((**i));
                break;
            }
            case READING_VAR2:
            {
                if((**i) == QLatin1Char(',') && brackets_tracker == 1)
                {
                    state = READING_VAR3;
                    break;
                }
                var2.append((**i));
                break;
            }
            case READING_VAR3:
            {
                if((**i) == QLatin1Char(')') && brackets_tracker == 0)
                {
                    state = FINISHED;
                    break;
                }
                var3.append((**i));
                break;
            }
            default:
                break;
            }

            if(state == FINISHED)
                break;

            (*i)++;
        }

        if(state != FINISHED)
        {
            qCWarning(core, "syntax error");
            return false;
        }

        Param param1, param2, param3;
        param1.type = Param::NODES, param2.type = Param::NODES, param3.type = Param::NODES;
        param1.children = compile(var1);
        param2.children = compile(var2);
        param3.children = compile(var3);
        node.params << param1 << param2 << param3;
        nodes->append(node);
        return true;
    }

    bool parseDir(QList<MetaDataFormatterPrivate::Node> *nodes, QString::const_iterator *i, QString::const_iterator end)
    {
        if((*i) + 1 == end || (*i) + 2 == end)
            return false;

        if((**i) != QLatin1Char('d') || *((*i)+1) != QLatin1Char('i') || *((*i)+2) != QLatin1Char('r'))
            return false;

        (*i)+=3;

        Node node;
        node.command = Node::DIR_FUNCTION;

        if((*i) == end || (**i) != QLatin1Char('(')) // %dir without params
        {
            (*i)--;
            nodes->append(node);
            return true;
        }

        QString var;

        enum {
            STARTING = 0,
            READING_VAR,
            FINISHED,

        } state = STARTING;

        while((*i) != end)
        {
            if((**i) == QLatin1Char('(') && state == STARTING)
            {
                state = READING_VAR;
                (*i)++;
                continue;
            }

            switch (state)
            {
            case STARTING:
            {
                break;
            }
            case READING_VAR:
            {
                if((**i) == QLatin1Char(')'))
                {
                    state = FINISHED;
                    break;
                }
                var.append((**i));
                break;
            }
            default:
                break;
            }

            if(state == FINISHED)
                break;

            (*i)++;
        }

        if(state != FINISHED)
        {
            qCWarning(core, "syntax error");
            return false;
        }

        Param param;
        param.type = Param::NUMERIC;
        param.number = var.toInt();

        node.params << param;
        nodes->append(node);
        return true;
    }

    void parseText(QList<MetaDataFormatterPrivate::Node> *nodes, QString::const_iterator *i, QString::const_iterator end)
    {
        Node node;
        node.command = Node::PRINT_TEXT;
        Param param;
        param.type = Param::TEXT;
        node.params.append(param);

        while((*i) != end &&  (**i) != QLatin1Char('%'))
        {
            node.params[0].text.append(**i);
            ++(*i);
        }
        (*i)--;

        if(!node.params[0].text.isEmpty())
            nodes->append(node);
    }

    void parseEscape(QList<MetaDataFormatterPrivate::Node> *nodes, QString::const_iterator *i, QString::const_iterator end)
    {
        if((*i) == end)
            return;

        Node node;
        node.command = Node::PRINT_TEXT;
        Param param;
        param.type = Param::TEXT;
        node.params.append(param);
        node.params[0].text.append(**i);
        nodes->append(node);
    }

    QString evalute(const QList<Node> *nodes, const TrackInfo *info, int trackIndex) const
    {
        QString out;
        for(int i = 0; i < nodes->count(); ++i)
        {
            Node node = nodes->at(i);
            if(node.command == Node::PRINT_TEXT)
            {
                Param p = node.params.constFirst();
                out.append(printParam(&p, info, trackIndex));

            }
            else if(node.command == Node::IF_KEYWORD)
            {
                QString var1 = printParam(&node.params[0], info, trackIndex);
                if(var1.isEmpty() || var1 == QLatin1String("0"))
                    out.append(printParam(&node.params[2], info, trackIndex));
                else
                    out.append(printParam(&node.params[1], info, trackIndex));
            }
            else if(node.command == Node::AND_OPERATOR)
            {
                QString var1 = printParam(&node.params[0], info, trackIndex);
                QString var2 = printParam(&node.params[1], info, trackIndex);
                if(!var1.isEmpty() && !var2.isEmpty())
                    out.append(QLatin1Char('1'));
            }
            else if(node.command == Node::OR_OPERATOR)
            {
                QString var1 = printParam(&node.params[0], info, trackIndex);
                if(!var1.isEmpty())
                    out.append(QLatin1Char('1'));
                else
                {
                    QString var2 = printParam(&node.params[1], info, trackIndex);
                    if(!var2.isEmpty())
                        out.append(QLatin1Char('1'));
                }
            }
            else if(node.command == Node::DIR_FUNCTION)
            {
                if(node.params.isEmpty())
                    out.append(info->path().mid(0, info->path().lastIndexOf(QLatin1Char('/'))));
                else
                    out.append(info->path().section(QLatin1Char('/'), -node.params[0].number - 2, -node.params[0].number - 2));
            }
        }
        return out;
    }

    QString printParam(MetaDataFormatterPrivate::Param *p, const TrackInfo *info, int trackIndex) const
    {
        switch (p->type)
        {
        case Param::FIELD:
            return printField(p->field, info, trackIndex);
        case Param::PROPERTY:
            return printProperty(p->field, info);
        case Param::TEXT:
            return p->text;
        case Param::NODES:
            return evalute(&p->children, info, trackIndex);
        default:
            ;
        }
        return QString();
    }

    QString printField(int field, const TrackInfo *info, int trackIndex) const
    {
        if(field >= Qmmp::TITLE && field <= Qmmp::DISCNUMBER)
        {
            if(field == Qmmp::TITLE)
            {
                QString title = info->value(Qmmp::TITLE);
                if(title.isEmpty()) //using file name if title is empty
                {
                    title = info->path().section(QLatin1Char('/'), -1);
                    title = title.left(title.lastIndexOf(QLatin1Char('.')));
                }

                if(title.isEmpty()) //using full path if file name is empty
                    title = info->path();

                return title;
            }
            return info->value((Qmmp::MetaData) field);
        }
        if(field == Param::PATH)
        {
            return info->path();
        }
        if(field == Param::TWO_DIGIT_TRACK)
        {
            return QStringLiteral("%1").arg(info->value(Qmmp::TRACK), 2, QLatin1Char('0'));
        }
        if(field == Param::DURATION)
        {
            return q_ptr->formatDuration(info->duration());
        }
        if(field == Param::FILE_NAME)
        {
            return info->path().section(QLatin1Char('/'), -1);
        }
        if(field == Param::TRACK_INDEX)
        {
            return QString::number(trackIndex + 1);
        }
        return QString();
    }

    QString printProperty(int field, const TrackInfo *info) const
    {
        return info->value(static_cast<Qmmp::TrackProperty>(field));
    }

    QString dumpNode(MetaDataFormatterPrivate::Node node) const
    {
        QString str;
        QStringList params;
        if(node.command == Node::PRINT_TEXT)
            str += u"PRINT_TEXT"_s;
        else if(node.command == Node::IF_KEYWORD)
            str += u"IF_KEYWORD"_s;
        else if(node.command == Node::AND_OPERATOR)
            str += u"AND_OPERATOR"_s;
        else if(node.command == Node::OR_OPERATOR)
            str += u"OR_OPERATOR"_s;
        else if(node.command == Node::DIR_FUNCTION)
            str += u"DIR_FUNCTION"_s;
        str += QLatin1Char('(');
        for(const Param &p : std::as_const(node.params))
        {
            if(p.type == Param::FIELD)
                params.append(QStringLiteral("FIELD:%1").arg(p.field));
            else if(p.type == Param::PROPERTY)
                params.append(QStringLiteral("PROPERTY:%1").arg(p.field));
            else if(p.type == Param::TEXT)
                params.append(QStringLiteral("TEXT:%1").arg(p.text));
            else if(p.type == Param::NUMERIC)
                params.append(QStringLiteral("NUMBER:%1").arg(p.number));
            else if(p.type == Param::NODES)
            {
                QStringList nodeStrList;
                for(const Node &n : std::as_const(p.children))
                {
                    nodeStrList.append(dumpNode(n));
                }
                params.append(QStringLiteral("NODES:%1").arg(nodeStrList.join(QLatin1Char(','))));
            }
        }
        str.append(params.join(QLatin1Char(',')));
        str.append(QLatin1Char(')'));
        return str;
    }

    QList<MetaDataFormatterPrivate::Node> compile(const QString &expr)
    {
        QList<Node> nodes;
        QString::const_iterator i = expr.constBegin();

        while (i != expr.constEnd())
        {
            if((*i) == QLatin1Char('%'))
            {
                ++i;
                if(i == expr.constEnd())
                    continue;

                if(parseDir(&nodes, &i, expr.constEnd()))
                {
                    ++i;
                    continue;
                }

                if(parseField(&nodes, &i, expr.constEnd()))
                {
                    ++i;
                    continue;
                }

                if(parseProperty(&nodes, &i, expr.constEnd()))
                {
                    ++i;
                    continue;
                }

                if(parseIf(&nodes, &i, expr.constEnd()))
                {
                    ++i;
                    continue;
                }
                continue;
            }

            if((*i) == QLatin1Char('&'))
            {
                ++i;
                Node node;
                node.command = Node::AND_OPERATOR;
                nodes.append(node);
            }
            else if((*i) == QLatin1Char('|'))
            {
                ++i;
                Node node;
                node.command = Node::OR_OPERATOR;
                nodes.append(node);
            }
            else if((*i) == QLatin1Char('\\'))
            {
                ++i;
                parseEscape(&nodes, &i, expr.constEnd());
                ++i;
            }
            else
            {
                parseText(&nodes, &i, expr.constEnd());
                ++i;
            }
        }

        //wrap operators
        for(int j = 0; j < nodes.count(); ++j)
        {
            if(nodes[j].command == Node::AND_OPERATOR || nodes[j].command == Node::OR_OPERATOR)
            {
                if(j == 0 || j == nodes.count() - 1)
                {
                    nodes.clear();
                    qCDebug(core) << "syntax error";
                }

                Param p1;
                p1.type = Param::NODES;
                p1.children << nodes.takeAt(j + 1);
                Param p2;
                p2.type = Param::NODES;
                p2.children << nodes.takeAt(j - 1);
                nodes[j - 1].params.append(p1);
                nodes[j - 1].params.append(p2);
                j--;
            }
        }
        return nodes;
    }

    QString pattern;
    QList<MetaDataFormatterPrivate::Node> nodes;
    static const QHash<QString, int> fieldNames;
    static const QHash<QString, int> propertyNames;
};

const QHash<QString, int> MetaDataFormatterPrivate::fieldNames = {
    { QStringLiteral("t"), Qmmp::TITLE },
    { QStringLiteral("p"), Qmmp::ARTIST },
    { QStringLiteral("aa"), Qmmp::ALBUMARTIST },
    { QStringLiteral("a"), Qmmp::ALBUM },
    { QStringLiteral("c"), Qmmp::COMMENT },
    { QStringLiteral("g"), Qmmp::GENRE },
    { QStringLiteral("C"), Qmmp::COMPOSER },
    { QStringLiteral("y"), Qmmp::YEAR },
    { QStringLiteral("n"), Qmmp::TRACK },
    { QStringLiteral("D"), Qmmp::DISCNUMBER },
    { QStringLiteral("F"), Param::PATH },
    { QStringLiteral("NN"), Param::TWO_DIGIT_TRACK },
    { QStringLiteral("l"), Param::DURATION },
    { QStringLiteral("f"), Param::FILE_NAME },
    { QStringLiteral("I"), Param::TRACK_INDEX }
};

const QHash<QString, int> MetaDataFormatterPrivate::propertyNames = {
    { QStringLiteral("bitrate"), Qmmp::BITRATE },
    { QStringLiteral("samplerate"), Qmmp::SAMPLERATE },
    { QStringLiteral("channels"), Qmmp::CHANNELS },
    { QStringLiteral("samplesize"), Qmmp::BITS_PER_SAMPLE },
    { QStringLiteral("format"), Qmmp::FORMAT_NAME },
    { QStringLiteral("decoder"), Qmmp::DECODER },
    { QStringLiteral("filesize"), Qmmp::FILE_SIZE }
};

MetaDataFormatter::MetaDataFormatter(const QString &pattern) :
    d_ptr(new MetaDataFormatterPrivate(this))
{
    if(!pattern.isEmpty())
        setPattern(pattern);
}

MetaDataFormatter::~MetaDataFormatter()
{
    delete d_ptr;
}

void MetaDataFormatter::setPattern(const QString &pattern)
{
    Q_D(MetaDataFormatter);

    if(pattern == d->pattern)
        return;

    d->pattern = pattern;
    d->nodes.clear();

#ifdef DUMP_NODES
    qCDebug(core) << "pattern:" << pattern;
#endif
    d->nodes = d->compile(pattern);
#ifdef DUMP_NODES
    qCDebug(core) << "dump of nodes";
    for(const Node &n : std::as_const(d->m_nodes))
    {
        qCDebug(core) << "=>" << dumpNode(n);
    }
    qCDebug(core) << "MetaDataFormatter: end of dump";
#endif
}

QString MetaDataFormatter::pattern() const
{
    return d_ptr->pattern;
}

QString MetaDataFormatter::format(const PlayListTrack *track) const
{
    Q_D(const MetaDataFormatter);
    return d->evalute(&d->nodes, track, track->trackIndex());
}

QString MetaDataFormatter::format(const TrackInfo &info, int trackIndex) const
{
    Q_D(const MetaDataFormatter);
    return d->evalute(&d->nodes, &info, trackIndex);
}

QString MetaDataFormatter::format(const TrackInfo *info, int trackIndex) const
{
    return format(*info, trackIndex);
}

QString MetaDataFormatter::formatDuration(qint64 duration, bool hideZero, bool showMs)
{
    if(duration <= 0)
    {
        if(hideZero)
            return QString();

        return showMs ? QStringLiteral("0:00.000") : QStringLiteral("0:00");
    }

    QString out;
    qint64 durationInSeconds = duration / 1000;
    if(durationInSeconds >= 3600)
        out = QStringLiteral("%1:%2").arg(durationInSeconds / 3600).arg(durationInSeconds % 3600 / 60, 2, 10, QLatin1Char('0'));
    else
        out = QStringLiteral("%1").arg(durationInSeconds % 3600 / 60);
    out += QStringLiteral(":%1").arg(durationInSeconds % 60, 2, 10, QLatin1Char('0'));
    if(showMs)
        out += QStringLiteral(".%1").arg(duration % 1000, 3, 10, QLatin1Char('0'));
    return out;
}


