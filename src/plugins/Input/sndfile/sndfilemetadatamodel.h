/***************************************************************************
 *   Copyright (C) 2025-2026 by Ilya Kotov                                 *
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

#ifndef SNDFILEMETADATAMODEL_H
#define SNDFILEMETADATAMODEL_H

#include <qmmp/metadatamodel.h>
#include <taglib/wavfile.h>
#include <taglib/aifffile.h>
#include <taglib/id3v2tag.h>
#include <taglib/tfilestream.h>

class SndFileMetaDataModel : public MetaDataModel
{
    Q_DECLARE_TR_FUNCTIONS(SndFileMetaDataModel)
public:
    SndFileMetaDataModel(const QString &path, bool readOnly);
    ~SndFileMetaDataModel();

    QList<TagModel* > tags() const override;
    QImage cover() const override;
    void setCover(const QImage &img) override;
    void removeCover() override;
    QString lyrics() const override;

private:
    QList<TagModel* > m_tags;
    TagLib::FileStream *m_stream  = nullptr;
    TagLib::RIFF::WAV::File *m_wavFile = nullptr;
    TagLib::RIFF::AIFF::File *m_aiffFile = nullptr;
};

class SndFileTagModel : public TagModel
{
public:
    SndFileTagModel(TagLib::RIFF::WAV::File *file);
    SndFileTagModel(TagLib::RIFF::AIFF::File *file);
    ~SndFileTagModel() = default;

    QString name() const override;
    QString value(Qmmp::MetaData key) const override;
    void setValue(Qmmp::MetaData key, const QString &value) override;
    bool exists() const override;
    void create() override;
    void remove() override;
    void save() override;
    QString lyrics() const;

private:
    TagLib::RIFF::WAV::File *m_wavFile = nullptr;
    TagLib::RIFF::AIFF::File *m_aiffFile = nullptr;
    TagLib::ID3v2::Tag *m_tag = nullptr;
};

#endif // MPEGMETADATAMODEL_H
