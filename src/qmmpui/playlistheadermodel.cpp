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

#include <QApplication>
#include <QMetaObject>
#include <QHash>
#include <QVariant>
#include <qmmp/qmmp.h>
#include "playlistmanager.h"
#include "columneditor_p.h"
#include "metadatahelper_p.h"
#include "playlistheadermodel.h"

class PlayListHeaderModelPrivate
{
public:
    ~PlayListHeaderModelPrivate()
    {
        columns.clear();
    }

    void updatePlayLists()
    {
        QStringList patterns;
        for(int i = 0; i < columns.count(); ++i)
            patterns.append(columns[i].pattern);
        helper->setTitleFormats(patterns);

        for(PlayListModel *model : PlayListManager::instance()->playLists())
        {
            QMetaObject::invokeMethod(model, "listChanged", Q_ARG(int, PlayListModel::METADATA));
        }
    }

    struct ColumnHeader
    {
        QString name;
        QString pattern;
        QHash<int, QVariant> data;
    };
    QList<ColumnHeader> columns;
    bool settingsLoaded = false;
    MetaDataHelper *helper = MetaDataHelper::instance();
};


PlayListHeaderModel::PlayListHeaderModel(QObject *parent) :
    QObject(parent),
    d_ptr(new PlayListHeaderModelPrivate)
{
    Q_D(PlayListHeaderModel);
    PlayListHeaderModelPrivate::ColumnHeader col;
    col.name = tr("Artist - Title");
    col.pattern = u"%if(%p,%p - %t,%t)"_s;
    d->columns.append(col);
    d->helper->setTitleFormats({ col.pattern });
}

PlayListHeaderModel::~PlayListHeaderModel()
{
    delete d_ptr;
}

void PlayListHeaderModel::restoreSettings(const QString &groupName)
{
    QSettings settings;
    settings.beginGroup(groupName);
    restoreSettings(&settings);
    settings.endGroup();
}

void PlayListHeaderModel::restoreSettings(QSettings *settings)
{
    Q_D(PlayListHeaderModel);
    QStringList names = settings->value(u"pl_column_names"_s).toStringList();
    QStringList patterns = settings->value(u"pl_column_patterns"_s).toStringList();

    if(!names.isEmpty() && (names.count() == patterns.count()))
    {
        d->columns.clear();
        for(int i = 0; i < names.count(); ++i)
        {
            PlayListHeaderModelPrivate::ColumnHeader h = { names.at(i), patterns.at(i), QHash<int, QVariant>() };
            d->columns.append(h);
        }
        d->helper->setTitleFormats(patterns);
    }
    d->settingsLoaded = true;
}

void PlayListHeaderModel::saveSettings(const QString &groupName)
{
    QSettings settings;
    settings.beginGroup(groupName);
    saveSettings(&settings);
    settings.endGroup();
}

void PlayListHeaderModel::saveSettings(QSettings *settings)
{
    Q_D(PlayListHeaderModel);
    QStringList names, patterns;
    for(int i = 0; i < d->columns.count(); ++i)
    {
        names << d->columns[i].name;
        patterns << d->columns[i].pattern;
    }

    settings->setValue(u"pl_column_names"_s, names);
    settings->setValue(u"pl_column_patterns"_s, patterns);
}

bool PlayListHeaderModel::isSettingsLoaded() const
{
    return d_ptr->settingsLoaded;
}

void PlayListHeaderModel::insert(int index, const QString &name, const QString &pattern)
{
    Q_D(PlayListHeaderModel);
    if(index < 0 || index > d->columns.size())
    {
        qCWarning(core, "index is out of range");
        return;
    }

    PlayListHeaderModelPrivate::ColumnHeader col;
    col.name = name;
    col.pattern = pattern;
    d->columns.insert(index, col);
    emit columnAdded(index);
    emit headerChanged();
    d->updatePlayLists();
}

void PlayListHeaderModel::remove(int index)
{
    Q_D(PlayListHeaderModel);
    if(index < 0 || index >= d->columns.size())
    {
        qCWarning(core, "index is out of range");
        return;
    }

    if(d->columns.count() == 1)
        return;

    d->columns.takeAt(index);
    emit columnRemoved(index);
    emit headerChanged();
    d->updatePlayLists();
}

void PlayListHeaderModel::move(int from, int to)
{
    Q_D(PlayListHeaderModel);
    if(from < 0 || from >= d->columns.size())
    {
        qCWarning(core, "index is out of range");
        return;
    }

    if(to < 0 || to >= d->columns.size())
    {
        qCWarning(core, "index is out of range");
        return;
    }

    d->columns.move(from, to);
    emit columnMoved(from, to);
    emit headerChanged();
    d->updatePlayLists();
}

void PlayListHeaderModel::execEdit(int index, QWidget *parent)
{
    Q_D(PlayListHeaderModel);
    if(index < 0 || index >= d->columns.size())
    {
        qCWarning(core, "index is out of range");
        return;
    }

    if(!parent)
        parent = qApp->activeWindow();

    ColumnEditor editor(d->columns[index].name, d->columns[index].pattern, parent);
    if(editor.exec() == QDialog::Accepted)
    {
        d->columns[index].name = editor.name();
        d->columns[index].pattern = editor.pattern();
        emit columnChanged(index);
        emit headerChanged();
        d->updatePlayLists();
    }
}

void PlayListHeaderModel::execInsert(int index, QWidget *parent)
{
    Q_D(PlayListHeaderModel);
    if(index < 0 || index > d->columns.size())
    {
        qCWarning(core, "index is out of range");
        return;
    }

    if(!parent)
        parent = qApp->activeWindow();

    ColumnEditor editor(tr("Title"), u"%t"_s, parent);
    editor.setWindowTitle(tr("Add Column"));
    if(editor.exec() == QDialog::Accepted)
        insert(index, editor.name(), editor.pattern());
}

int PlayListHeaderModel::count() const
{
    return d_ptr->columns.count();
}

QString PlayListHeaderModel::name(int index) const
{
    Q_D(const PlayListHeaderModel);
    if(index < 0 || index >= d->columns.size())
    {
        qCWarning(core, "index is out of range");
        return QString();
    }
    return d->columns[index].name;
}
QString PlayListHeaderModel::pattern(int index) const
{
    Q_D(const PlayListHeaderModel);
    if(index < 0 || index >= d->columns.size())
    {
        qCWarning(core, "index is out of range");
        return QString();
    }
    return d->columns[index].pattern;
}

void PlayListHeaderModel::setData(int index, int key, const QVariant &data)
{
    Q_D(PlayListHeaderModel);
    if(index < 0 || index >= d->columns.size())
    {
        qCWarning(core, "index is out of range");
        return;
    }
    d->columns[index].data.insert(key, data);
}

QVariant PlayListHeaderModel::data(int index, int key) const
{
    Q_D(const PlayListHeaderModel);
    if(index < 0 || index >= d->columns.size())
    {
        qCWarning(core, "index is out of range");
        return QString();
    }
    return d->columns[index].data.value(key);
}


