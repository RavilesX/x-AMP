/***************************************************************************
 *   Copyright (C) 2012-2026 by Ilya Kotov                                 *
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
#include <QApplication>
#include <qmmp/qmmp.h>
#include "metadatahelper_p.h"
#include "playlistmanager.h"
#include "qmmpuisettings.h"

class QmmpUiSettingsPrivate
{
    Q_DECLARE_PUBLIC(QmmpUiSettings)
    Q_DECLARE_TR_FUNCTIONS(QmmpUiSettings)
public:
    QmmpUiSettingsPrivate(QmmpUiSettings *qu) : q_ptr(qu)
    {
        helper = new MetaDataHelper;
        QSettings s;
        s.beginGroup(u"PlayList"_s);
        groupFormat = s.value(u"group_format"_s, u"%p%if(%p&%a, - %if(%y,[%y] ,),)%a"_s).toString();
        groupExtraRowFormat = s.value(u"group_extra_row_format"_s, tr("%if(%l,%l | ,)%{format} | %{bitrate} kbps | %{samplerate} Hz")).toString();
        linesPerGroup = s.value(u"lines_per_group"_s, 1).toInt();
        groupExtraRowVisible = s.value(u"group_extra_row_visible"_s, true).toBool();
        groupCoverVisible = s.value(u"group_cover_visible"_s, true).toBool();
        groupDividingLineVisible = s.value(u"group_dividing_line_visible"_s, true).toBool();
        convertUnderscore = s.value (u"convert_underscore"_s, true).toBool();
        convertTwenty = s.value(u"convert_twenty"_s, true).toBool();
        useMetadata = s.value(u"load_metadata"_s, true).toBool();
        autosavePlaylist = s.value(u"autosave"_s, true).toBool();
        repeateList = s.value(u"repeate_list"_s, false).toBool();
        shuffle = s.value(u"shuffle"_s, false).toBool();
        groupsEnabled = s.value(u"groups"_s, false).toBool();
        repeatTrack = s.value(u"repeate_track"_s, false).toBool();
        noPlAdvance = s.value(u"no_advance"_s, false).toBool();
        clearPrevPlaylist = s.value(u"clear_previous"_s, false).toBool();
        readMetadataForPlaylist = s.value(u"read_metadata_for_playlist"_s, true).toBool();
        transitBetweenPlaylists = s.value(u"transit_between_playlists"_s, false).toBool();
        skipExistingTracks = s.value(u"skip_existing_tracks"_s, false).toBool();
        stopAfterRemovingOfCurrent = s.value(u"stop_after_removing_of_current"_s, false).toBool();
        s.endGroup();
        s.beginGroup(u"General"_s);
        resumeOnStartup = s.value(u"resume_on_startup"_s, false).toBool();
        restrictFilters = s.value(u"restrict_filters"_s).toStringList();
        excludeFilters = s.value(u"exclude_filters"_s).toStringList();
        useDefaultPl = s.value(u"use_default_pl"_s, false).toBool();
        defaultPlName = s.value(u"default_pl_name"_s, tr("Playlist")).toString();
        s.endGroup();
        useClipboard = s.value(u"URLDialog/use_clipboard"_s, false).toBool();

        helper->setGroupFormat0(groupFormat);
        helper->setGroupFormat1(groupExtraRowFormat);
    }

    ~QmmpUiSettingsPrivate()
    {
        instance = nullptr;
        sync();
        delete helper;
    }

    void saveSettings(bool rebuildGroups = false)
    {
        saveSettingsRequest = true;

        if(rebuildGroups)
            rebuildGroupsRequest = true;

        QMetaObject::invokeMethod(q_ptr, [this] { sync(); }, Qt::QueuedConnection);
    }

    void sync()
    {
        if(saveSettingsRequest)
        {
            qCDebug(core) << "saving settings...";
            QSettings s;
            s.setValue(u"PlayList/group_format"_s, groupFormat);
            s.setValue(u"PlayList/group_extra_row_format"_s, groupExtraRowFormat);
            s.setValue(u"PlayList/lines_per_group"_s, linesPerGroup);
            s.setValue(u"PlayList/group_extra_row_visible"_s, groupExtraRowVisible);
            s.setValue(u"PlayList/group_cover_visible"_s, groupCoverVisible);
            s.setValue(u"PlayList/group_dividing_line_visible"_s, groupDividingLineVisible);
            s.setValue(u"PlayList/convert_underscore"_s, convertUnderscore);
            s.setValue(u"PlayList/convert_twenty"_s, convertTwenty);
            s.setValue(u"PlayList/load_metadata"_s, useMetadata);
            s.setValue(u"PlayList/autosave"_s, autosavePlaylist);
            s.setValue(u"PlayList/repeate_list"_s, repeateList);
            s.setValue(u"PlayList/shuffle"_s, shuffle);
            s.setValue(u"PlayList/groups"_s, groupsEnabled);
            s.setValue(u"PlayList/repeate_track"_s, repeatTrack);
            s.setValue(u"PlayList/no_advance"_s, noPlAdvance);
            s.setValue(u"PlayList/clear_previous"_s, clearPrevPlaylist);
            s.setValue(u"PlayList/read_metadata_for_playlist"_s, readMetadataForPlaylist);
            s.setValue(u"PlayList/transit_between_playlists"_s, transitBetweenPlaylists);
            s.setValue(u"PlayList/skip_existing_tracks"_s, skipExistingTracks);
            s.setValue(u"PlayList/stop_after_removing_of_current"_s, stopAfterRemovingOfCurrent);
            s.setValue(u"General/resume_on_startup"_s, resumeOnStartup);
            s.setValue(u"General/restrict_filters"_s, restrictFilters);
            s.setValue(u"General/exclude_filters"_s, excludeFilters);
            s.setValue(u"General/use_default_pl"_s, useDefaultPl);
            s.setValue(u"General/default_pl_name"_s, defaultPlName);
            s.setValue(u"URLDialog/use_clipboard"_s, useClipboard);
            saveSettingsRequest = false; //protect from multiple calls
        }

        if(rebuildGroupsRequest)
        {
            qCDebug(core) << "rebuilding groups...";
            PlayListManager::instance()->rebuildGroups();
            rebuildGroupsRequest = false; //protect from multiple calls
        }
    }

    void setHelper(MetaDataHelper *newHelper);

private:
    QmmpUiSettings *q_ptr;

    static QmmpUiSettings *instance;
    //playlist
    QString groupFormat;
    QString groupExtraRowFormat;
    int linesPerGroup;
    bool groupExtraRowVisible;
    bool groupCoverVisible;
    bool groupDividingLineVisible;
    bool convertUnderscore, convertTwenty;
    bool useMetadata;
    bool autosavePlaylist;
    bool repeateList;
    bool shuffle;
    bool groupsEnabled;
    bool repeatTrack;
    bool noPlAdvance;
    bool clearPrevPlaylist;
    bool readMetadataForPlaylist;
    bool transitBetweenPlaylists;
    bool skipExistingTracks;
    bool stopAfterRemovingOfCurrent;
    //general
    bool resumeOnStartup;
    QStringList excludeFilters, restrictFilters;
    //default playlist
    bool useDefaultPl;
    QString defaultPlName;
    //url dialog
    bool useClipboard;
    //formatters
    MetaDataHelper *helper;
    //protect from multiple calls
    bool saveSettingsRequest = false;
    bool rebuildGroupsRequest = false;
};

QmmpUiSettings *QmmpUiSettingsPrivate::instance = nullptr;

QmmpUiSettings::QmmpUiSettings(QObject *parent) :
    QObject(parent),
    d_ptr(new QmmpUiSettingsPrivate(this))
{
    if(QmmpUiSettingsPrivate::instance)
        qCFatal(core) << "only one instance is allowed";
    QmmpUiSettingsPrivate::instance = this;
}

QmmpUiSettings::~QmmpUiSettings()
{
    delete d_ptr;
}

QString QmmpUiSettings::groupFormat() const
{
    return d_ptr->groupFormat;
}

QString QmmpUiSettings::groupExtraRowFormat() const
{
    return d_ptr->groupExtraRowFormat;
}

int QmmpUiSettings::linesPerGroup() const
{
    return d_ptr->linesPerGroup;
}

bool QmmpUiSettings::groupExtraRowVisible() const
{
    return d_ptr->groupExtraRowVisible;
}

bool QmmpUiSettings::groupCoverVisible() const
{
    return d_ptr->groupCoverVisible;
}

bool QmmpUiSettings::groupDividingLineVisible() const
{
    return d_ptr->groupDividingLineVisible;
}

bool QmmpUiSettings::isRepeatableList() const
{
    return d_ptr->repeateList;
}

bool QmmpUiSettings::isShuffle() const
{
    return d_ptr->shuffle;
}

bool QmmpUiSettings::isGroupsEnabled() const
{
    return d_ptr->groupsEnabled;
}

bool QmmpUiSettings::isRepeatableTrack() const
{
    return d_ptr->repeatTrack;
}

bool QmmpUiSettings::isNoPlayListAdvance() const
{
    return d_ptr->noPlAdvance;
}

bool QmmpUiSettings::isPlayListTransitionEnabled() const
{
    return d_ptr->transitBetweenPlaylists;
}

bool QmmpUiSettings::convertUnderscore() const
{
    return d_ptr->convertUnderscore;
}

bool QmmpUiSettings::convertTwenty() const
{
    return d_ptr->convertTwenty;
}

bool QmmpUiSettings::useMetaData() const
{
    return d_ptr->useMetadata;
}

void QmmpUiSettings::setConvertUnderscore(bool yes)
{
    d_ptr->convertUnderscore = yes;
    d_ptr->saveSettings();
}

void  QmmpUiSettings::setConvertTwenty(bool yes)
{
    d_ptr->convertTwenty = yes;
    d_ptr->saveSettings();
}

void QmmpUiSettings::setGroupFormat(const QString &groupFormat)
{
    Q_D(QmmpUiSettings);
    if(groupFormat != d->groupFormat)
    {
        d->groupFormat = groupFormat;
        d->helper->setGroupFormat0(d->groupFormat);
        d->saveSettings(true);
    }
}

void QmmpUiSettings::setGroupExtraRowFormat(const QString &extraRowFormat)
{
    Q_D(QmmpUiSettings);
    if(d->groupExtraRowFormat != extraRowFormat)
    {
        d->groupExtraRowFormat = extraRowFormat;
        d->helper->setGroupFormat1(d->groupExtraRowFormat);
        d->saveSettings(true);
    }
}

void QmmpUiSettings::setLinesPerGroup(int lines)
{
    Q_D(QmmpUiSettings);
    if(d->linesPerGroup != lines)
    {
        d->linesPerGroup = lines;
        d->saveSettings(true);
    }
}

void QmmpUiSettings::setGroupExtraRowVisible(bool enabled)
{
    Q_D(QmmpUiSettings);
    if(d->groupExtraRowVisible != enabled)
    {
        d->groupExtraRowVisible = enabled;
        d->saveSettings(true);
    }
}

void QmmpUiSettings::setGroupCoverVisible(bool enabled)
{
    Q_D(QmmpUiSettings);
    if(d->groupCoverVisible != enabled)
    {
        d->groupCoverVisible = enabled;
        d->saveSettings(true);
    }
}

void QmmpUiSettings::setGroupDividingLineVisible(bool enabled)
{
    Q_D(QmmpUiSettings);
    if(d->groupDividingLineVisible != enabled)
    {
        d->groupDividingLineVisible = enabled;
        d->saveSettings(true);
    }
}

void QmmpUiSettings::setUseMetaData(bool yes)
{
    Q_D(QmmpUiSettings);
    d->useMetadata = yes;
    d->saveSettings();
}

bool QmmpUiSettings::resumeOnStartup() const
{
    return d_ptr->resumeOnStartup;
}

void QmmpUiSettings::setResumeOnStartup(bool enabled)
{
    Q_D(QmmpUiSettings);
    d->resumeOnStartup = enabled;
    d->saveSettings();
}

void QmmpUiSettings::setUseClipboard(bool enabled)
{
    Q_D(QmmpUiSettings);
    d->useClipboard = enabled;
    d->saveSettings();
}

bool QmmpUiSettings::useClipboard() const
{
    return d_ptr->useClipboard;
}

void QmmpUiSettings::setRepeatableList(bool r)
{
    Q_D(QmmpUiSettings);
    if(d->repeateList != r)
    {
        d->repeateList = r;
        d->saveSettings();
        emit repeatableListChanged(r);
    }
}

void QmmpUiSettings::setShuffle(bool s)
{
    Q_D(QmmpUiSettings);
    if(d->shuffle != s)
    {
        d->shuffle = s;
        d->saveSettings();
        emit shuffleChanged(s);
    }
}

void QmmpUiSettings::setGroupsEnabled(bool enabled)
{
    Q_D(QmmpUiSettings);
    if(d->groupsEnabled != enabled)
    {
        d->groupsEnabled = enabled;
        d->saveSettings();
        emit groupsChanged(enabled);
    }
}

void QmmpUiSettings::setRepeatableTrack(bool enabled)
{
    Q_D(QmmpUiSettings);
    if(d->repeatTrack != enabled)
    {
        d->repeatTrack = enabled;
        d->saveSettings();
        emit repeatableTrackChanged(enabled);
    }
}

void QmmpUiSettings::setNoPlayListAdvance(bool enabled)
{
    Q_D(QmmpUiSettings);
    if(d->noPlAdvance != enabled)
    {
        d->noPlAdvance = enabled;
        d->saveSettings();
        emit noPlayListAdvanceChanged(enabled);
    }
}

void QmmpUiSettings::setPlayListTransitionEnabled(bool enabled)
{
    Q_D(QmmpUiSettings);
    if(d->transitBetweenPlaylists != enabled)
    {
        d->transitBetweenPlaylists = enabled;
        d->saveSettings();
        emit playListTransitionChanged(enabled);
    }
}

QStringList QmmpUiSettings::restrictFilters() const
{
    return d_ptr->restrictFilters;
}

void QmmpUiSettings::setRestrictFilters(const QString &filters)
{
    Q_D(QmmpUiSettings);
    d->restrictFilters = filters.trimmed().split(QLatin1Char(','), Qt::SkipEmptyParts);
    d->saveSettings();
}

QStringList QmmpUiSettings::excludeFilters() const
{
    return d_ptr->excludeFilters;
}

void QmmpUiSettings::setExcludeFilters(const QString &filters)
{
    Q_D(QmmpUiSettings);
    d->excludeFilters = filters.trimmed().split(QLatin1Char(','), Qt::SkipEmptyParts);
    d->saveSettings();
}

bool QmmpUiSettings::useDefaultPlayList() const
{
    return d_ptr->useDefaultPl;
}

QString QmmpUiSettings::defaultPlayListName() const
{
    return d_ptr->defaultPlName;
}

QmmpUiSettings *QmmpUiSettings::instance()
{
    if(!QmmpUiSettingsPrivate::instance)
        return new QmmpUiSettings(qApp);
    return QmmpUiSettingsPrivate::instance;
}

void QmmpUiSettings::setDefaultPlayList(const QString &name, bool enabled)
{
    Q_D(QmmpUiSettings);
    d->useDefaultPl = enabled;
    d->defaultPlName = name;
    d->saveSettings();
}

void QmmpUiSettings::setAutoSavePlayList(bool enabled)
{
    Q_D(QmmpUiSettings);
    d->autosavePlaylist = enabled;
    d->saveSettings();
}

bool QmmpUiSettings::autoSavePlayList() const
{
    return d_ptr->autosavePlaylist;
}

void QmmpUiSettings::setClearPreviousPlayList(bool enabled)
{
    Q_D(QmmpUiSettings);
    d->clearPrevPlaylist = enabled;
    d->saveSettings();
}

bool QmmpUiSettings::clearPreviousPlayList() const
{
    return d_ptr->clearPrevPlaylist;
}

void QmmpUiSettings::setSkipExistingTracks(bool enabled)
{
    Q_D(QmmpUiSettings);
    d->skipExistingTracks = enabled;
    d->saveSettings();
}

bool QmmpUiSettings::skipExistingTracks() const
{
    return d_ptr->skipExistingTracks;
}

bool QmmpUiSettings::stopAfterRemovingOfCurrentTrack() const
{
    return d_ptr->stopAfterRemovingOfCurrent;
}

void QmmpUiSettings::setStopAfterRemovingOfCurrentTrack(bool enabled)
{
    Q_D(QmmpUiSettings);
    d->stopAfterRemovingOfCurrent = enabled;
    d->saveSettings();
}

bool QmmpUiSettings::readMetaDataForPlayLists() const
{
    return d_ptr->readMetadataForPlaylist;
}

void QmmpUiSettings::setReadMetaDataForPlayLists(bool enabled)
{
    Q_D(QmmpUiSettings);
    d->readMetadataForPlaylist = enabled;
    d->saveSettings();
}

void QmmpUiSettingsPrivate::setHelper(MetaDataHelper *newHelper)
{
    helper = newHelper;
}
