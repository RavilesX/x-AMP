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

#include <QDir>
#include <QList>
#include <QFileInfo>
#include <QProcess>
#include <QByteArray>
#include <QApplication>
#include <QFile>
#include <QSet>
#include <algorithm>
#include <qmmp/qmmp.h>
#include "skinreader.h"

SkinReader::SkinReader(QObject *parent)
        : QObject(parent)
{
    //create cache dir
    QDir dir(Qmmp::cacheDir());
    dir.mkdir(u"skinned"_s);
    dir.cd(u"skinned"_s);
    dir.mkdir(u"thumbs"_s);
    dir.mkdir(u"skin"_s);
}

SkinReader::~SkinReader()
{}

void SkinReader::loadSkins()
{
    m_skins.clear();
    m_previewHash.clear();
    QStringList paths = skinPaths();
    QFileInfoList infoList;
    //find all file and directories
    for(const QString &path : std::as_const(paths))
    {
        QDir dir(path);
        dir.setSorting(QDir::Name);
        infoList << dir.entryInfoList(QDir::Files | QDir::Hidden | QDir::Dirs | QDir::NoDotAndDotDot);
    }

    //find skins and generate thumbnailes
    QDir cacheDir(Qmmp::cacheDir() + QStringLiteral("/skinned/thumbs"));
    QHash<QString, QString> thumbnailHash; //base name, full path
    QFileInfoList thumbnailes = cacheDir.entryInfoList(QDir::Files | QDir::Hidden);

    for(const QFileInfo &i : std::as_const(thumbnailes))
        thumbnailHash.insert(i.completeBaseName(), i.canonicalFilePath());

    for(const QFileInfo &info : std::as_const(infoList))
    {
        if(info.isDir())
        {
            QDir dir(info.canonicalFilePath());
            const QFileInfoList files = dir.entryInfoList({ QStringLiteral("main.*") }, QDir::Files | QDir::Hidden);
            if(!files.isEmpty())
            {
                m_skins << info.canonicalFilePath();
                m_previewHash.insert(info.canonicalFilePath(), files.constFirst().canonicalFilePath());
            }
        }
        else if(info.isFile())
        {
            QString thumbnailPath = thumbnailHash.value(info.fileName());

            if(thumbnailPath.isEmpty())
            {
                QString name = info.fileName().toLower();

                if(name.endsWith(u".tgz"_s) || name.endsWith(u".tar.gz"_s) || name.endsWith(u".tar.bz2"_s))
                {
                    untar(info.filePath(), cacheDir.absolutePath(), true);
                    m_skins << info.canonicalFilePath();
                    m_previewHash.insert(info.canonicalFilePath(), QString());
                }
                else if(name.endsWith(u".zip"_s) || name.endsWith(u".wsz"_s))
                {
                    unzip(info.filePath(), cacheDir.absolutePath(), true);
                    m_skins << info.canonicalFilePath();
                    m_previewHash.insert(info.canonicalFilePath(), QString());
                }
            }
            else
            {
                m_skins << info.canonicalFilePath();
                m_previewHash.insert(info.canonicalFilePath(), thumbnailPath);
            }
        }
    }


    //add new skins to cache
    cacheDir.refresh();
    thumbnailHash.clear();
    thumbnailes = cacheDir.entryInfoList(QDir::Files | QDir::Hidden);

    for(const QFileInfo &i : std::as_const(thumbnailes))
    {
        if (i.size() > 0)
            thumbnailHash.insert(i.completeBaseName(), i.canonicalFilePath());
    }

    for(const QFileInfo &info : std::as_const(infoList))
    {
        if(info.isDir())
            continue;

        if(m_previewHash.contains(info.canonicalFilePath()) && m_previewHash.value(info.canonicalFilePath()).isEmpty())
        {
            QString thumbnailPath = thumbnailHash.value(info.fileName());
            if(!thumbnailPath.isEmpty())
            {
                qCDebug(plugin, "adding %s to cache", qPrintable(info.fileName()));
                m_previewHash.insert(info.canonicalFilePath(), thumbnailPath);
            }
        }
    }

    //clear removed skins from cache
    const QSet<QString> usedThumbnails(m_previewHash.constBegin(), m_previewHash.constEnd());
    for(const QFileInfo &i : std::as_const(thumbnailes))
    {
        if(!usedThumbnails.contains(i.canonicalFilePath()))
        {
            QFile::remove(i.canonicalFilePath());
            qCDebug(plugin, "deleting %s from cache", qPrintable(i.fileName()));
        }
    }
}

const QStringList &SkinReader::skins() const
{
    return m_skins;
}

const QPixmap SkinReader::getPreview(const QString &skinPath) const
{
    return QPixmap(m_previewHash.value(skinPath));
}

void SkinReader::unpackSkin(const QString &path)
{
    //remove old skin
    QDir dir(unpackedSkinPath());
    dir.setFilter(QDir::Files | QDir::Hidden);
    const QFileInfoList f = dir.entryInfoList();
    for(const QFileInfo &file : std::as_const(f))
        dir.remove(file.fileName());
    //create skin cache directory
    if(!QFile::exists(unpackedSkinPath()))
        QDir::root().mkpath(unpackedSkinPath());
    //unpack
    QString name = QFileInfo(path).fileName().toLower();
    if (name.endsWith(u".tgz"_s) || name.endsWith(u".tar.gz"_s) || name.endsWith(u".tar.bz2"_s))
        untar(path, unpackedSkinPath(), false);
    else if (name.endsWith(u".zip"_s) || name.endsWith(u".wsz"_s))
        unzip(path, unpackedSkinPath(), false);
}

QPixmap SkinReader::getPixmapFromDirectory(const QString &name, const QString &path)
{
    QDir dir(path);
    QFileInfoList f = dir.entryInfoList({ name + QStringLiteral(".*") }, QDir::Files | QDir::Hidden);
    if(!f.isEmpty())
        return QPixmap(f.constFirst().filePath());
    return QPixmap();
}

QString SkinReader::unpackedSkinPath()
{
    return Qmmp::cacheDir() + QStringLiteral("/skinned/skin");
}

QString SkinReader::defaultSkinPath()
{
    return QStringLiteral(":/glare");
}

QStringList SkinReader::findSkins()
{
    static const QStringList filters = {
        u"*.tgz"_s, u"*.tar.gz"_s, u"*.tar.bz2"_s, u"*.zip"_s, u"*.wsz"_s
    };

    QStringList paths = skinPaths();
    QStringList skins;
    QFileInfoList infoList;
    //find all file and directories
    for(const QString &path : std::as_const(paths))
    {
        QDir dir(path);
        dir.setSorting(QDir::Name);
        infoList << dir.entryInfoList(filters, QDir::Files | QDir::Hidden | QDir::Dirs | QDir::NoDotAndDotDot);
        infoList << dir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot);
    }

    for(const QFileInfo &info : std::as_const(infoList))
        skins << info.canonicalFilePath();

    return skins;
}

QStringList SkinReader::skinPaths()
{
    QStringList paths = {
        Qmmp::configDir() + QStringLiteral("/skins"),
        Qmmp::dataPath() + QStringLiteral("/skins"),
#if defined(Q_OS_UNIX) || defined(Q_OS_CYGWIN)
        Qmmp::userDataPath() + QStringLiteral("/skins"),
        //1.x version compatibility
        QDir(qApp->applicationDirPath() +  QStringLiteral("/../share/qmmp-1/skins")).absolutePath()
#endif
    };

    paths.removeDuplicates();
    return paths;
}

#if defined(Q_OS_WIN) && !defined(Q_OS_CYGWIN)
void SkinReader::untar(const QString &from, const QString &to, bool preview)
{
    QProcess process1;
    QProcess process2;

    process1.setStandardOutputProcess(&process2);
    process1.start(u"7za"_s, { u"e"_s, u"-so"_s, from });
    QStringList args = { u"e"_s, u"-si"_s, u"-ttar"_s, u"-y"_s, u"-o"_s + to };

    if(preview)
        args << u"main.???"_s << u"*/main.???"_s;

    process2.start(u"7za"_s, args);

    process1.waitForFinished();
    process2.waitForFinished();

    if(preview)
    {
        QDir dir(to);
        dir.setFilter(QDir::Files | QDir::Hidden);
        const QFileInfoList fileList = dir.entryInfoList();
        for(const QFileInfo &thumbInfo : std::as_const(fileList))
        {
            if(thumbInfo.fileName().startsWith(u"main."_s, Qt::CaseInsensitive))
            {
                dir.rename(thumbInfo.fileName(), from.section(QLatin1Char('/'), -1) + QLatin1Char('.') + thumbInfo.suffix());
            }
        }
    }
}

void SkinReader::unzip(const QString &from, const QString &to, bool preview)
{
    if(preview)
    {
        QStringList args = { u"e"_s, from, u"-y"_s, u"-o"_s + to, u"main.???"_s, u"*/main.???"_s };
        QProcess::execute(u"7za"_s, args);
        QDir dir(to);
        dir.setFilter(QDir::Files | QDir::Hidden);
        const QFileInfoList fileList = dir.entryInfoList();
        for(const QFileInfo &thumbInfo : std::as_const(fileList))
        {
            if(thumbInfo.fileName().startsWith(u"main."_s, Qt::CaseInsensitive))
            {
                dir.rename(thumbInfo.fileName(), from.section(QLatin1Char('/'), -1) + QLatin1Char('.') + thumbInfo.suffix());
            }
        }
    }
    else
    {
        QStringList args = { u"e"_s, from, u"-y"_s, u"-o"_s + to };
        QProcess::execute(u"7za"_s, args);
    }
}

#else
void SkinReader::untar(const QString &from, const QString &to, bool preview)
{
    QProcess process;
    process.start(u"tar"_s, { u"tf"_s, from }); //list archive
    process.waitForFinished();
    QByteArray array = process.readAllStandardOutput();
    const QStringList outputList = QString::fromLocal8Bit(array).split(QChar::LineFeed, Qt::SkipEmptyParts);

    for(QString str : std::as_const(outputList))
    {
        str = str.trimmed();

        if(str.endsWith(QLatin1Char('/')))
            continue;

        if(preview && !str.endsWith(u".png"_s, Qt::CaseInsensitive) &&
                !str.endsWith(u".bmp"_s, Qt::CaseInsensitive) &&
                !str.endsWith(u".xpm"_s, Qt::CaseInsensitive))
        {
            continue;
        }

        if(!preview || (str.contains(u"/main."_s, Qt::CaseInsensitive) || str.startsWith(u"main."_s, Qt::CaseInsensitive)))
        {
            str.replace(QLatin1Char('['), QStringLiteral("\\\\["));
            str.replace(QLatin1Char(']'), QStringLiteral("\\\\]"));

            QStringList args = { u"xvfk"_s , from , u"-O"_s , u"--"_s, str };
            process.start(u"tar"_s, args);
            process.waitForFinished();
            array = process.readAllStandardOutput();

            QString name;
            if (preview)
                name = from.section(QLatin1Char('/'), -1) + QLatin1Char('.') + str.section(QLatin1Char('.'), -1);
            else
                name = str.contains(QLatin1Char('/')) ? str.section(QLatin1Char('/'), -1).toLower() : str.toLower();

            QFile file(to + QLatin1Char('/') + name);
            if(file.open(QIODevice::WriteOnly))
                file.write(array);
        }
    }
}

void SkinReader::unzip(const QString &from, const QString &to, bool preview)
{
    if (preview)
    {
        QStringList args = { u"-C"_s, u"-j"_s, u"-o"_s, u"-qq"_s, u"-d"_s, to, from, u"main.???"_s, u"*/main.???"_s };
        QProcess::execute(u"unzip"_s, args);
        QDir dir(to);
        dir.setFilter(QDir::Files | QDir::Hidden);
        const QFileInfoList fileList = dir.entryInfoList();
        for(const QFileInfo &thumbInfo : std::as_const(fileList))
        {
            if(thumbInfo.fileName().startsWith(u"main."_s, Qt::CaseInsensitive))
            {
                dir.rename(thumbInfo.fileName(), from.section(QLatin1Char('/'), -1) + QLatin1Char('.') + thumbInfo.suffix());
            }
        }
    }
    else
    {
        QStringList args = { u"-j"_s, u"-o"_s, u"-qq"_s, u"-d"_s, to, from };
        QProcess::execute(u"unzip"_s, args);
    }
}
#endif
