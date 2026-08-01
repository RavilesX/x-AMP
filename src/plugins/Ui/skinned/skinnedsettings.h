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

#ifndef SKINNEDSETTINGS_H
#define SKINNEDSETTINGS_H

#include <QWidget>
#include <QFileInfo>

class QListWidgetItem;
class QLabel;
class SkinReader;
class Skin;

namespace Ui {
class SkinnedSettings;
}

class SkinnedSettings : public QWidget
{
    Q_OBJECT
public:
    explicit SkinnedSettings(QWidget *parent = nullptr);
    virtual ~SkinnedSettings();
    void writeSettings();

private slots:
    void on_listWidget_itemClicked(QListWidgetItem *item);
    void on_resetFontsButton_clicked();
    void on_skinInstallButton_clicked();
    void loadSkins();
    void on_popupTemplateButton_clicked();
    void addWindowTitleString(const QString &str);
    void on_loadSkinColorsButton_clicked();

private:
    void showEvent(QShowEvent *) override;
    void selectFont(QLabel *label);
    void setFont(QLabel *label, const QString &fontName);
    void loadFonts();
    void createActions();
    void readSettings();

    enum SkinPath
    {
        UserConfigPath = 0,
        UserDataPath
    };

    Ui::SkinnedSettings *m_ui;
    QString m_currentSkinPath;
    Skin *m_skin;
    SkinReader *m_reader;

};

#endif // SKINNEDSETTINGS_H
