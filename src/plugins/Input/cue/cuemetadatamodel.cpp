/***************************************************************************
 *   Copyright (C) 2009-2026 by Ilya Kotov                                 *
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

#include <QFileInfo>
#include <QSettings>
#include <qmmp/metadatamanager.h>
#include <qmmp/qmmptextcodec.h>
#ifdef WITH_ENCA
#include <enca.h>
#endif
#include "cuefile.h"
#include "cuemetadatamodel.h"

CUEMetaDataModel::CUEMetaDataModel(bool readOnly, const QString &url) : MetaDataModel(readOnly, IsCueEditable)
{
    CueFile file(url);
    if(file.isEmpty())
    {
        qCWarning(plugin, "invalid cue file");
        return;
    }
    int track = url.section(QLatin1Char('#'), -1).toInt();
    m_dataFilePath = file.dataFilePath(track);
    m_cueFilePath = file.cueFilePath();
}

CUEMetaDataModel::~CUEMetaDataModel()
{
    if(m_codec)
    {
        delete m_codec;
        m_codec = nullptr;
    }
}

QList<MetaDataItem> CUEMetaDataModel::extraProperties() const
{
    QList<MetaDataItem> ep;
    MetaDataModel *model = MetaDataManager::instance()->createMetaDataModel(m_dataFilePath, true);
    if(model)
    {
        ep = model->extraProperties();
        delete model;
    }
    return ep;
}

QString CUEMetaDataModel::coverPath() const
{
    return MetaDataManager::instance()->findCoverFile(m_dataFilePath);
}

QString CUEMetaDataModel::cue() const
{
    if(m_codec)
    {
        delete m_codec;
        m_codec = nullptr;
    }

    QFile file(m_cueFilePath);
    if(!file.open(QIODevice::ReadOnly))
        return QString();

    QByteArray data = file.readAll();

    QSettings settings;
    settings.beginGroup(u"CUE"_s);
#ifdef WITH_ENCA
    EncaAnalyser analyser = nullptr;
    if(settings.value(u"use_enca"_s, false).toBool())
    {
        analyser = enca_analyser_alloc(settings.value(u"enca_lang"_s).toByteArray ().constData());

        if(analyser)
        {
            enca_set_threshold(analyser, 1.38);
            EncaEncoding encoding = enca_analyse(analyser, (uchar *)data.constData(), data.size());
            if(encoding.charset != ENCA_CS_UNKNOWN)
            {
                m_codec = new QmmpTextCodec(enca_charset_name(encoding.charset,ENCA_NAME_STYLE_ENCA));
            }
            enca_analyser_free(analyser);
        }
    }
#endif
    if(!m_codec)
        m_codec = new QmmpTextCodec(settings.value(u"encoding"_s, u"UTF-8"_s).toByteArray());
    settings.endGroup();

    return m_codec->toUnicode(data);
}

void CUEMetaDataModel::setCue(const QString &content)
{
    if(!m_codec)
    {
        QSettings settings;
        m_codec = new QmmpTextCodec(settings.value(u"CUE/encoding"_s, u"UTF-8"_s).toByteArray());
    }

    QFile file(m_cueFilePath);
    if(!file.open(QIODevice::WriteOnly))
    {
        qCWarning(plugin, "unable to write cue file, error: %s", qPrintable(file.errorString()));
        return;
    }
    file.write(m_codec->fromUnicode(content));
}

void CUEMetaDataModel::removeCue()
{
    QFile::remove(m_cueFilePath);
}
