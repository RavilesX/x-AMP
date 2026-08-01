/***************************************************************************
 *   Copyright (C) 2010-2026 by Ilya Kotov                                 *
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

#include <QSettings>
#include <QDir>
#include <QInputDialog>
#include <qmmp/inputsourcefactory.h>
#include <qmmp/inputsource.h>
#include <qmmp/decoderfactory.h>
#include <qmmp/outputfactory.h>
#include <qmmp/visualfactory.h>
#include <qmmp/effectfactory.h>
#include <qmmp/effect.h>
#include <qmmp/visual.h>
#include <qmmp/output.h>
#include <qmmp/soundcore.h>
#include <qmmp/enginefactory.h>
#include <qmmp/abstractengine.h>
#include "generalfactory.h"
#include "general.h"
#include "uihelper.h"
#include "filedialogfactory.h"
#include "filedialog.h"
#include "uiloader.h"
#include "radioitemdelegate_p.h"
#include "pluginitem_p.h"

PluginItem::PluginItem(QTreeWidgetItem *parent, InputSourceFactory *factory, const QString &path) :
    PluginItem(parent, factory->properties().name, path, InputSource::isEnabled(factory), TRANSPORT)
{
    m_hasAbout = factory->properties().hasAbout;
    m_hasConfig = factory->properties().hasSettings;
    m_factory = factory;
}

PluginItem::PluginItem(QTreeWidgetItem *parent, DecoderFactory *factory, const QString &path) :
    PluginItem(parent, factory->properties().name, path, Decoder::isEnabled(factory), DECODER)
{
    m_hasAbout = factory->properties().hasAbout;
    m_hasConfig = factory->properties().hasSettings;
    m_factory = factory;
}

PluginItem::PluginItem(QTreeWidgetItem *parent, EngineFactory *factory, const QString &path) :
    PluginItem(parent, factory->properties().name, path, AbstractEngine::isEnabled(factory), ENGINE)
{
    m_hasAbout = factory->properties().hasAbout;
    m_hasConfig = factory->properties().hasSettings;
    m_factory = factory;
}

PluginItem::PluginItem(QTreeWidgetItem *parent, EffectFactory *factory, const QString &path) :
    PluginItem(parent, factory->properties().name, path, Effect::isEnabled(factory), EFFECT)
{
    m_hasAbout = factory->properties().hasAbout;
    m_hasConfig = factory->properties().hasSettings;
    m_factory = factory;
}

PluginItem::PluginItem(QTreeWidgetItem *parent, VisualFactory *factory, const QString &path) :
    PluginItem(parent, factory->properties().name, path, Visual::isEnabled(factory), VISUAL)
{
    m_hasAbout = factory->properties().hasAbout;
    m_hasConfig = factory->properties().hasSettings;
    m_factory = factory;
}

PluginItem::PluginItem(QTreeWidgetItem *parent, GeneralFactory *factory, const QString &path) :
    PluginItem(parent, factory->properties().name, path, General::isEnabled(factory), GENERAL)
{
    m_hasAbout = factory->properties().hasAbout;
    m_hasConfig = factory->properties().hasSettings;
    m_factory = factory;
}

PluginItem::PluginItem(QTreeWidgetItem *parent, OutputFactory *factory, const QString &path) :
    PluginItem(parent, factory->properties().name, path, Output::currentFactory() == factory, OUTPUT)
{
    m_hasAbout = factory->properties().hasAbout;
    m_hasConfig = factory->properties().hasSettings;
    m_factory = factory;
    setData(0, RadioButtonRole, true);
}

PluginItem::PluginItem(QTreeWidgetItem *parent, FileDialogFactory *factory, const QString &path) :
    PluginItem(parent, factory->properties().name, path, FileDialog::isEnabled(factory), FILE_DIALOG)
{
    m_hasAbout = factory->properties().hasAbout;
    m_hasConfig = false;
    m_factory = factory;
    setData(0, RadioButtonRole, true);
}

PluginItem::PluginItem(QTreeWidgetItem *parent, UiFactory *factory, const QString &path) :
    PluginItem(parent, factory->properties().name, path, UiLoader::selected() == factory, USER_INTERFACE)
{
    m_hasAbout = factory->properties().hasAbout;
    m_hasConfig = false;
    m_factory = factory;
    setData(0, RadioButtonRole, true);
}

bool PluginItem::hasAbout() const
{
    return m_hasAbout;
}
bool PluginItem::hasSettings() const
{
    return m_hasConfig;
}

void PluginItem::showAbout(QWidget *parent)
{
    switch(type())
    {
    case PluginItem::TRANSPORT:
        static_cast<InputSourceFactory *>(m_factory)->showAbout(parent);
        break;
    case PluginItem::DECODER:
        static_cast<DecoderFactory *>(m_factory)->showAbout(parent);
        break;
    case PluginItem::ENGINE:
        static_cast<EngineFactory *>(m_factory)->showAbout(parent);
        break;
    case PluginItem::EFFECT:
        static_cast<EffectFactory *>(m_factory)->showAbout(parent);
        break;
    case PluginItem::VISUAL:
        static_cast<VisualFactory *>(m_factory)->showAbout(parent);
        break;
    case PluginItem::GENERAL:
        static_cast<GeneralFactory *>(m_factory)->showAbout(parent);
        break;
    case PluginItem::OUTPUT:
        static_cast<OutputFactory *>(m_factory)->showAbout(parent);
        break;
    case PluginItem::FILE_DIALOG:
        static_cast<FileDialogFactory *>(m_factory)->showAbout(parent);
        break;
    case PluginItem::USER_INTERFACE:
        static_cast<UiFactory *>(m_factory)->showAbout(parent);
        break;
    default:
        ;
    }
}

void PluginItem::showSettings(QWidget *parent)
{
    QDialog *settingDialog = nullptr;

    switch(type())
    {
    case PluginItem::TRANSPORT:
        settingDialog = static_cast<InputSourceFactory *>(m_factory)->createSettings(parent);
        break;
    case PluginItem::DECODER:
        settingDialog = static_cast<DecoderFactory *>(m_factory)->createSettings(parent);
        break;
    case PluginItem::ENGINE:
        settingDialog = static_cast<EngineFactory *>(m_factory)->createSettings(parent);
        break;
    case PluginItem::EFFECT:
        settingDialog = static_cast<EffectFactory *>(m_factory)->createSettings(parent);
        break;
    case PluginItem::VISUAL:
        Visual::showSettings(static_cast<VisualFactory *>(m_factory), parent);
        break;
    case PluginItem::GENERAL:
        General::showSettings(static_cast<GeneralFactory *>(m_factory), parent);
        break;
    case PluginItem::OUTPUT:
        settingDialog = static_cast<OutputFactory *>(m_factory)->createSettings(parent);
        break;
    default:
        ;
    }

    if(settingDialog)
    {
        settingDialog->exec();
        settingDialog->deleteLater();
    }
}

void PluginItem::showPriority(QWidget *parent)
{
    if(type() == PluginItem::DECODER)
    {
        DecoderFactory *factory = static_cast<DecoderFactory *>(m_factory);
        int priority = Decoder::priority(factory);
        bool ok;
        priority = QInputDialog::getInt(parent,
                                        factory->properties().name,
                                        QCoreApplication::translate("PluginItem", "Priority (a higher value means lower priority):"),
                                        priority, 0, 100, 1, &ok);
        if(ok)
        {
            Decoder::setPriority(factory, priority);
        }
    }
    else if(type() == PluginItem::ENGINE)
    {
        EngineFactory *factory = static_cast<EngineFactory *>(m_factory);
        int priority = AbstractEngine::priority(factory);
        bool ok;
        priority = QInputDialog::getInt(parent,
                                        factory->properties().name,
                                        QCoreApplication::translate("PluginItem", "Priority (a higher value means lower priority):"),
                                        priority, 0, 100, 1, &ok);
        if(ok)
        {
            AbstractEngine::setPriority(factory, priority);
        }
    }
}

void PluginItem::setEnabled(bool enabled)
{
    switch(type())
    {
    case PluginItem::TRANSPORT:
        InputSource::setEnabled(static_cast<InputSourceFactory *>(m_factory), enabled);
        break;
    case PluginItem::DECODER:
        Decoder::setEnabled(static_cast<DecoderFactory *>(m_factory), enabled);
        break;
    case PluginItem::ENGINE:
        AbstractEngine::setEnabled(static_cast<EngineFactory *>(m_factory), enabled);
        break;
    case PluginItem::EFFECT:
        Effect::setEnabled(static_cast<EffectFactory *>(m_factory), enabled);
        break;
    case PluginItem::VISUAL:
        Visual::setEnabled(static_cast<VisualFactory *>(m_factory), enabled);
        break;
    case PluginItem::GENERAL:
        General::setEnabled(static_cast<GeneralFactory *>(m_factory), enabled);
        break;
    case PluginItem::OUTPUT:
        if(enabled)
        {
            Output::setCurrentFactory(static_cast<OutputFactory *>(m_factory));
        }
        break;
    case PluginItem::FILE_DIALOG:
        if(enabled)
        {
            FileDialog::setEnabled(static_cast<FileDialogFactory *>(m_factory));
        }
        break;
    case PluginItem::USER_INTERFACE:
        if(enabled)
        {
             UiLoader::select(static_cast<UiFactory *>(m_factory));
        }
        break;
    default:
        ;
    }
}

void PluginItem::revertState()
{
    if((checkState(0) == Qt::Checked) != m_prevState)
        setEnabled(m_prevState);
}

PluginItem::PluginItem(QTreeWidgetItem *parent, const QString &name, const QString &path, bool checked, int type) :
    QTreeWidgetItem(parent, { name, path.section(QLatin1Char('/'), -1) }, type),
    m_prevState(checked)
{
    setCheckState(0, checked ? Qt::Checked : Qt::Unchecked);
}
