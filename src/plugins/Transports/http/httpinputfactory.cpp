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

#include <QtPlugin>
#include <QMessageBox>
#include <curl/curlver.h>
#include <qmmp/qmmp.h>
#include "httpsettingsdialog.h"
#include "httpinputsource.h"
#include "httpinputfactory.h"

InputSourceProperties HTTPInputFactory::properties() const
{
    InputSourceProperties properties;
    properties.protocols = QStringList { u"http"_s, u"https"_s };
    properties.name = tr("HTTP Plugin");
    properties.shortName = "http"_L1;
    properties.hasAbout = true;
    properties.hasSettings = true;
    return properties;
}

InputSource *HTTPInputFactory::create(const QString &url, QObject *parent)
{
    return new HTTPInputSource(url, parent);
}

QDialog *HTTPInputFactory::createSettings(QWidget *parent)
{
    return new HttpSettingsDialog(parent);
}

void HTTPInputFactory::showAbout(QWidget *parent)
{
    QMessageBox::about(parent, tr("About HTTP Transport Plugin"),
                       tr("x-AMP HTTP Transport Plugin") + QChar::LineFeed +
                       tr("Compiled against libcurl-%1").arg(QLatin1StringView(LIBCURL_VERSION)) + QChar::LineFeed +
                       tr("Written by: Ilya Kotov <forkotov02@ya.ru>"));
}

QString HTTPInputFactory::translation() const
{
    return QLatin1String(":/http_plugin_");
}
