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

#include <QFile>
#include <QTextStream>
#include <qmmp/qmmp.h>
#include "ui_aboutqsuidialog.h"
#include "aboutqsuidialog.h"

AboutQSUIDialog::AboutQSUIDialog(QWidget *parent) :
    QDialog(parent), m_ui(new Ui::AboutQSUIDialog)
{
    m_ui->setupUi(this);
    m_ui->aboutTextEdit->setHtml(loadAbout());
}

AboutQSUIDialog::~AboutQSUIDialog()
{
    delete m_ui;
}

QString AboutQSUIDialog::loadAbout()
{
    QString text = u"<head><META content=\"text/html; charset=UTF-8\"></head>"_s;
    text.append(u"<h3>"_s + tr("Qmmp Simple User Interface (QSUI)") + u"</h3>"_s);
    text.append(tr("Qmmp version: <b>%1</b>").arg(Qmmp::strVersion()));
    text.append(u"<p>"_s + tr("Simple user interface based on standard widgets set.") + u"</p>"_s);
    text.append(u"<b>"_s + tr("Developers:") + u"</b>"_s);
    text.append(u"<p>"_s + tr("Ilya Kotov <forkotov02@ya.ru>") + u"</p>"_s);

    text.append(u"<b>"_s + tr("Translators:") + u"</b>"_s);
    text.append(u"<p>"_s + getStringFromResource(u":translators"_s).replace(u"<"_s, u"&lt;"_s)
                .replace(u">"_s, u"&gt;"_s).replace(u"\n"_s, u"<br>"_s) + u"</p>"_s);
    return text;
}

QString AboutQSUIDialog::getStringFromResource(const QString& res_file)
{
    QString ret_string;
    QStringList paths;
    paths << res_file + QLatin1Char('_') + Qmmp::systemLanguageID() + u".txt"_s;
    if(Qmmp::systemLanguageID().contains(QLatin1Char('.')))
        paths << res_file + QLatin1Char('_') + Qmmp::systemLanguageID().split(QLatin1Char('.')).at(0) + u".txt"_s;
    if(Qmmp::systemLanguageID().contains(QLatin1Char('_')))
        paths << res_file + QLatin1Char('_') + Qmmp::systemLanguageID().split(QLatin1Char('_')).at(0) + u".txt"_s;
    paths << res_file + u".txt"_s << res_file;

    for(const QString &path : std::as_const(paths))
    {
        QFile file(path);
        if (file.open(QIODevice::ReadOnly))
        {
            QTextStream ts(&file);
            ret_string = ts.readAll();
            file.close();
            return ret_string;
        }
    }
    return ret_string;
}
