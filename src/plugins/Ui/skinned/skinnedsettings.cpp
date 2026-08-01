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

#include <QSettings>
#include <QDir>
#include <QFontDialog>
#include <QStandardPaths>
#include <qmmp/qmmp.h>
#include <qmmpui/filedialog.h>
#include <qmmpui/uihelper.h>
#include <qmmpui/metadataformattermenu.h>
#include "skinreader.h"
#include "skin.h"
#include "skinnedpopupsettings.h"
#include "ui_skinnedsettings.h"
#include "skinnedsettings.h"

SkinnedSettings::SkinnedSettings(QWidget *parent) : QWidget(parent), m_ui(new Ui::SkinnedSettings)
{
    m_ui->setupUi(this);
    m_ui->listWidget->setIconSize(QSize(105, 34));
    m_skin = Skin::instance();
    m_reader = new SkinReader(this);
    connect(m_ui->skinReloadButton, &QPushButton::clicked, this, &SkinnedSettings::loadSkins);
    connect(m_ui->plTransparencySlider, &QSlider::valueChanged, m_ui->plTransparencyLabel, qOverload<int>(&QLabel::setNum));
    connect(m_ui->mwTransparencySlider, &QSlider::valueChanged, m_ui->mwTransparencyLabel, qOverload<int>(&QLabel::setNum));
    connect(m_ui->eqTransparencySlider, &QSlider::valueChanged, m_ui->eqTransparencyLabel, qOverload<int>(&QLabel::setNum));
    connect(m_ui->mainFontButton, &QToolButton::clicked, this, [this] { selectFont(m_ui->mainFontLabel); });
    connect(m_ui->plFontButton, &QToolButton::clicked, this, [this] { selectFont(m_ui->plFontLabel); });
    connect(m_ui->groupFontButton, &QToolButton::clicked, this, [this] { selectFont(m_ui->groupFontLabel); });
    connect(m_ui->extraRowFontButton, &QToolButton::clicked, this, [this] { selectFont(m_ui->extraRowFontLabel); });
    connect(m_ui->headerFontButton, &QToolButton::clicked, this, [this] { selectFont(m_ui->headerFontLabel); });

    m_ui->skinPathComboBox->addItem(Qmmp::configDir() + QStringLiteral("/skins"));
    m_ui->skinPathComboBox->addItem(Qmmp::userDataPath() + QStringLiteral("/skins"));

#if defined(Q_OS_WIN) && !defined(Q_OS_CYGWIN)
    m_ui->skinPathComboBox->setVisible(false);
    m_ui->skinPathLabel->setVisible(false);
#endif

    readSettings();
    loadSkins();
    loadFonts();
    createActions();
}

SkinnedSettings::~SkinnedSettings()
{
    delete m_ui;
}

void SkinnedSettings::on_listWidget_itemClicked(QListWidgetItem *item)
{
    m_currentSkinPath = item->data(Qt::UserRole).toString();
    m_skin->setSkin(m_currentSkinPath, true);
}

void SkinnedSettings::on_resetFontsButton_clicked()
{
    QFont extraRowDefaultFont = qApp->font();
    extraRowDefaultFont.setPointSize(extraRowDefaultFont.pointSize() - 1);
    extraRowDefaultFont.setStyle(QFont::StyleItalic);

    setFont(m_ui->mainFontLabel, qApp->font().toString());
    setFont(m_ui->plFontLabel, qApp->font().toString());
    setFont(m_ui->groupFontLabel, qApp->font().toString());
    setFont(m_ui->extraRowFontLabel, extraRowDefaultFont.toString());
    setFont(m_ui->headerFontLabel, qApp->font().toString());
}

void SkinnedSettings::on_skinInstallButton_clicked()
{
    const QStringList files = FileDialog::getOpenFileNames(this,tr("Select Skin Files"), QDir::homePath(),
                                                           tr("Skin files") + u" (*.tar.gz *.tgz *.tar.bz2 *.zip *.wsz)"_s);
    for(const QString &path : std::as_const(files))
    {
        QFile file(path);
        file.copy(m_ui->skinPathComboBox->currentText() + QLatin1Char('/') + QFileInfo(path).fileName());
    }
    loadSkins();
}

void SkinnedSettings::showEvent(QShowEvent *)
{
    m_ui->hiddenCheckBox->setEnabled(UiHelper::instance()->visibilityControl());
    m_ui->hideOnCloseCheckBox->setEnabled(UiHelper::instance()->visibilityControl());
}

void SkinnedSettings::selectFont(QLabel *label)
{
    bool ok = false;
    QFont font = QFontDialog::getFont(&ok, label->font(), this);
    if(ok)
    {
        label->setText(font.family() + QChar::Space + QString::number(font.pointSize()));
        label->setFont(font);
    }
}

void SkinnedSettings::setFont(QLabel *label, const QString &fontName)
{
    QFont font;
    font.fromString(fontName);
    label->setText(font.family() + QChar::Space + QString::number(font.pointSize()));
    label->setFont(font);
}

void SkinnedSettings::loadFonts()
{
    QFont extraRowDefaultFont = qApp->font();
    extraRowDefaultFont.setPointSize(extraRowDefaultFont.pointSize() - 1);
    extraRowDefaultFont.setStyle(QFont::StyleItalic);

    QSettings settings;
    settings.beginGroup("Skinned"_L1);
    setFont(m_ui->mainFontLabel, settings.value("mw_font"_L1, qApp->font().toString()).toString());
    setFont(m_ui->plFontLabel, settings.value("pl_font"_L1, qApp->font().toString()).toString());
    setFont(m_ui->groupFontLabel, settings.value(u"pl_group_font"_s, qApp->font().toString()).toString());
    setFont(m_ui->extraRowFontLabel, settings.value(u"pl_extra_row_font"_s, extraRowDefaultFont.toString()).toString());
    setFont(m_ui->headerFontLabel, settings.value(u"pl_header_font"_s, qApp->font().toString()).toString());
    m_ui->useBitmapCheckBox->setChecked(settings.value("bitmap_font"_L1, false).toBool());
    settings.endGroup();
}

void SkinnedSettings::createActions()
{
    MetaDataFormatterMenu *menu = new MetaDataFormatterMenu(MetaDataFormatterMenu::TITLE_MENU, this);
    m_ui->windowTitleButton->setMenu(menu);
    m_ui->windowTitleButton->setPopupMode(QToolButton::InstantPopup);
    connect(menu, &MetaDataFormatterMenu::patternSelected, this, &SkinnedSettings::addWindowTitleString);
}

void SkinnedSettings::loadSkins()
{
    m_reader->loadSkins();
    m_ui->listWidget->clear();

    //default skin
    QFileInfo fileInfo(SkinReader::defaultSkinPath());
    QListWidgetItem *item = new QListWidgetItem(fileInfo.fileName());
    item->setIcon(SkinReader::getPixmapFromDirectory(u"main"_s, fileInfo.filePath()));
    item->setData(Qt::UserRole, fileInfo.filePath());
    item->setToolTip(tr("Default skin"));
    m_ui->listWidget->addItem(item);

    for(const QString &path : std::as_const(m_reader->skins()))
    {
        fileInfo.setFile(path);
        item = new QListWidgetItem(fileInfo.fileName());
        item->setIcon(m_reader->getPreview(path));
        item->setData(Qt::UserRole, path);
        item->setToolTip(fileInfo.isDir() ? tr("Unarchived skin %1").arg(path) : tr("Archived skin %1").arg(path));
        m_ui->listWidget->addItem(item);
    }

    qCDebug(plugin) << m_currentSkinPath;

    for(int i = 0; i < m_ui->listWidget->count(); ++i)
    {
        if(m_ui->listWidget->item(i)->data(Qt::UserRole).toString() == m_currentSkinPath)
        {
            m_ui->listWidget->setCurrentRow(i, QItemSelectionModel::Select);
            break;
        }
    }
}

void SkinnedSettings::on_popupTemplateButton_clicked()
{
    SkinnedPopupSettings *p = new SkinnedPopupSettings(this);
    p->exec();
    p->deleteLater();
}

void SkinnedSettings::addWindowTitleString(const QString &str)
{
    if(m_ui->windowTitleLineEdit->cursorPosition () < 1)
        m_ui->windowTitleLineEdit->insert(str);
    else
        m_ui->windowTitleLineEdit->insert(u" - "_s + str);
}

void SkinnedSettings::readSettings()
{
    QSettings settings;
    settings.beginGroup("Skinned"_L1);
    //playlist
    m_ui->smoothScrollingCheckBox->setChecked(settings.value(u"pl_smooth_scrolling"_s, true).toBool());
    m_ui->protocolCheckBox->setChecked(settings.value ("pl_show_protocol"_L1, false).toBool());
    m_ui->numbersCheckBox->setChecked(settings.value("pl_show_numbers"_L1, true).toBool());
    m_ui->lengthsCheckBox->setChecked(settings.value("pl_show_lengths"_L1, true).toBool());
    m_ui->alignCheckBox->setChecked(settings.value("pl_align_numbers"_L1, false).toBool());
    m_ui->anchorCheckBox->setChecked(settings.value("pl_show_anchor"_L1, false).toBool());
    m_ui->showSplittersCheckBox->setChecked(settings.value("pl_show_splitters"_L1, true).toBool());
    m_ui->alternateSplitterColorCheckBox->setChecked(settings.value("pl_alt_splitter_color"_L1, false).toBool());
    m_ui->popupCheckBox->setChecked(settings.value("pl_show_popup"_L1, false).toBool());
    m_ui->plSeplineEdit->setText(settings.value("pl_separator"_L1, u"::"_s).toString());
    m_ui->showNewPLCheckBox->setChecked(settings.value("pl_show_create_button"_L1, false).toBool());
    //transparency
    m_ui->mwTransparencySlider->setValue(100 - settings.value("mw_opacity"_L1, 1.0).toDouble()*100);
    m_ui->eqTransparencySlider->setValue(100 - settings.value("eq_opacity"_L1, 1.0).toDouble()*100);
    m_ui->plTransparencySlider->setValue(100 - settings.value("pl_opacity"_L1, 1.0).toDouble()*100);
    //skins
    m_currentSkinPath = settings.value("skin_path"_L1, SkinReader::defaultSkinPath()).toString();
    if(!QFile::exists(m_currentSkinPath))
        m_currentSkinPath = SkinReader::defaultSkinPath();
    m_ui->randomSkinCheckBox->setChecked(settings.value("random_skin"_L1).toBool());
    m_ui->skinPathComboBox->setCurrentIndex(settings.value("config_path_for_skins"_L1, true).toBool() ? UserConfigPath : UserDataPath);
    //view
    m_ui->skinCursorsCheckBox->setChecked(settings.value("skin_cursors"_L1, false).toBool());
    m_ui->hiddenCheckBox->setChecked(settings.value("start_hidden"_L1, false).toBool());
    m_ui->hideOnCloseCheckBox->setChecked(settings.value("hide_on_close"_L1, false).toBool());
    m_ui->windowTitleLineEdit->setText(settings.value("window_title_format"_L1, u"%if(%p,%p - %t,%t)"_s).toString());
    //playlist colors
    m_ui->plSkinColorsCheckBox->setChecked(settings.value("pl_use_skin_colors"_L1, true).toBool());
    m_ui->plBg1Color->setColor(settings.value("pl_bg1_color"_L1, u"#0d0d0d"_s).toString());
    m_ui->plBg2Color->setColor(settings.value("pl_bg2_color"_L1, u"#0d0d0d"_s).toString());
    m_ui->plHlColor->setColor(settings.value("pl_highlight_color"_L1, u"#2a2a2a"_s).toString());
    m_ui->plTextNormalColor->setColor(settings.value("pl_normal_text_color"_L1, u"#5a5a5a"_s).toString());
    m_ui->plTextCurrentColor->setColor(settings.value("pl_current_text_color"_L1, u"#407dec"_s).toString());
    m_ui->plTextHlColor->setColor(settings.value("pl_hl_text_color"_L1, u"#5a5a5a"_s).toString());
    m_ui->plGrBgColor->setColor(settings.value("pl_group_bg"_L1, u"#0d0d0d"_s).toString());
    m_ui->plSplitterColor->setColor(settings.value("pl_splitter_color"_L1, u"#5a5a5a"_s).toString());
    m_ui->plGrTextColor->setColor(settings.value("pl_group_text"_L1, u"#5a5a5a"_s).toString());
    m_ui->plCurrentTrackBgColor->setColor(settings.value("pl_current_bg_color"_L1, u"#0d0d0d"_s).toString());
    m_ui->plOverrideGroupBgCheckBox->setChecked(settings.value("pl_override_group_bg"_L1, false).toBool());
    m_ui->plOverrideCurrentBgCheckBox->setChecked(settings.value("pl_override_current_bg"_L1, false).toBool());
    settings.endGroup();
}

void SkinnedSettings::writeSettings()
{
    QSettings settings;
    settings.beginGroup("Skinned"_L1);
    settings.setValue(u"pl_smooth_scrolling"_s, m_ui->smoothScrollingCheckBox->isChecked());
    settings.setValue("pl_show_protocol"_L1, m_ui->protocolCheckBox->isChecked());
    settings.setValue("pl_show_numbers"_L1, m_ui->numbersCheckBox->isChecked());
    settings.setValue("pl_show_lengths"_L1, m_ui->lengthsCheckBox->isChecked());
    settings.setValue("pl_align_numbers"_L1, m_ui->alignCheckBox->isChecked());
    settings.setValue("pl_show_anchor"_L1, m_ui->anchorCheckBox->isChecked());
    settings.setValue("pl_show_splitters"_L1, m_ui->showSplittersCheckBox->isChecked());
    settings.setValue("pl_alt_splitter_color"_L1, m_ui->alternateSplitterColorCheckBox->isChecked());
    settings.setValue("pl_show_popup"_L1, m_ui->popupCheckBox->isChecked());
    settings.setValue("pl_separator"_L1, m_ui->plSeplineEdit->text());
    settings.setValue("pl_show_create_button"_L1, m_ui->showNewPLCheckBox->isChecked());
    settings.setValue("mw_opacity"_L1, 1.0 - (double)m_ui->mwTransparencySlider->value()/100);
    settings.setValue("eq_opacity"_L1, 1.0 - (double)m_ui->eqTransparencySlider->value()/100);
    settings.setValue("pl_opacity"_L1, 1.0 - (double)m_ui->plTransparencySlider->value()/100);
    settings.setValue("bitmap_font"_L1, m_ui->useBitmapCheckBox->isChecked());
    settings.setValue("skin_cursors"_L1, m_ui->skinCursorsCheckBox->isChecked());
    settings.setValue("start_hidden"_L1, m_ui->hiddenCheckBox->isChecked());
    settings.setValue("hide_on_close"_L1, m_ui->hideOnCloseCheckBox->isChecked());
    settings.setValue("window_title_format"_L1, m_ui->windowTitleLineEdit->text());
    settings.setValue("mw_font"_L1, m_ui->mainFontLabel->font().toString());
    settings.setValue("pl_font"_L1, m_ui->plFontLabel->font().toString());
    settings.setValue("pl_group_font"_L1, m_ui->groupFontLabel->font().toString());
    settings.setValue("pl_extra_row_font"_L1, m_ui->extraRowFontLabel->font().toString());
    settings.setValue("pl_header_font"_L1,  m_ui->headerFontLabel->font().toString());
    //playlist colors
    settings.setValue("pl_use_skin_colors"_L1, m_ui->plSkinColorsCheckBox->isChecked());
    settings.setValue("pl_bg1_color"_L1, m_ui->plBg1Color->colorName());
    settings.setValue("pl_bg2_color"_L1, m_ui->plBg2Color->colorName());
    settings.setValue("pl_highlight_color"_L1, m_ui->plHlColor->colorName());
    settings.setValue("pl_normal_text_color"_L1, m_ui->plTextNormalColor->colorName());
    settings.setValue("pl_current_text_color"_L1, m_ui->plTextCurrentColor->colorName());
    settings.setValue("pl_hl_text_color"_L1, m_ui->plTextHlColor->colorName());
    settings.setValue("pl_group_bg"_L1, m_ui->plGrBgColor->colorName());
    settings.setValue("pl_splitter_color"_L1, m_ui->plSplitterColor->colorName());
    settings.setValue("pl_group_text"_L1, m_ui->plGrTextColor->colorName());
    settings.setValue("pl_current_bg_color"_L1, m_ui->plCurrentTrackBgColor->colorName());
    settings.setValue("pl_override_group_bg"_L1, m_ui->plOverrideGroupBgCheckBox->isChecked());
    settings.setValue("pl_override_current_bg"_L1, m_ui->plOverrideCurrentBgCheckBox->isChecked());
    //skins
    settings.setValue("skin_path"_L1, m_currentSkinPath);
    settings.setValue("random_skin"_L1, m_ui->randomSkinCheckBox->isChecked());
    settings.setValue("config_path_for_skins"_L1, m_ui->skinPathComboBox->currentIndex() == UserConfigPath);
    settings.endGroup();
}

void SkinnedSettings::on_loadSkinColorsButton_clicked()
{
    m_ui->plBg1Color->setColor(m_skin->getPLValue("normalbg"));
    m_ui->plBg2Color->setColor(m_skin->getPLValue("normalbg"));
    m_ui->plHlColor->setColor(m_skin->getPLValue("selectedbg"));
    m_ui->plTextNormalColor->setColor(m_skin->getPLValue("normal"));
    m_ui->plTextCurrentColor->setColor(m_skin->getPLValue("current"));
    m_ui->plTextHlColor->setColor(m_skin->getPLValue("normal"));
    m_ui->plGrBgColor->setColor(m_skin->getPLValue("normalbg"));
    m_ui->plSplitterColor->setColor(m_skin->getPLValue("normal"));
    m_ui->plGrTextColor->setColor(m_skin->getPLValue("normal"));
    m_ui->plCurrentTrackBgColor->setColor(m_skin->getPLValue("normalbg"));
    m_ui->plOverrideGroupBgCheckBox->setChecked(false);
    m_ui->plOverrideCurrentBgCheckBox->setChecked(false);
}
