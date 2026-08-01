/***************************************************************************
 *   Copyright (C) 2021-2026 by Ilya Kotov                                 *
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

#include <iconv.h>
#include <errno.h>
#include "qmmp.h"
#include "qmmptextcodec.h"

class QmmpTextCodecPrivate
{
public:
    QmmpTextCodecPrivate(const QByteArray &name) : name(name) {}

    ~QmmpTextCodecPrivate()
    {
        if(to)
            iconv_close(to);
        if(from)
            iconv_close(from);
    }

    iconv_t to = nullptr, from = nullptr;
    QByteArray name;
};

QmmpTextCodec::QmmpTextCodec(const QByteArray &charset) :
    d_ptr(new QmmpTextCodecPrivate(charset.toUpper()))
{
    Q_D(QmmpTextCodec);

    if(d->name == "UTF-8"_ba || d->name == "UTF-16"_ba)
        return;

    if((d->to = iconv_open(d->name.constData(), "UTF-16")) == (iconv_t)(-1))
    {
        qCWarning(core, "error: %s", strerror(errno));
        d->to = nullptr;
    }

    if((d->from = iconv_open("UTF-16", d->name.constData())) == (iconv_t)(-1))
    {
        qCWarning(core, "error: %s", strerror(errno));
        d->from = nullptr;
    }
}

QmmpTextCodec::~QmmpTextCodec()
{
    delete d_ptr;
}

QByteArray QmmpTextCodec::name() const
{
    return d_ptr->name;
}

QString QmmpTextCodec::toUnicode(const QByteArray &a) const
{
    Q_D(const QmmpTextCodec);

    if(d->name == "UTF-16"_ba)
        return QString::fromUtf16(reinterpret_cast<const char16_t *>(a.data()), a.size() / 2);
    if(!d->from || d->name == "UTF-8"_ba)
        return QString::fromUtf8(a);

    size_t inBytesLeft = 0;
    size_t outBytesLeft = 0;

    // reset state
    iconv(d->from, nullptr, &inBytesLeft, nullptr, &outBytesLeft);

    char *inBytes = const_cast<char *>(a.data());
    inBytesLeft = a.size();
    outBytesLeft = a.size() * 2 + 2;
    QByteArray ba(outBytesLeft, Qt::Uninitialized);
    char *outBytes = ba.data();
    outBytesLeft = ba.size();

    while(inBytesLeft > 0)
    {
        size_t ret = iconv(d->from,  &inBytes, &inBytesLeft, &outBytes, &outBytesLeft);

        if(ret == (size_t) -1)
        {
            if(errno == E2BIG) //increase buffer size
            {
                int offset = ba.size() - outBytesLeft;
                ba.resize(ba.size() * 2);
                outBytes = ba.data() + offset;
                outBytesLeft = ba.size() - offset;
                continue;
            }

            if(errno == EINVAL)
                break;

            if(errno == EILSEQ)
            {
                // skip the next character
                ++inBytes;
                --inBytesLeft;
                continue;
            }

            //fallback
            return QString::fromLatin1(a);
        }
    }

    return QString::fromUtf16(reinterpret_cast<const char16_t *>(ba.constData()), (ba.size() - outBytesLeft) / 2);
}

QString QmmpTextCodec::toUnicode(const char *chars) const
{
    return toUnicode(QByteArray(chars));
}

QByteArray QmmpTextCodec::fromUnicode(const QString &str) const
{
    Q_D(const QmmpTextCodec);

    if(d->name == "UTF-16"_ba)
        return QByteArray(reinterpret_cast<const char*>(str.utf16()), str.size() * 2);
    if(!d->from || d->name == "UTF-8"_ba)
        return str.toUtf8();

    size_t inBytesLeft = 0;
    size_t outBytesLeft = 0;

    // reset state
    iconv(d->to, nullptr, &inBytesLeft, nullptr, &outBytesLeft);

    char *inBytes =  const_cast<char *>(reinterpret_cast<const char*>(str.utf16()));
    inBytesLeft = str.size() * 2;
    outBytesLeft = str.size() * 2;

    QByteArray ba(outBytesLeft, Qt::Uninitialized);
    char *outBytes = ba.data();
    outBytesLeft = ba.size();

    while(inBytesLeft > 0)
    {
        size_t ret = iconv(d->to,  &inBytes, &inBytesLeft, &outBytes, &outBytesLeft);

        if(ret == (size_t) -1)
        {
            if(errno == E2BIG) //increase buffer size
            {
                int offset = ba.size() - outBytesLeft;
                ba.resize(ba.size() * 2);
                outBytes = ba.data() + offset;
                outBytesLeft = ba.size() - offset;
                continue;
            }

            if(errno == EINVAL)
                break;

            if(errno == EILSEQ)
            {
                // skip the next character
                ++inBytes;
                --inBytesLeft;
                continue;
            }

            //fallback
            return str.toLatin1();
        }
    }

    ba.resize(ba.size() - outBytesLeft);

    return ba;
}

QStringList QmmpTextCodec::availableCharsets()
{
    static const QStringList charsets = {
        u"BIG5"_s,
        u"EUC-JP"_s,
        u"EUC-KR"_s,
        u"GB18030"_s,
        u"GBK"_s,
        u"IBM866"_s,
        u"ISO-2022-JP"_s,
        u"ISO-8859-10"_s,
        u"ISO-8859-13"_s,
        u"ISO-8859-14"_s,
        u"ISO-8859-15"_s,
        u"ISO-8859-16"_s,
        u"ISO-8859-1"_s,
        u"ISO-8859-2"_s,
        u"ISO-8859-3"_s,
        u"ISO-8859-4"_s,
        u"ISO-8859-5"_s,
        u"ISO-8859-6"_s,
        u"ISO-8859-7"_s,
        u"ISO-8859-8"_s,
        u"ISO-8859-8-I"_s,
        u"KOI8-R"_s,
        u"KOI8-U"_s,
        u"MACINTOSH"_s,
        u"SHIFT_JIS"_s,
        u"UTF-32"_s,
        u"UTF-32LE"_s,
        u"UTF-32BE"_s,
        u"UTF-16"_s,
        u"UTF-16LE"_s,
        u"UTF-16BE"_s,
        u"UTF-8"_s,
        u"WINDOWS-1250"_s,
        u"WINDOWS-1251"_s,
        u"WINDOWS-1252"_s,
        u"WINDOWS-1253"_s,
        u"WINDOWS-1254"_s,
        u"WINDOWS-1255"_s,
        u"WINDOWS-1256"_s,
        u"WINDOWS-1257"_s,
        u"WINDOWS-1258"_s,
        u"WINDOWS-874"_s
    };

    return charsets;
}
