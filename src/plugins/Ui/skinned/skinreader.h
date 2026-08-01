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
#ifndef SKINREADER_H
#define SKINREADER_H

#include <QObject>
#include <QMap>
#include <QHash>
#include <QPixmap>
#include <QStringList>

/**
    @author Ilya Kotov <forkotov02@ya.ru>
*/

class SkinReader : public QObject
{
    Q_OBJECT
public:
    explicit SkinReader(QObject *parent = nullptr);

    ~SkinReader();

    void loadSkins();
    const QStringList &skins() const;
    const QPixmap getPreview(const QString &skinPath) const;

    static void unpackSkin(const QString &path);
    static QPixmap getPixmapFromDirectory(const QString &name, const QString &path);
    static QString unpackedSkinPath();
    static QString defaultSkinPath();
    static QStringList findSkins();

private:
    static QStringList skinPaths();
    static void untar(const QString &from, const QString &to, bool preview);
    static void unzip(const QString &from, const QString &to,  bool preview);

    QStringList m_skins;
    QHash<QString, QString> m_previewHash; //skin path, thumbnail path
};

#endif
