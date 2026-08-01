/***************************************************************************
 *   Copyright (C) 2017-2026 by Ilya Kotov                                 *
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
#include <QPixmap>
#include <QPainter>
#include <QPaintEvent>
#include <QAction>
#include <QSettings>
#include <QStandardPaths>
#include <QVariant>
#include <QDir>
#include <QFileInfo>
#include <qmmp/qmmp.h>
#include "filedialog.h"
#include "coverviewer_p.h"

CoverViewer::CoverViewer(QWidget *parent)
        : QWidget(parent)
{
    QAction *saveAsAction = new QAction(tr("&Save As..."), this);
    connect(saveAsAction, &QAction::triggered, this, &CoverViewer::saveAs);
    addAction(saveAsAction);
    setContextMenuPolicy(Qt::ActionsContextMenu);
    QSettings settings;
    m_lastDir = QStandardPaths::writableLocation(QStandardPaths::PicturesLocation);
    m_lastDir = settings.value(u"CoverEditor/last_dir"_s, m_lastDir).toString();
}

CoverViewer::~CoverViewer()
{
    QSettings settings;
    settings.setValue(u"CoverEditor/last_dir"_s, m_lastDir);
}

void CoverViewer::setImage(const QImage &img)
{
    m_image = img;
    update();
}

bool CoverViewer::hasImage() const
{
    return !m_image.isNull();
}

QImage CoverViewer::image() const
{
    return m_image;
}

void CoverViewer::saveAs()
{
    QString path = FileDialog::getSaveFileName(this, tr("Save Cover As"),
                                               m_lastDir + u"/cover.jpg"_s,
                                               tr("Images") + u" (*.png *.jpg)"_s);

    if (!path.isEmpty())
    {
        m_lastDir = QFileInfo(path).absoluteDir().path();
        m_image.save(path);
    }
}

void CoverViewer::load()
{
    QString path = FileDialog::getOpenFileName(this, tr("Open Image"),
                                               m_lastDir,
                                               tr("Images") + u" (*.png *.jpg)"_s);
    if(!path.isEmpty())
    {
        m_lastDir = QFileInfo(path).absoluteDir().path();
        m_image.load(path);
        if(m_image.width() > 512)
            m_image = m_image.scaled(512, 512, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    }
    update();
}

void CoverViewer::clear()
{
    m_image = QImage();
    update();
}

void CoverViewer::paintEvent(QPaintEvent *)
{
    if(!m_image.isNull())
    {
        QPainter paint(this);
        QImage image = m_image.scaled(size().width() - 10, size().height() - 10, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        paint.drawImage((width() - image.width()) / 2, (height() - image.height()) / 2, image);
    }
}
