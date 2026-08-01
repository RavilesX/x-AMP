/***************************************************************************
 *   Copyright (C) 2009-2026 by Ilya Kotov                                 *
 *   forkotov02@ya.ru                                                      *
 *                                                                         *
 *   Copyright (C) 2007 by  projectM team                                  *
 *                                                                         *
 *   Carmelo Piccione  carmelo.piccione+projectM@gmail.com                 *
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
#include <QKeyEvent>
#include <QMenu>
#include <QApplication>
#include <QListWidget>
#include <QOpenGLContext>
#include <projectM-4/projectM.h>
#include <projectM-4/playlist.h>
#include <qmmp/soundcore.h>
#include <qmmp/qmmp.h>
#include "projectm4widget.h"

#ifndef PROJECTM_PRESET_PATH
#define PROJECTM_PRESET_PATH "/usr/share/projectM/presets"
#endif

#ifndef PROJECTM_TEXTURE_PATH
#define PROJECTM_TEXTURE_PATH "/usr/share/projectM/textures"
#endif


ProjectM4Widget::ProjectM4Widget(QListWidget *listWidget, QWidget *parent)
        : QOpenGLWidget(parent)
{
    setMouseTracking(true);
    m_listWidget = listWidget;
    m_menu = new QMenu(this);
    createActions();
}

ProjectM4Widget::~ProjectM4Widget()
{
    if(m_playlistHandle)
    {
        projectm_playlist_destroy(m_playlistHandle);
        m_playlistHandle = nullptr;
    }

    if(m_handle)
    {
        projectm_destroy(m_handle);
        m_handle = nullptr;
    }
}

projectm_handle ProjectM4Widget::handle()
{
    return m_handle;
}

void ProjectM4Widget::addPCM(float *left, float *right)
{
    for(size_t i = 0; i < QMMP_VISUAL_NODE_SIZE; ++i)
    {
        m_buf[i * 2] = left[i];
        m_buf[i * 2 + 1] = right[i];
    }
    projectm_pcm_add_float(m_handle, m_buf, QMMP_VISUAL_NODE_SIZE, PROJECTM_STEREO);
}

void ProjectM4Widget::initializeGL()
{
    glShadeModel(GL_SMOOTH);
    glClearColor(0,0,0,0);
    // Setup our viewport
    glViewport(0, 0, width(), height());
    // Change to the projection matrix and set our viewing volume.
    glMatrixMode(GL_TEXTURE);
    glLoadIdentity();
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glDrawBuffer(GL_BACK);
    glReadBuffer(GL_BACK);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_LINE_SMOOTH);
    glEnable(GL_POINT_SMOOTH);
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glLineStipple(2, 0xAAAA);

    if(!m_handle)
    {
        m_handle = projectm_create();
        if(!m_handle)
        {
            qCDebug(plugin, "unable to initialize GL widget");
            return;
        }

        const char *texture_paths[] = { PROJECTM_TEXTURE_PATH };
        projectm_set_texture_search_paths(m_handle, texture_paths, 1);
        projectm_set_fps(m_handle, 60);
        projectm_set_mesh_size(m_handle, 220, 125);
        projectm_set_aspect_correction(m_handle, true);
        projectm_set_preset_duration(m_handle, 10.0);
        projectm_set_soft_cut_duration(m_handle, 3.0);
        projectm_set_hard_cut_enabled(m_handle, true);
        projectm_set_hard_cut_sensitivity(m_handle, 1.0);
        projectm_set_beat_sensitivity(m_handle, 1.0);
        projectm_set_easter_egg(m_handle, 1.0);

        m_playlistHandle = projectm_playlist_create(m_handle);
        projectm_playlist_set_shuffle(m_playlistHandle, false);
        projectm_playlist_set_preset_switched_event_callback(m_playlistHandle, &ProjectM4Widget::presetSwitchedEvent, this);
        findPresets(QStringLiteral(PROJECTM_PRESET_PATH));
        connect(m_listWidget, &QListWidget::currentRowChanged, this, &ProjectM4Widget::selectPreset);
    }
}

void ProjectM4Widget::resizeGL(int w, int h)
{
    if(m_handle)
    {
        initializeGL();
        projectm_set_window_size(m_handle, w, h);
    }
}

void ProjectM4Widget::paintGL()
{
    if(m_handle)
    {
        glClearColor(0.0, 0.0, 0.0, 0.0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        projectm_opengl_render_frame(m_handle);
    }
}

void ProjectM4Widget::mousePressEvent (QMouseEvent *event)
{
    if(event->button() == Qt::RightButton)
        m_menu->exec(event->globalPosition().toPoint());
}

void ProjectM4Widget::createActions()
{
    m_menu->addAction(tr("&Show Menu"), tr("M"), this, &ProjectM4Widget::showMenuToggled)->setCheckable(true);
    m_menu->addSeparator();
    m_menu->addAction(tr("&Next Preset"), tr("N"), this, &ProjectM4Widget::nextPreset);
    m_menu->addAction(tr("&Previous Preset"), tr("P"), this, &ProjectM4Widget::previousPreset);
    m_menu->addAction(tr("&Shuffle"), tr("R"), this, &ProjectM4Widget::setShuffle)->setCheckable(true);
    m_menu->addAction(tr("&Lock Preset"), tr("L"), this, &ProjectM4Widget::lockPreset)->setCheckable(true);
    m_menu->addSeparator();
    m_menu->addAction(tr("&Fullscreen"), tr("F"), this, &ProjectM4Widget::fullscreenToggled)->setCheckable(true);
    m_menu->addSeparator();
    addActions(m_menu->actions());
}

void ProjectM4Widget::findPresets(const QString &path)
{
    QDir presetDir(path);
    presetDir.setFilter(QDir::Files);
    const QFileInfoList files = presetDir.entryInfoList({ "*.prjm", "*.milk" }, QDir::Files);

    for(const QFileInfo &info : std::as_const(files))
    {
        projectm_playlist_add_preset(m_playlistHandle, qPrintable(info.canonicalFilePath()), false);
        m_listWidget->addItem(info.fileName());
        m_listWidget->setCurrentRow(0, QItemSelectionModel::Select);
    }

    const QFileInfoList dirs = presetDir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot);
    for(const QFileInfo &info : std::as_const(dirs))
        findPresets(info.canonicalFilePath());
}

void ProjectM4Widget::nextPreset()
{
    if(m_playlistHandle)
        projectm_playlist_play_next(m_playlistHandle, true);
}

void ProjectM4Widget::previousPreset()
{
    if(m_playlistHandle)
        projectm_playlist_play_previous(m_playlistHandle, true);
}

void ProjectM4Widget::setShuffle(bool enabled)
{
    if(m_playlistHandle)
        projectm_playlist_set_shuffle(m_playlistHandle, enabled);
}

void ProjectM4Widget::lockPreset(bool lock)
{
    if(m_handle)
        projectm_set_preset_locked(m_handle, lock);
}

void ProjectM4Widget::setCurrentRow(int row)
{
    m_listWidget->setCurrentRow(row);
}

void ProjectM4Widget::selectPreset(int index)
{
    if(m_playlistHandle)
        projectm_playlist_set_position(m_playlistHandle, index, true);
}

void ProjectM4Widget::presetSwitchedEvent(bool isHardCut, unsigned int index, void *data)
{
    Q_UNUSED(isHardCut);
    static_cast<ProjectM4Widget *>(data)->m_listWidget->setCurrentRow(index);
}
