/***************************************************************************
 *   Copyright (C) 2013-2026 by Ilya Kotov                                 *
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
#include <QDateTime>
#include <QFileInfo>
#include <QPluginLoader>
#include <QApplication>
#include <QTranslator>
#include "decoderfactory.h"
#include "outputfactory.h"
#include "enginefactory.h"
#include "effectfactory.h"
#include "inputsourcefactory.h"
#include "qmmpplugincache_p.h"

QmmpPluginCache::QmmpPluginCache(const QString &file, QSettings *settings)
{
    bool update = false;
    QFileInfo info(file);
    m_path = info.QFileInfo::canonicalFilePath();

    settings->beginGroup(u"PluginCache"_s);
    QString key = m_path;
#ifndef Q_OS_WIN
    key.remove(0,1);
#endif
    if(settings->allKeys().contains(key))
    {
        QStringList values = settings->value(m_path).toStringList();
        if(values.count() != 6)
            update = true;
        else
        {
            m_shortName = values.at(0);
            m_priority = values.at(1).toInt();
            m_protocols = values.at(2).split(QLatin1Char(';'), Qt::SkipEmptyParts);
            m_filters = values.at(3).split(QLatin1Char(';'), Qt::SkipEmptyParts);
            m_contentTypes = values.at(4).split(QLatin1Char(';'), Qt::SkipEmptyParts);
            update = (info.lastModified().toString(Qt::ISODate) != values.at(5));
        }
    }
    else
        update = true;


    if(update)
    {
        if(decoderFactory())
        {
            m_shortName = m_decoderFactory->properties().shortName;
            m_priority = m_decoderFactory->properties().priority;
            m_protocols = m_decoderFactory->properties().protocols;
            m_filters = m_decoderFactory->properties().filters;
            m_contentTypes = m_decoderFactory->properties().contentTypes;
        }
        else if(outputFactory())
        {
            m_shortName = m_outputFactory->properties().shortName;
            m_priority = 0;
        }
        else if(engineFactory())
        {
            m_shortName = m_engineFactory->properties().shortName;
            m_priority = m_engineFactory->properties().priority;
            m_protocols = m_engineFactory->properties().protocols;
            m_filters = m_engineFactory->properties().filters;
            m_contentTypes = m_engineFactory->properties().contentTypes;
        }
        else if(effectFactory())
        {
            m_shortName = m_effectFactory->properties().shortName;
            m_priority = m_effectFactory->properties().priority;
        }
        else if(inputSourceFactory())
        {
            m_shortName = m_inputSourceFactory->properties().shortName;
            m_priority = 0;
            m_protocols = m_inputSourceFactory->properties().protocols;
        }
        else
        {
            qCWarning(core, "unknown plugin type: %s", qPrintable(m_path));
            m_error = true;
        }

        if(!m_error)
        {
            const QStringList values = {
                m_shortName,
                QString::number(m_priority),
                m_protocols.join(QLatin1Char(';')),
                m_filters.join(QLatin1Char(';')),
                m_contentTypes.join(QLatin1Char(';')),
                info.lastModified().toString(Qt::ISODate)
            };
            settings->setValue(m_path, values);
            qCDebug(core, "added cache item \"%s=%s\"", qPrintable(info.fileName()), qPrintable(values.join(QLatin1Char(','))));
        }
    }

    settings->endGroup();
}

QString QmmpPluginCache::shortName() const
{
    return m_shortName;
}

QString QmmpPluginCache::file() const
{
    return m_path;
}

QStringList QmmpPluginCache::filters() const
{
    if(m_decoderFactory)
        return m_decoderFactory->properties().filters;

    if(m_engineFactory)
        return m_engineFactory->properties().filters;

    return m_filters;
}

QStringList QmmpPluginCache::contentTypes() const
{
    return m_contentTypes;
}

QStringList QmmpPluginCache::protocols() const
{
    return m_protocols;
}

int QmmpPluginCache::priority() const
{
    return m_priority;
}

void QmmpPluginCache::setPriority(int priority)
{
    m_priority = priority;
}

DecoderFactory *QmmpPluginCache::decoderFactory()
{
    if(!m_decoderFactory)
    {
        m_decoderFactory = qobject_cast<DecoderFactory *> (instance());
        if(m_decoderFactory)
            loadTranslation(m_decoderFactory->translation());
    }
    return m_decoderFactory;
}

OutputFactory *QmmpPluginCache::outputFactory()
{
    if(!m_outputFactory)
    {
        m_outputFactory = qobject_cast<OutputFactory *> (instance());
        if(m_outputFactory)
            loadTranslation(m_outputFactory->translation());
    }
    return m_outputFactory;
}

EngineFactory *QmmpPluginCache::engineFactory()
{
    if(!m_engineFactory)
    {
        m_engineFactory = qobject_cast<EngineFactory *> (instance());
        if(m_engineFactory)
            loadTranslation(m_engineFactory->translation());
    }
    return m_engineFactory;
}

EffectFactory *QmmpPluginCache::effectFactory()
{
    if(!m_effectFactory)
    {
        m_effectFactory = qobject_cast<EffectFactory *> (instance());
        if(m_effectFactory)
            loadTranslation(m_effectFactory->translation());
    }
    return m_effectFactory;
}

InputSourceFactory *QmmpPluginCache::inputSourceFactory()
{
    if(!m_inputSourceFactory)
    {
        m_inputSourceFactory = qobject_cast<InputSourceFactory *> (instance());
        if(m_inputSourceFactory)
            loadTranslation(m_inputSourceFactory->translation());
    }
    return m_inputSourceFactory;
}

void QmmpPluginCache::update(QSettings *settings)
{
    //save changed filters list only
    if(m_filters != filters())
    {
        m_filters = filters();

        settings->beginGroup(u"PluginCache"_s);
        QStringList values = settings->value(m_path).toStringList();
        if(values.count() == 6)
        {
            values[3] = m_filters.join(QLatin1Char(';'));
            settings->setValue(m_path, values);
        }
        settings->endGroup();
    }
}

bool QmmpPluginCache::hasError() const
{
    return m_error;
}

QObject *QmmpPluginCache::instance()
{
    if(m_error)
        return nullptr;
    if(m_instance)
        return m_instance;
    QPluginLoader loader(m_path);
    m_instance = loader.instance();
    if (loader.isLoaded())
        qCDebug(core) << "loaded plugin" << QFileInfo(m_path).fileName();
    else
    {
        m_error = true;
        m_filters.clear();
        m_contentTypes.clear();
        m_protocols.clear();
        qCWarning(core, "error: %s", qPrintable(loader.errorString ()));
    }
    return m_instance;
}

void QmmpPluginCache::loadTranslation(const QString &translation)
{
    if(!translation.isEmpty())
    {
        QTranslator *translator = new QTranslator(qApp);
        if(translator->load(translation + Qmmp::systemLanguageID()))
            qApp->installTranslator(translator);
        else
            delete translator;
    }
}

void QmmpPluginCache::cleanup(QSettings *settings)
{
    settings->beginGroup(u"PluginCache"_s);

    for(const QString &key : settings->allKeys())
    {
#ifdef Q_OS_WIN
        if(!QFile::exists(key))
#else
        if(!QFile::exists(u"/"_s + key))
#endif
        {
            settings->remove(key);
            qCDebug(core) << "removed key" << key;
        }
    }
    settings->endGroup();
}
