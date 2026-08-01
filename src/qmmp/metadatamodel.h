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

#ifndef METADATAMODEL_H
#define METADATAMODEL_H

#include <QHash>
#include <QList>
#include <QString>
#include <QCoreApplication>
#include <QImage>
#include <QVariant>
#include <QFlags>
#include "tagmodel.h"

class MetaDataItemPrivate;
class MetaDataModelPrivate;

/*! @brief Container of extra file/track property.
 * @author Ilya Kotov <forkotov02@ya.ru>
 */
class QMMP_EXPORT MetaDataItem
{
    Q_DECLARE_PRIVATE(MetaDataItem)
public:
    /*!
     * Constructor
     * \param name Localized name of property.
     * \param value Property value.
     * \param suffix Localized suffix of property (i.e. kbit, kbps, etc).
     */
    MetaDataItem(const QString &name, const QVariant &value, const QString &suffix = QString());
    /*!
     * Constructs a copy of \b other.
     */
    MetaDataItem(const MetaDataItem &other);
    /*!
     * Move-constructs a MetaDataItem instance, making it point at the same object that \b other was pointing to.
     */
    MetaDataItem(MetaDataItem &&other) noexcept;
    /*!
     * Destructor.
     */
    ~MetaDataItem();
    /*!
     * Assigns \b other to this MetaDataItem instance and returns a reference to this instance.
     */
    MetaDataItem &operator=(const MetaDataItem &other);
    /*!
     * Move-assigns \b other to this MetaDataItem instance.
     */
    MetaDataItem &operator=(MetaDataItem &&other) noexcept;
    /*!
     * Returns localized name of property.
     */
    QString name() const;
    /*!
     * Changes localized name to \b name
     */
    void setName(const QString &name);
    /*!
     * Returns property value.
     */
    QVariant value() const;
    /*!
     * Changes property value.
     */
    void setValue(const QVariant &value);
    /*!
     * Returns suffix of property.
     */
    QString suffix() const;
    /*!
     * Changes property suffix to \b suffixed
     */
    void setSuffix(const QString &suffix);

private:
    MetaDataItemPrivate *d_ptr;
};

/*! @brief The MetaDataModel is the base interface class of metadata access.
 * @author Ilya Kotov <forkotov02@ya.ru>
 */
class QMMP_EXPORT MetaDataModel
{
    Q_DECLARE_PRIVATE(MetaDataModel)
public:
    /*!
     * Details dialog settings.
     */
    enum DialogHint
    {
        IsCoverEditable = 0x01,      /*!< Enable cover editor. */
        CompletePropertyList = 0x02, /*!< Show properties from \b extraProperties() only (ignore other sources) */
        IsCueEditable = 0x04,        /*!< Enable CUE editor. */
        IsLyricsEditable = 0x08      /*!< Enable Lyrics editor. */
    };
    Q_DECLARE_FLAGS(DialogHints, DialogHint)
    /*!
     * Constructor.
     * \param readOnly Open file in read-only mode (\b true - enabled, \b false - disable).
     * \param hints Details dialog settings.
     */
    MetaDataModel(bool readOnly, DialogHints hints = DialogHints());
    /*!
     * Destructor.
     */
    virtual ~MetaDataModel();
    /*!
     * Returns extra properties of the media source (in addition to the \b Qmmp::TrackProperty values).
     * Default implemetation returns empty array.
     */
    virtual QList<MetaDataItem> extraProperties() const;
    /*!
     * Returns a list of long descriptions.
     * Default implemetation returns empty array.
     */
    virtual QList<MetaDataItem> descriptions() const;
    /*!
     * Returns a list of available tags.
     * Subclass should reimplement this function. Default implementation returns empty array.
     */
    virtual QList<TagModel* > tags() const;
    /*!
     * Returns cover image.
     * Subclass should reimplement this function. Default implementation returns empty pixmap.
     */
    virtual QImage cover() const;
    /*!
     * Sets cover.
     * \param img Cover image.
     * Subclass should reimplement this function. Default implementation does nothing.
     */
    virtual void setCover(const QImage &img);
    /*!
     * Removes cover.
     * Subclass should reimplement this function. Default implementation does nothing.
     */
    virtual void removeCover();
    /*!
     * Returns path to cover pixmap.
     */
    virtual QString coverPath() const;
    /*!
     * Returns CUE file or tag content if necessary. Default implementation returns empty string.
     */
    virtual QString cue() const;
    /*!
     * Updates CUE file or tag content. Default implementation doesn nothing.
     */
    virtual void setCue(const QString &content);
    /*!
     * Removes CUE file or tag. Default implementation does nothing.
     */
    virtual void removeCue();
    /*!
     * Returns song lyrics. Default implementation returns empty string.
     */
    virtual QString lyrics() const;
    /*!
     * Sets song lyrics. Default implementation does nothing.
     */
    virtual void setLyrics(const QString &content);
    /*!
     * Removes song lyrics. Default implementation calls setLyrics(QString()).
     */
    virtual void removeLyrics();
    /*!
     * Returns \b true if file is opened in read only mode. Otherwise returns \b false.
     */
    bool isReadOnly() const;
    /*!
     * Returns details dialog hints.
     */
    DialogHints dialogHints() const;

protected:
    /*!
     * Changes details dialog hints to \b hints
     */
    void setDialogHints(const DialogHints &hints);
    /*!
     * Enables/Disables read only mode (\b true - enabled, \b false - disable).
     * \param readOnly read only mode (\b true - enabled, \b false - disable).
     */
    void setReadOnly(bool readOnly);

private:
    MetaDataModelPrivate *d_ptr;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(MetaDataModel::DialogHints)

#endif // METADATAMODEL_H
