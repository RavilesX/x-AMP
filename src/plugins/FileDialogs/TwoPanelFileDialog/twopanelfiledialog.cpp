/**************************************************************************
*   Copyright (C) 2016-2026 by Ilya Kotov                                 *
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
#include <qmmp/qmmp.h>
#include "twopanelfiledialogimpl.h"
#include "twopanelfiledialog.h"

TwoPanelFileDialog::TwoPanelFileDialog()
{
    m_dialog = new TwoPanelFileDialogImpl();
    connect(m_dialog, &TwoPanelFileDialogImpl::filesSelected, this, &TwoPanelFileDialog::filesSelected);
}

TwoPanelFileDialog::~TwoPanelFileDialog()
{
    qCWarning(plugin) << Q_FUNC_INFO;
    delete m_dialog;
}
void TwoPanelFileDialog::raise(const QString &dir, Mode mode, const QString &caption,
                           const QStringList &mask)
{
    m_dialog->setModeAndMask(dir, mode, mask);
    m_dialog->setWindowTitle(caption);
    m_dialog->show();
    m_dialog->raise();
}

QStringList TwoPanelFileDialog::exec(QWidget *parent, const QString &dir, FileDialog::Mode mode, const QString &caption, const QString &filter, QString *)
{
    TwoPanelFileDialogImpl *dialog = new TwoPanelFileDialogImpl(parent);
    dialog->setWindowTitle(caption);
    dialog->setModeAndMask(dir, mode, filter.split(u";;"_s, Qt::SkipEmptyParts));
    QStringList l;
    if(dialog->exec() == QDialog::Accepted)
        l = dialog->selectedFiles();
    dialog->deleteLater();
    return l;
}

FileDialog* TwoPanelFileDialogFactory::create()
{
    return new TwoPanelFileDialog();
}

FileDialogProperties TwoPanelFileDialogFactory::properties() const
{
    FileDialogProperties properties;
    properties.name = tr("Two-panel File Dialog");
    properties.shortName = "twopanel_dialog"_L1;
    properties.hasAbout = true;
    properties.modal = false;
    return properties;
}

void TwoPanelFileDialogFactory::showAbout(QWidget *parent)
{
    QMessageBox::about(parent, tr("About Two-panel File Dialog"),
                       tr("Two-panel File Dialog") + QChar::LineFeed +
                       tr("Written by: Ilya Kotov <forkotov02@ya.ru>") + QChar::LineFeed +
                       tr("Based on code from the Qt library"));
}

QString TwoPanelFileDialogFactory::translation() const
{
    return QLatin1String(":/two_panel_file_dialog_plugin_");
}
