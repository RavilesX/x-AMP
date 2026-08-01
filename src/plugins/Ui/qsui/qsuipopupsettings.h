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
#ifndef QSUIPOPSETTINGS_H
#define QSUIPOPSETTINGS_H

#include <QDialog>
#include <QObject>

namespace Ui {
class QSUiPopupSettings;
}

/**
    @author Ilya Kotov <forkotov02@ya.ru>
*/
class QSUiPopupSettings : public QDialog
{
Q_OBJECT
public:
    QSUiPopupSettings(QWidget *parent = nullptr);
    ~QSUiPopupSettings();


public slots:
    virtual void accept() override;

private slots:
    void on_resetButton_clicked();

private:
    void createMenu();
    Ui::QSUiPopupSettings *m_ui;
};

#endif
