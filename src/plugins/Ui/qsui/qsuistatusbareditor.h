/***************************************************************************
 *   Copyright (C) 2013-2026 by Ilya Kotov                                 *
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

#ifndef QSUISTATUSBAREDITOR_H
#define QSUISTATUSBAREDITOR_H

#include <QDialog>
#include <QVariantList>
#include <QIcon>

namespace Ui {
class QSUiStatusBarEditor;
}

class QListWidgetItem;

/**
    @author Ilya Kotov <forkotov02@ya.ru>
*/
class QSUiStatusBarEditor : public QDialog
{
    Q_OBJECT

public:
    explicit QSUiStatusBarEditor(QWidget *parent);
    ~QSUiStatusBarEditor();

public slots:
    void accept() override;

private slots:
    void on_addToolButton_clicked();
    void on_removeToolButton_clicked();
    void on_upToolButton_clicked();
    void on_downToolButton_clicked();

private:
    void populateLabelList(bool reset);
    Ui::QSUiStatusBarEditor *m_ui;
};

#endif // QSUiStatusBarEditor_H
