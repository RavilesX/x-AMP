/***************************************************************************
 *   Copyright (C) 2008-2026 by Ilya Kotov                                 *
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
#ifndef NOTIFIERSETTINGSDIALOG_H
#define NOTIFIERSETTINGSDIALOG_H

#include <QDialog>
#include <QHash>

class Action;

namespace Ui {
class NotifierSettingsDialog;
}

/**
    @author Ilya Kotov <forkotov02@ya.ru>
*/
class NotifierSettingsDialog : public QDialog
{
Q_OBJECT
public:
    explicit NotifierSettingsDialog(QWidget *parent = nullptr);

    virtual ~NotifierSettingsDialog();


public slots:
    virtual void accept() override;

private slots:
    void on_fontButton_pressed();
    void on_templateButton_pressed();

private:
    Ui::NotifierSettingsDialog *m_ui;
    QString m_template;
    QHash<uint, QPushButton*> m_buttons;

};

#endif
