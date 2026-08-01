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

#include <QIODevice>
#include <QSettings>
#include <QByteArray>
#include <QBuffer>
#include <QSet>
#include <QHash>
#include <QLocale>
#include <stdlib.h>
#ifdef WITH_LIBRCD
#include <librcd.h>
#endif
#include <qmmp/qmmptextcodec.h>
#include "tagextractor.h"

#define CSTR_TO_QSTR(str,utf) codec->toUnicode(str.toCString(utf)).trimmed()

bool TagExtractor::m_using_rusxmms = false;

TagExtractor::TagExtractor(QIODevice *d) : m_input(d)
{}

TagExtractor::~TagExtractor()
{}

QMap<Qmmp::MetaData, QString> TagExtractor::id3v2tag() const
{
    QByteArray array = m_input->peek(2048);
    int offset = array.indexOf("ID3");
    if (offset < 0)
        return QMap<Qmmp::MetaData, QString>();

    MpegID3v2Tag tag(&array, offset);
    if (tag.isEmpty())
        return QMap<Qmmp::MetaData, QString>();

    QSettings settings;
    settings.beginGroup(u"MPEG"_s);
    QByteArray codecName = settings.value(u"ID3v2_encoding"_s, u"UTF-8"_s).toByteArray();

    if(m_using_rusxmms || codecName.contains("UTF") || codecName.isEmpty())
        codecName = "UTF-8";

    if(!m_using_rusxmms && settings.value("detect_encoding"_L1, false).toBool())
    {
        QByteArray detectedCharset = TagExtractor::detectCharset(&tag);
        if(!detectedCharset.isEmpty())
            codecName = detectedCharset;
    }
    settings.endGroup();

    QmmpTextCodec *codec = new QmmpTextCodec(codecName);

    bool utf = codec->name().contains("UTF");

    QMap<Qmmp::MetaData, QString> tags = {
        { Qmmp::ARTIST, CSTR_TO_QSTR(tag.artist(), utf) },
        { Qmmp::ALBUM, CSTR_TO_QSTR(tag.album(), utf) },
        { Qmmp::COMMENT, CSTR_TO_QSTR(tag.comment(), utf) },
        { Qmmp::GENRE, CSTR_TO_QSTR(tag.genre(), utf) },
        { Qmmp::TITLE, CSTR_TO_QSTR(tag.title(), utf) },
        { Qmmp::YEAR, QString::number(tag.year()) },
        { Qmmp::TRACK, QString::number(tag.track()) },
    };

    if(!tag.frameListMap()["TCOM"].isEmpty())
    {
        TagLib::String composer = tag.frameListMap()["TCOM"].front()->toString();
        tags.insert(Qmmp::COMPOSER, codec->toUnicode(composer.toCString(utf)).trimmed());
    }
    if(!tag.frameListMap()["TPOS"].isEmpty())
    {
        TagLib::String disc = tag.frameListMap()["TPOS"].front()->toString();
        tags.insert(Qmmp::DISCNUMBER, QString::fromLatin1(disc.toCString()).trimmed());
    }

    delete codec;

    return tags;
}

void TagExtractor::setForceUtf8(bool enabled)
{
    m_using_rusxmms = enabled;
}

QByteArray TagExtractor::detectCharset(const TagLib::Tag *tag)
{
    if(tag->title().isLatin1() && tag->album().isLatin1() &&
            tag->artist().isLatin1() && tag->comment().isLatin1())
    {
#ifdef WITH_LIBRCD
        QSet<int> charsets;
        charsets << rcdGetRussianCharset(tag->title().toCString(), 0);
        charsets << rcdGetRussianCharset(tag->artist().toCString(), 0);
        charsets << rcdGetRussianCharset(tag->album().toCString(), 0);
        charsets << rcdGetRussianCharset(tag->comment().toCString(), 0);

        if(charsets.contains(RUSSIAN_CHARSET_WIN))
            return "WINDOWS-1251"_ba;
        if(charsets.contains(RUSSIAN_CHARSET_ALT))
            return "IBM866"_ba;
        if(charsets.contains(RUSSIAN_CHARSET_KOI))
            return "KOI8-R"_ba;
        if(charsets.contains(RUSSIAN_CHARSET_UTF8))
            return "UTF-8"_ba;
        if(charsets.contains(RUSSIAN_CHARSET_LATIN))
            return "ISO-8859-1"_ba;
#else
        return QByteArray();
#endif
    }
    return "UTF-8";
}

QByteArray TagExtractor::charsetForLocale()
{
    static const QHash<QByteArray, QByteArray> charsets = {
        { "af_ZA", "CP1252" }, { "ar_SA", "CP1256" }, { "ar_LB", "CP1256" }, { "ar_EG", "CP1256" }, { "ar_DZ", "CP1256" },
        { "ar_BH", "CP1256" }, { "ar_IQ", "CP1256" }, { "ar_JO", "CP1256" }, { "ar_KW", "CP1256" }, { "ar_LY", "CP1256" },
        { "ar_MA", "CP1256" }, { "ar_OM", "CP1256" }, { "ar_QA", "CP1256" }, { "ar_SY", "CP1256" }, { "ar_TN", "CP1256" },
        { "ar_AE", "CP1256" }, { "ar_YE", "CP1256" }, { "ast_ES", "CP1252"}, { "az_AZ", "CP1251" }, { "az_AZ", "CP1254" },
        { "be_BY", "CP1251" }, { "bg_BG", "CP1251" }, { "br_FR", "CP1252" }, { "ca_ES", "CP1252" }, { "zh_CN", "CP936"  },
        { "zh_TW", "CP950"  }, { "kw_GB", "CP1252" }, { "cs_CZ", "CP1250" }, { "cy_GB", "CP1252" }, { "da_DK", "CP1252" },
        { "de_AT", "CP1252" }, { "de_LI", "CP1252" }, { "de_LU", "CP1252" }, { "de_CH", "CP1252" }, { "de_DE", "CP1252" },
        { "el_GR", "CP1253" }, { "en_AU", "CP1252" }, { "en_CA", "CP1252" }, { "en_GB", "CP1252" }, { "en_IE", "CP1252" },
        { "en_JM", "CP1252" }, { "en_BZ", "CP1252" }, { "en_PH", "CP1252" }, { "en_ZA", "CP1252" }, { "en_TT", "CP1252" },
        { "en_US", "CP1252" }, { "en_ZW", "CP1252" }, { "en_NZ", "CP1252" }, { "es_PA", "CP1252" }, { "es_BO", "CP1252" },
        { "es_CR", "CP1252" }, { "es_DO", "CP1252" }, { "es_SV", "CP1252" }, { "es_EC", "CP1252" }, { "es_GT", "CP1252" },
        { "es_HN", "CP1252" }, { "es_NI", "CP1252" }, { "es_CL", "CP1252" }, { "es_MX", "CP1252" }, { "es_ES", "CP1252" },
        { "es_CO", "CP1252" }, { "es_ES", "CP1252" }, { "es_PE", "CP1252" }, { "es_AR", "CP1252" }, { "es_PR", "CP1252" },
        { "es_VE", "CP1252" }, { "es_UY", "CP1252" }, { "es_PY", "CP1252" }, { "et_EE", "CP1257" }, { "eu_ES", "CP1252" },
        { "fa_IR", "CP1256" }, { "fi_FI", "CP1252" }, { "fo_FO", "CP1252" }, { "fr_FR", "CP1252" }, { "fr_BE", "CP1252" },
        { "fr_CA", "CP1252" }, { "fr_LU", "CP1252" }, { "fr_MC", "CP1252" }, { "fr_CH", "CP1252" }, { "ga_IE", "CP1252" },
        { "gd_GB", "CP1252" }, { "gv_IM", "CP1252" }, { "gl_ES", "CP1252" }, { "he_IL", "CP1255" }, { "hr_HR", "CP1250" },
        { "hu_HU", "CP1250" }, { "id_ID", "CP1252" }, { "is_IS", "CP1252" }, { "it_IT", "CP1252" }, { "it_CH", "CP1252" },
        { "iv_IV", "CP1252" }, { "ja_JP", "CP932"  }, { "kk_KZ", "CP1251" }, { "ko_KR", "CP949"  }, { "ky_KG", "CP1251" },
        { "lt_LT", "CP1257" }, { "lv_LV", "CP1257" }, { "mk_MK", "CP1251" }, { "mn_MN", "CP1251" }, { "ms_BN", "CP1252" },
        { "ms_MY", "CP1252" }, { "nl_BE", "CP1252" }, { "nl_NL", "CP1252" }, { "nl_SR", "CP1252" }, { "nn_NO", "CP1252" },
        { "nb_NO", "CP1252" }, { "pl_PL", "CP1250" }, { "pt_BR", "CP1252" }, { "pt_PT", "CP1252" }, { "rm_CH", "CP1252" },
        { "ro_RO", "CP1250" }, { "ru_RU", "CP1251" }, { "sk_SK", "CP1250" }, { "sl_SI", "CP1250" }, { "sq_AL", "CP1250" },
        { "sr_RS", "CP1251" }, { "sr_RS", "CP1250" }, { "sv_SE", "CP1252" }, { "sv_FI", "CP1252" }, { "sw_KE", "CP1252" },
        { "th_TH", "CP874"  }, { "tr_TR", "CP1254" }, { "tt_RU", "CP1251" }, { "uk_UA", "CP1251" }, { "ur_PK", "CP1256" },
        { "uz_UZ", "CP1251" }, { "uz_UZ", "CP1254" }, { "vi_VN", "CP1258" }, { "wa_BE", "CP1252" }, { "zh_HK", "CP950"  },
        { "zh_SG", "CP936"  }
    };

    static QByteArray foundCharset;

    if(foundCharset.isEmpty())
    {
        QString name = QLocale::system().name();
        foundCharset = charsets.value(name.toLatin1(), "ISO-8859-1");
    }

    return foundCharset;
}

MpegID3v2Tag::MpegID3v2Tag(QByteArray *array, long offset) : TagLib::ID3v2::Tag(),
    m_offset(offset)
{
    m_buf = new QBuffer(array);
    m_buf->open(QIODevice::ReadOnly);
    read();
}

MpegID3v2Tag::~MpegID3v2Tag()
{
    delete m_buf;
}

void MpegID3v2Tag::read()
{
    m_buf->seek(m_offset);
    uint to_read = TagLib::ID3v2::Header::size();
    if(to_read > 2048 - uint(m_offset))
        return;
    header()->setData(TagLib::ByteVector(m_buf->read(to_read).constData(), to_read));
    to_read = header()->tagSize();
    if(!to_read ||  2048 < m_offset + TagLib::ID3v2::Header::size())
        return;
    QByteArray array = m_buf->read(to_read);
    TagLib::ByteVector v(array.data(), array.size());
    parse(v);
}
