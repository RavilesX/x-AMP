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

#include <QStringList>
#include <QMessageBox>
#include <xmp.h>
#include "xmpsettingsdialog.h"
#include "decoder_xmp.h"
#include "xmpmetadatamodel.h"
#include "decoderxmpfactory.h"

// DecodeXmpFactory

bool DecoderXmpFactory::canDecode(QIODevice *) const
{
    return false;
}

DecoderProperties DecoderXmpFactory::properties() const
{
    DecoderProperties properties;
    properties.name = tr("XMP Plugin");
    properties.filters = {
        u"*.mod"_s, u"*.m15"_s, u"*.nt"_s, u"*.flx"_s, u"*.wow"_s,
        u"*.dbm"_s, u"*.digi"_s, u"*.emod"_s, u"*.med"_s, u"*.mtn"_s, u"*.okt"_s, u"*.sfx"_s,
        u"*.dtm"_s, u"*.gtk"_s, u"*.mgt"_s,
        u"*.669"_s, u"*.far"_s, u"*.fnk"_s, u"*.imf"_s, u"*.it"_s, u"*.liq"_s, u"*.mdl"_s,
        u"*.mtm"_s, u"*.rtm"_s, u"*.s3m"_s, u"*.stm"_s, u"*.ult"_s, u"*.xm"_s,
        u"*.amf"_s, u"*.gdm"_s, u"*.stx"_s,
        u"*.abk"_s, u"*.amf"_s, u"*.psm"_s, u"*.j2b"_s, u"*.mfp"_s, u"*.smp"_s, u"*.stim"_s, u"*.umx"_s,
        u"*.amd"_s, u"*.rad"_s, u"*.hsc"_s, u"*.s3m"_s,
        u"*.xm"_s, u"*.s3z"_s, u"*.s3r"_s, u"*.s3gz"_s,
        u"*.mdz"_s, u"*.mdr"_s, u"*.mdbz"_s, u"*.mdgz"_s,
        u"*.itz"_s, u"*.itr"_s, u"*.itgz"_s,
        u"*.xmr"_s, u"*.xmgz"_s, u"*.xmz"_s
    };
    properties.description = tr("Module Files");
    //properties.contentType = ;
    properties.shortName = "xmp"_L1;
    properties.hasAbout = true;
    properties.hasSettings = true;
    properties.noInput = true;
    properties.protocols = { u"file"_s };
    return properties;
}

Decoder *DecoderXmpFactory::create(const QString &path, QIODevice *input)
{
    Q_UNUSED(input);
    return new DecoderXmp(path);
}

QList<TrackInfo> DecoderXmpFactory::createPlayList(const QString &path, TrackInfo::Parts parts, QStringList *)
{
    QList<TrackInfo> list;
    TrackInfo info(path);
    if(parts & (TrackInfo::MetaData | TrackInfo::Properties))
    {
        xmp_context ctx = xmp_create_context();
        int error = xmp_load_module(ctx, path.toLocal8Bit().constData());
        if(error != 0)
        {
            qCWarning(plugin, "unable to load module, error = %d", error);
            xmp_free_context(ctx);
            return list;
        }
        xmp_module_info mi;
        xmp_get_module_info(ctx, &mi);
        info.setValue(Qmmp::TITLE, mi.mod->name);
        info.setValue(Qmmp::FORMAT_NAME, mi.mod->type);
        info.setDuration(mi.seq_data[0].duration);
        xmp_release_module(ctx);
        xmp_free_context(ctx);
    }
    list << info;
    return list;
}

MetaDataModel* DecoderXmpFactory::createMetaDataModel(const QString &path, bool readOnly)
{
    Q_UNUSED(readOnly);
    return new XmpMetaDataModel(path);
}

QDialog *DecoderXmpFactory::createSettings(QWidget *parent)
{
    return new XmpSettingsDialog(parent);
}

void DecoderXmpFactory::showAbout(QWidget *parent)
{
    QMessageBox::about(parent, tr("About XMP Audio Plugin"),
                       tr("x-AMP XMP Audio Plugin") + QChar::LineFeed +
                       tr("Written by: Ilya Kotov <forkotov02@ya.ru>") + QChar::LineFeed +
                       tr("Compiled against libxmp-%1").arg(QString::fromLatin1(XMP_VERSION)));
}

QString DecoderXmpFactory::translation() const
{
    return QLatin1String(":/xmp_plugin_");
}
