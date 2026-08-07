/**************************************************************************
*   Copyright (C) 2006-2026 by Ilya Kotov                                 *
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
#include <QSysInfo>
#include <qmmp/decoder.h>
#include <qmmp/decoderfactory.h>
#include <qmmp/output.h>
#include <qmmp/outputfactory.h>
#include <qmmp/visual.h>
#include <qmmp/visualfactory.h>
#include <qmmp/effect.h>
#include <qmmp/effectfactory.h>
#include <qmmp/inputsource.h>
#include <qmmp/inputsourcefactory.h>
#include <qmmp/abstractengine.h>
#include <qmmp/enginefactory.h>
#include <qmmpui/filedialog.h>
#include <qmmpui/filedialogfactory.h>
#include <qmmpui/uiloader.h>
#include <qmmpui/uifactory.h>
#include <qmmp/qmmp.h>
#include "ui_aboutdialog.h"
#include "general.h"
#include "generalfactory.h"
#include "aboutdialog_p.h"


AboutDialog::AboutDialog(QWidget* parent)
        : QDialog(parent), m_ui(new Ui::AboutDialog)
{

    m_ui->setupUi(this);
    m_ui->licenseTextBrowser->setPlainText(getStringFromResource(u":COPYING"_s));
    m_ui->aboutTextBrowser->setHtml(loadAbout());
    m_ui->authorsTextBrowser->setPlainText(loadAuthors());
    m_ui->thanksToTextBrowser->setPlainText(getStringFromResource(u":thanks"_s));
    m_ui->translatorsTextBrowser->setPlainText(getStringFromResource(u":translators"_s));
}

AboutDialog::~AboutDialog()
{
    delete m_ui;
}

/*$SPECIALIZATION$*/
QString AboutDialog::loadAbout()
{
    QString text;
    text.append(u"<head><META content=\"text/html; charset=UTF-8\"></head>"_s);
    text.append(layoutDirection() == Qt::RightToLeft ? u"<div dir='rtl'>"_s : u"<div>"_s);
    text.append(u"<h3>"_s + tr("Qt-based Multimedia Player (x-AMP)") + u"</h3>"_s);
    text.append(u"<p>"_s + getStringFromResource(u":description"_s) + u"</p>"_s);

    text.append(u"<p><b>"_s + tr("Version: %1").arg(Qmmp::strVersion()) + u"</b><br>"_s);
    text.append(tr("Qt version: %1 (compiled with %2)").arg(QString::fromLatin1(qVersion()), QLatin1StringView(QT_VERSION_STR)) + u"<br>"_s);
    text.append(tr("Qt platform: %1").arg(QGuiApplication::platformName()) + u"<br>"_s);
    text.append(tr("System: %1").arg(QSysInfo::prettyProductName()) + u"<br>"_s);
    text.append(tr("Build ABI: %1").arg(QSysInfo::buildAbi()) + u"</p>"_s);

    //x-AMP: the fork's own identity first, then upstream's, so neither is
    //mistaken for the other. The upstream copyright line keeps its original
    //wording on purpose -- it is their attribution, not ours to reword, and
    //rewording it would orphan its translations.
    text.append(u"<p>"_s);
    text.append(tr("(c) %1 x-AMP contributors").arg(2026) + u"<br>"_s);
    //the name is a name: outside tr() so no translation can mangle it, and the
    //same goes for the addresses below it
    text.append(tr("Lead developer: %1").arg(u"Ricardo Aviles Sanders"_s) + u"<br>"_s);
    text.append(u"<a href=\"https://Ravilesx.org\">https://Ravilesx.org</a> &middot; "_s);
    text.append(u"<a href=\"https://github.com/RavilesX\">https://github.com/RavilesX</a><br>"_s);
    text.append(u"<a href=\"https://github.com/RavilesX/x-AMP\">https://github.com/RavilesX/x-AMP</a>"_s);
    text.append(u"</p>"_s);

    text.append(u"<p>"_s);
    text.append(tr("Based on Qmmp:") + u"<br>"_s);
    text.append(tr("(c) %1-%2 Qmmp Development Team").arg(2006).arg(2026) + u"<br>"_s);
    text.append(u"<a href=\"https://qmmp.ylsoftware.com/\">https://qmmp.ylsoftware.com/</a><br>"_s);
    text.append(u"<a href=\"https://sourceforge.net/projects/qmmp-dev/\">https://sourceforge.net/projects/qmmp-dev/</a>"_s);
    text.append(u"</p>"_s);

    text.append(u"<h5>"_s + tr("Transports:") + u"</h5>"_s);
    text.append(u"<ul type=\"square\">"_s);
    for(const InputSourceFactory *fact : InputSource::factories())
        text.append(u"<li>"_s + fact->properties().name + u"</li>"_s);
    text.append(u"</ul>"_s);

    text.append(u"<h5>"_s + tr("Decoders:") + u"</h5>"_s);
    text.append(u"<ul type=\"square\">"_s);
    for(const DecoderFactory *fact : Decoder::factories())
        text.append(u"<li>"_s + fact->properties().name + u"</li>"_s);
    text.append(u"</ul>"_s);

    if(!AbstractEngine::factories().isEmpty())
    {
        text.append(u"<h5>"_s + tr("Engines:") + u"</h5>"_s);
        text.append(u"<ul type=\"square\">"_s);
        for(const EngineFactory *fact : AbstractEngine::factories())
            text.append(u"<li>"_s + fact->properties().name + u"</li>"_s);
        text.append(u"</ul>"_s);
    }

    text.append(u"<h5>"_s + tr("Effects:") + u"</h5>"_s);
    text.append(u"<ul type=\"square\">"_s);
    for(const EffectFactory *fact : Effect::factories())
        text.append(u"<li>"_s + fact->properties().name + u"</li>"_s);
    text.append(u"</ul>"_s);

    if(!Visual::factories().isEmpty())
    {
        text.append(u"<h5>"_s + tr("Visual plugins:") + u"</h5>"_s);
        text.append(u"<ul type=\"square\">"_s);
        for(const VisualFactory *fact : Visual::factories())
            text.append(u"<li>"_s + fact->properties().name + u"</li>"_s);
        text.append(u"</ul>"_s);
    }

    text.append(u"<h5>"_s + tr("General plugins:") + u"</h5>"_s);
    text.append(u"<ul type=\"square\">"_s);
    for(const GeneralFactory *fact : General::factories())
        text.append(u"<li>"_s + fact->properties().name + u"</li>"_s);
    text.append(u"</ul>"_s);

    text.append(u"<h5>"_s + tr("Output plugins:") + u"</h5>"_s);
    text.append(u"<ul type=\"square\">"_s);
    for(const OutputFactory *fact : Output::factories())
        text.append(u"<li>"_s + fact->properties().name + u"</li>"_s);
    text.append(u"</ul>"_s);

    if(!FileDialog::factories().isEmpty())
    {
        text.append(u"<h5>"_s + tr("File dialogs:") + u"</h5>"_s);
        text.append(u"<ul type=\"square\">"_s);
        for(const FileDialogFactory *fact : FileDialog::factories())
            text.append(u"<li>"_s + fact->properties().name + u"</li>"_s);
        text.append(u"</ul>"_s);
    }

    if(!UiLoader::factories().isEmpty())
    {
        text.append(u"<h5>"_s + tr("User interfaces:") + u"</h5>"_s);
        text.append(u"<ul type=\"square\">"_s);
        for(const UiFactory *fact :UiLoader::factories())
            text.append(u"<li>"_s + fact->properties().name + u"</li>"_s);
        text.append(u"</ul>"_s);
    }
    text.append(u"<div>"_s);

    return text;
}

QString AboutDialog::loadAuthors()
{
    //x-AMP: the fork's own credits are built here rather than written into
    //authors.txt, because that resource has a translated copy per language --
    //authors_es.txt and two dozen more -- and getStringFromResource() picks
    //the one matching the user's locale. Editing the English file alone left
    //x-AMP unattributed on every system not running in English. Going through
    //tr() puts these lines in the same .ts files as the rest of the interface,
    //so one edit covers every language, and upstream's files stay untouched:
    //they are the credits for the earlier work, which is all they claim to be.
    QString text = u"x-AMP\n=====\n\n"_s;
    text += tr("Lead Developer:") + u"\n\n"_s;
    text += u"    Ricardo Aviles Sanders <ravilesx@gmail.com> - "_s;
    text += tr("x-AMP fork, xui interface, artwork and maintenance") + u"\n"_s;
    text += u"        https://Ravilesx.org\n"_s;
    text += u"        https://github.com/RavilesX\n\n\n"_s;

    text += u"Qmmp\n====\n\n"_s;
    text += tr("x-AMP is built on Qmmp, by Ilya Kotov and the Qmmp Development "
               "Team. The engine, the plugin architecture and most of the "
               "decoders are their work, and x-AMP would not exist without it. "
               "The people listed below are credited for that earlier work. They "
               "are not involved in x-AMP and should not be contacted about it.")
            + u"\n\n"_s;
    text += getStringFromResource(u":authors"_s);
    return text;
}

QString AboutDialog::getStringFromResource(const QString &res_file)
{
    QString ret_string;
    QStringList paths = { QStringLiteral("%1_%2.txt").arg(res_file, Qmmp::systemLanguageID()) };
    if(Qmmp::systemLanguageID().contains(QLatin1Char('.')))
        paths << QStringLiteral("%1_%2.txt").arg(res_file, Qmmp::systemLanguageID().split(QLatin1Char('.')).at(0));
    if(Qmmp::systemLanguageID().contains(QLatin1Char('_')))
        paths << QStringLiteral("%1_%2.txt").arg(res_file, Qmmp::systemLanguageID().split(QLatin1Char('_')).at(0));
    paths << res_file + u".txt"_s;
    paths << res_file;

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
