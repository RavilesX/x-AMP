/**************************************************************************
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

#include <QFile>
#include <QSettings>
#include <QPluginLoader>
#include <QMetaObject>
#include <QDir>
#include <QFileInfo>
#include <algorithm>
#include <qmmp/qmmp.h>
#include "qcoreapplication.h"
#include "qmmpuiplugincache_p.h"
#include "filedialog.h"
#include "qtfiledialog_p.h"

Q_GLOBAL_STATIC(QList<QmmpUiPluginCache *>, fileDialogCache);

class FileDialogPrivate
{
public:
    FileDialogPrivate(FileDialog *dialog)
    {
        instance = dialog;
    }

    ~FileDialogPrivate()
    {
        instance = nullptr;
    }

    static void loadPlugins()
    {
        if(fileDialogCache.exists())
            return;

        fileDialogCache->append(new QmmpUiPluginCache(new QtFileDialogFactory));

        QSettings settings;
        for(const QString &filePath : Qmmp::findPlugins(u"FileDialogs"_s))
        {
            QmmpUiPluginCache *item = new QmmpUiPluginCache(filePath, &settings);
            if(item->hasError())
            {
                delete item;
                continue;
            }
            fileDialogCache->append(item);
        }
        qAddPostRoutine(FileDialogPrivate::cleanup);
    }

    void updateLastDir(const QStringList &list)
    {
        if(!list.isEmpty() && lastDir)
        {
            QString path = list.constFirst();
            if(path.endsWith(QLatin1Char('/')))
                path.remove(path.size() - 1, 1);
            *lastDir = path.left(path.lastIndexOf(QLatin1Char('/')));
        }
    }

    static void cleanup()
    {
        if(fileDialogCache.exists())
        {
            qDeleteAll(*fileDialogCache);
        }
    }

    bool initialized = false;
    QString *lastDir = nullptr;

    static FileDialogFactory *currentFactory;
    static FileDialog *instance;
};

//static functions
FileDialog *FileDialogPrivate::instance = nullptr;
FileDialogFactory *FileDialogPrivate::currentFactory = nullptr;

QList<FileDialogFactory *> FileDialog::factories()
{
    FileDialogPrivate::loadPlugins();
    QList<FileDialogFactory *> list;
    for(QmmpUiPluginCache *item : std::as_const(*fileDialogCache))
    {
        if(item->fileDialogFactory())
            list.append(item->fileDialogFactory());
    }
    return list;
}

void FileDialog::setEnabled(const FileDialogFactory *factory)
{
    FileDialogPrivate::loadPlugins();
    QSettings settings;
    settings.setValue("FileDialog"_L1, factory->properties().shortName);
}

bool FileDialog::isEnabled(const FileDialogFactory *factory)
{
    FileDialogPrivate::loadPlugins();
    QSettings settings;
    QString name = settings.value(u"FileDialog"_s, u"qt_dialog"_s).toString();
    return factory->properties().shortName == name;
}

QString FileDialog::file(const FileDialogFactory *factory)
{
    FileDialogPrivate::loadPlugins();
    auto it = std::find_if(fileDialogCache->cbegin(), fileDialogCache->cend(),
                           [factory] (QmmpUiPluginCache *item){ return item->shortName() == factory->properties().shortName; } );
    return it == fileDialogCache->cend() ? QString() : (*it)->file();
}

QString FileDialog::getExistingDirectory(QWidget *parent,
        const QString &caption,
        const QString &dir)
{
    QStringList l = instance()->exec(parent, dir, FileDialog::AddDir, caption);
    return l.isEmpty() ? QString() : l.constFirst();
}

QString FileDialog::getOpenFileName(QWidget *parent,
                                    const QString &caption,
                                    const QString &dir,
                                    const QString &filter,
                                    QString *selectedFilter)
{
   QStringList l = instance()->exec(parent, dir, FileDialog::AddFile, caption, filter, selectedFilter);
   return l.isEmpty() ? QString() : l.constFirst();
}

QStringList FileDialog::getOpenFileNames(QWidget *parent, const QString &caption,
        const QString &dir,const QString &filter,
        QString *selectedFilter)
{
    return instance()->exec(parent, dir, FileDialog::AddFiles, caption, filter, selectedFilter);
}

QString FileDialog::getSaveFileName (QWidget *parent, const QString &caption,
                                     const QString& dir, const QString &filter,
                                     QString *selectedFilter)
{
    QStringList l = instance()->exec(parent, dir, FileDialog::SaveFile, caption, filter, selectedFilter);
    return l.isEmpty() ? QString() : l.at(0);
}

FileDialog *FileDialog::instance()
{
    FileDialogPrivate::loadPlugins();
    FileDialogFactory *selected = nullptr;

    QSettings settings;
    QString name = settings.value(u"FileDialog"_s, u"qt_dialog"_s).toString();

    auto it = std::find_if(fileDialogCache->cbegin(), fileDialogCache->cend(),
                           [name] (QmmpUiPluginCache *item){ return item->shortName() == name; } );
    if(it != fileDialogCache->cend())
        selected = (*it)->fileDialogFactory();

    if(!selected)
        selected = fileDialogCache->constFirst()->fileDialogFactory();

    if(selected == FileDialogPrivate::currentFactory && FileDialogPrivate::instance)
        return FileDialogPrivate::instance;

    delete FileDialogPrivate::instance;
    FileDialogPrivate::currentFactory = selected;
    return FileDialogPrivate::currentFactory->create();
}

void FileDialog::popupImpl(QWidget *parent,
                           Mode m,
                           QString *dir,
                           const QString &caption,
                           const QString &filters)

{
    if(!dir)
        qCFatal(core) << "empty last dir pointer";
    FileDialog *inst = instance();
    inst->setParent(parent);
    inst->d_ptr->lastDir = dir;
    inst->d_ptr->initialized = true;
    connect(inst, &FileDialog::filesSelected, inst, [inst](const QStringList &selected) { inst->d_func()->updateLastDir(selected); });
    if(!FileDialogPrivate::currentFactory->properties().modal)
        inst->raise(*dir, m, caption, filters.split(u";;"_s));
    else
    {
        QStringList files;
        if(m == AddFiles || m == AddFile || m == AddDirsFiles || m == PlayDirsFiles)
        {
            QString selectedFilter;
            files = getOpenFileNames(parent, caption, *dir, filters, &selectedFilter);
        }
        else if(m == AddDirs || m == AddDir)
        {
            QString path = getExistingDirectory(parent, caption, *dir);
            if(!path.isEmpty())
                files << path;
        }
        QMetaObject::invokeMethod(inst, "filesSelected", Q_ARG(QStringList, files));
    }
}

//base implementation
FileDialog::FileDialog() : d_ptr(new FileDialogPrivate(this))
{}

FileDialog::~FileDialog()
{
    delete d_ptr;
}

void FileDialog::raise(const QString &dir, Mode mode, const QString &caption, const QStringList &mask)
{
    Q_UNUSED(dir);
    Q_UNUSED(mode);
    Q_UNUSED(caption);
    Q_UNUSED(mask);
}
