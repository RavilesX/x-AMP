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
#include <QOpenGLVersionFunctionsFactory>
#include <QOpenGLFunctions_2_0>
#include <qmmp/soundcore.h>
#include <qmmp/qmmp.h>
#include <qmmp/visual.h>
#include "projectmwrapper.h"
#include "projectmwidget.h"

#ifndef PROJECTM_CONFIG
#define PROJECTM_CONFIG "/usr/share/projectM/config.inp"
#endif

ProjectMWidget::ProjectMWidget(QListWidget *listWidget, QWidget *parent)
        : QOpenGLWidget(parent)
{
    setMouseTracking(true);
    m_listWidget = listWidget;
    m_menu = new QMenu(this);
    connect(SoundCore::instance(), &SoundCore::trackInfoChanged, this, &ProjectMWidget::updateTitle);
    createActions();
}

ProjectMWidget::~ProjectMWidget()
{}

void ProjectMWidget::addPCM(float *left, float *right)
{
    if(!m_projectM)
        return;

    for(size_t i = 0; i < qMin(512, QMMP_VISUAL_NODE_SIZE); i++)
    {
        m_buf[0][i] = left[i] * 32767.0;
        m_buf[1][i] = right[i] * 32767.0;
    }
    m_projectM->pcm()->addPCM16(m_buf);
}

void ProjectMWidget::initializeGL()
{
    QOpenGLFunctions_2_0 *f = QOpenGLVersionFunctionsFactory::get<QOpenGLFunctions_2_0>(context());
    f->glShadeModel(GL_SMOOTH);
    f->glClearColor(0,0,0,0);
    // Setup our viewport
    f->glViewport(0, 0, width(), height());
    // Change to the projection matrix and set our viewing volume.
    f->glMatrixMode(GL_TEXTURE);
    f->glLoadIdentity();
    f->glMatrixMode(GL_PROJECTION);
    f->glLoadIdentity();
    f->glMatrixMode(GL_MODELVIEW);
    f->glLoadIdentity();
    f->glDrawBuffer(GL_BACK);
    f->glReadBuffer(GL_BACK);
    f->glEnable(GL_BLEND);
    f->glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    f->glEnable(GL_LINE_SMOOTH);
    f->glEnable(GL_POINT_SMOOTH);
    f->glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    f->glLineStipple(2, 0xAAAA);

    if (!m_projectM)
    {
#ifdef Q_OS_WIN
        projectM::Settings settings;
        settings.meshX = 32;
        settings.meshY = 24;
        settings.fps = 35;
        settings.textureSize = 1024;
        settings.windowWidth = 512;
        settings.windowHeight = 512;
        settings.presetURL = QString(qApp->applicationDirPath() + u"/../share/projectM/presets"_s).toLocal8Bit().constData();
        settings.titleFontURL = QString(qApp->applicationDirPath() + u"/../share/projectM/fonts/Vera.ttf"_s).toLocal8Bit().constData();
        settings.menuFontURL = QString(qApp->applicationDirPath() + u"/../share/projectM/fonts/VeraMono.ttf"_s).toLocal8Bit().constData();
        settings.smoothPresetDuration = 5;
        settings.presetDuration = 30;
        settings.beatSensitivity = 1.0;
        settings.aspectCorrection = true;
        settings.easterEgg = 1.0;
        settings.shuffleEnabled = false;
        settings.softCutRatingsEnabled = false;
        m_projectM = new ProjectMWrapper(settings, projectM::FLAG_DISABLE_PLAYLIST_LOAD, this);
#else
        m_projectM = new ProjectMWrapper(PROJECTM_CONFIG, projectM::FLAG_DISABLE_PLAYLIST_LOAD, this);
#endif
        findPresets(QString::fromLocal8Bit(m_projectM->settings().presetURL.c_str()));

        connect(m_listWidget, &QListWidget::currentRowChanged, m_projectM, &ProjectMWrapper::selectPreset);
        connect(m_projectM, &ProjectMWrapper::currentPresetChanged, this, &ProjectMWidget::setCurrentRow);
        updateTitle();
    }
}

void ProjectMWidget::resizeGL(int w, int h)
{
    if(m_projectM)
    {
        m_projectM->projectM_resetGL(w, h);
        initializeGL();
    }
}

void ProjectMWidget::paintGL()
{
    if(m_projectM)
        m_projectM->renderFrame();
}

void ProjectMWidget::mousePressEvent (QMouseEvent *event)
{
    if(event->button () == Qt::RightButton)
        m_menu->exec(event->globalPosition().toPoint());
}

void ProjectMWidget::createActions()
{
#if QT_VERSION < QT_VERSION_CHECK(6, 3, 0)
    m_menu->addAction(tr("&Help"), this, &ProjectMWidget::showHelp, tr("F1"))->setCheckable(true);
    m_menu->addAction(tr("&Show Song Title"), this, &ProjectMWidget::showTitle, tr("F2"))->setCheckable(true);
    m_menu->addAction(tr("&Show Preset Name"), this, &ProjectMWidget::showPresetName, tr("F3"))->setCheckable(true);
    m_menu->addAction(tr("&Show Menu"), this, &ProjectMWidget::showMenuToggled, tr("M"))->setCheckable(true);
    m_menu->addSeparator();
    m_menu->addAction(tr("&Next Preset"), this, &ProjectMWidget::nextPreset, tr("N"));
    m_menu->addAction(tr("&Previous Preset"),  this, &ProjectMWidget::previousPreset, tr("P"));
    m_menu->addAction(tr("&Random Preset"), this, &ProjectMWidget::randomPreset, tr("R"));
    m_menu->addAction(tr("&Lock Preset"), this, &ProjectMWidget::lockPreset, tr("L"))->setCheckable(true);
    m_menu->addSeparator();
    m_menu->addAction(tr("&Fullscreen"), this, &ProjectMWidget::fullscreenToggled, tr("F"))->setCheckable(true);
#else
    m_menu->addAction(tr("&Help"), tr("F1"), this, &ProjectMWidget::showHelp)->setCheckable(true);
    m_menu->addAction(tr("&Show Song Title"), tr("F2"), this, &ProjectMWidget::showTitle)->setCheckable(true);
    m_menu->addAction(tr("&Show Preset Name"), tr("F3"), this, &ProjectMWidget::showPresetName)->setCheckable(true);
    m_menu->addAction(tr("&Show Menu"), tr("M"), this, &ProjectMWidget::showMenuToggled)->setCheckable(true);
    m_menu->addSeparator();
    m_menu->addAction(tr("&Next Preset"), tr("N"), this, &ProjectMWidget::nextPreset);
    m_menu->addAction(tr("&Previous Preset"),  tr("P"), this, &ProjectMWidget::previousPreset);
    m_menu->addAction(tr("&Random Preset"), tr("R"), this, &ProjectMWidget::randomPreset);
    m_menu->addAction(tr("&Lock Preset"), tr("L"), this, &ProjectMWidget::lockPreset)->setCheckable(true);
    m_menu->addSeparator();
    m_menu->addAction(tr("&Fullscreen"), tr("F"), this, &ProjectMWidget::fullscreenToggled)->setCheckable(true);
#endif
    m_menu->addSeparator();
    addActions(m_menu->actions());
}

void ProjectMWidget::findPresets(const QString &path)
{
    QDir presetDir(path);
    presetDir.setFilter(QDir::Files);
    const QFileInfoList files = presetDir.entryInfoList({ u"*.prjm"_s, u"*.milk"_s }, QDir::Files);

    const RatingList list = { 3, 3 };
    for(const QFileInfo &info : std::as_const(files))
    {
        m_projectM->addPresetURL(info.absoluteFilePath().toStdString(), info.fileName().toStdString(), list);
        m_listWidget->addItem(info.fileName());
        m_listWidget->setCurrentRow(0,QItemSelectionModel::Select);
    }

    const QFileInfoList dirs = presetDir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot);
    for(const QFileInfo &info : std::as_const(dirs))
        findPresets(info.canonicalFilePath());
}

void ProjectMWidget::showHelp()
{
    m_projectM->key_handler(PROJECTM_KEYDOWN, PROJECTM_K_F1, PROJECTM_KMOD_LSHIFT);
}

void ProjectMWidget::showPresetName()
{
    m_projectM->key_handler(PROJECTM_KEYDOWN, PROJECTM_K_F3, PROJECTM_KMOD_LSHIFT);
}

void ProjectMWidget::showTitle()
{
    m_projectM->key_handler(PROJECTM_KEYDOWN, PROJECTM_K_F2, PROJECTM_KMOD_LSHIFT);
}

void ProjectMWidget::nextPreset()
{
    m_projectM->key_handler(PROJECTM_KEYDOWN, PROJECTM_K_n, PROJECTM_KMOD_LSHIFT);
}

void ProjectMWidget::previousPreset()
{
    m_projectM->key_handler(PROJECTM_KEYDOWN, PROJECTM_K_p, PROJECTM_KMOD_LSHIFT);
}

void ProjectMWidget::randomPreset()
{
    m_projectM->key_handler(PROJECTM_KEYDOWN, PROJECTM_K_r, PROJECTM_KMOD_LSHIFT);
}

void ProjectMWidget::lockPreset(bool lock)
{
    m_projectM->setPresetLock(lock);
}

void ProjectMWidget::updateTitle()
{
    std::string artist = SoundCore::instance()->metaData(Qmmp::ARTIST).toLocal8Bit().constData();
    std::string title = SoundCore::instance()->metaData(Qmmp::TITLE).toLocal8Bit().constData();
    m_projectM->projectM_setTitle(artist + " - " + title);
}

void ProjectMWidget::setCurrentRow(int row)
{
    m_listWidget->setCurrentRow(row);
}
