/***************************************************************************
 *   Copyright (C) 2009-2026 by Ilya Kotov                                 *
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
#ifndef PROJECTM4WIDGET_H
#define PROJECTM4WIDGET_H

#include <QOpenGLWidget>
#include <projectM-4/types.h>
#include <projectM-4/playlist_core.h>
#include <qmmp/visual.h>

class QMenu;
class QTimer;
class QListWidget;

/**
    @author Ilya Kotov <forkotov02@ya.ru>
*/
class ProjectM4Widget : public QOpenGLWidget
{
    Q_OBJECT
public:
    explicit ProjectM4Widget(QListWidget *listWidget, QWidget *parent = nullptr);

    ~ProjectM4Widget();

    projectm_handle handle();
    void addPCM(float *left, float *right);

signals:
    void showMenuToggled(bool);
    void fullscreenToggled(bool);

protected:
    virtual void initializeGL() override;
    virtual void resizeGL(int width, int height) override;
    virtual void paintGL() override;
    virtual void mousePressEvent(QMouseEvent *event) override;

private slots:
    void nextPreset();
    void previousPreset();
    void setShuffle(bool enabled);
    void lockPreset(bool lock);
    void setCurrentRow(int row);
    void selectPreset(int index);

private:
    void createActions();
    void findPresets(const QString &path);
    static void presetSwitchedEvent(bool isHardCut, unsigned int index, void *data);

    projectm_handle m_handle = nullptr;
    projectm_playlist_handle m_playlistHandle = nullptr;
    QMenu *m_menu;
    QListWidget *m_listWidget;
    float m_buf[QMMP_VISUAL_NODE_SIZE * 2] = { 0 };
};

#endif
