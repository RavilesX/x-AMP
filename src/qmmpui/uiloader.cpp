/***************************************************************************
 *   Copyright (C) 2011-2026 by Ilya Kotov                                 *
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
#include <QApplication>
#include <qmmp/qmmp.h>
#include <algorithm>
#include "qmmpuiplugincache_p.h"
#include "uiloader.h"

#ifndef QMMP_DEFAULT_UI
#define QMMP_DEFAULT_UI "skinned"
#endif

Q_GLOBAL_STATIC(QList<QmmpUiPluginCache *>, uiCache);

class UiLoaderPrivate
{
public:
    static void loadPlugins()
    {
        if(uiCache.exists())
            return;

        QSettings settings;
        for(const QString &filePath : Qmmp::findPlugins(u"Ui"_s))
        {
            QmmpUiPluginCache *item = new QmmpUiPluginCache(filePath, &settings);
            if(item->hasError())
            {
                delete item;
                continue;
            }
            uiCache->append(item);
        }
        qAddPostRoutine(UiLoaderPrivate::cleanup);
    }

    static void cleanup()
    {
        if(uiCache.exists())
        {
            qDeleteAll(*uiCache);
        }
    }
};

QList<UiFactory *> UiLoader::factories()
{
    UiLoaderPrivate::loadPlugins();
    QList<UiFactory *> list;
    for(QmmpUiPluginCache *item : std::as_const(*uiCache))
    {
        if(item->uiFactory())
            list.append(item->uiFactory());
    }
    return list;
}

QStringList UiLoader::names()
{
    QStringList out;
    UiLoaderPrivate::loadPlugins();
    for(const QmmpUiPluginCache *item : std::as_const(*uiCache))
    {
        out << item->shortName();
    }
    return out;
}

QString UiLoader::file(UiFactory *factory)
{
    UiLoaderPrivate::loadPlugins();
    auto it = std::find_if(uiCache->cbegin(), uiCache->cend(),
                           [factory](QmmpUiPluginCache *item) { return item->shortName() == factory->properties().shortName; } );
    return it == uiCache->cend() ? QString() : (*it)->file();
}

void UiLoader::select(UiFactory* factory)
{
    select(factory->properties().shortName);
}

void UiLoader::select(const QString &name)
{
    UiLoaderPrivate::loadPlugins();
    if(std::any_of(uiCache->cbegin(), uiCache->cend(), [name](QmmpUiPluginCache *item) { return item->shortName() == name; } ))
    {
        QSettings settings;
        settings.setValue (u"Ui/current_plugin"_s, name);
    }
}

UiFactory *UiLoader::selected()
{
    UiLoaderPrivate::loadPlugins();
    QSettings settings;
#ifdef Q_OS_UNIX
    QString defaultUi = QStringLiteral(QMMP_DEFAULT_UI);
    if(defaultUi == QLatin1String("skinned") && qApp->platformName() == QLatin1String("wayland"))
        defaultUi = u"qsui"_s;
    QString name = settings.value(u"Ui/current_plugin"_s, defaultUi).toString();
#else
    QString name = settings.value(u"Ui/current_plugin"_s, QStringLiteral(QMMP_DEFAULT_UI)).toString();
#endif
    for(QmmpUiPluginCache *item : std::as_const(*uiCache))
    {
        if(item->shortName() == name && item->uiFactory())
            return item->uiFactory();
    }
    if(!uiCache->isEmpty())
        return uiCache->constFirst()->uiFactory();
    return nullptr;
}
