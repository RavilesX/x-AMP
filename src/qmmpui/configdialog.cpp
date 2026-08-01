/***************************************************************************
 *   Copyright (C) 2007-2026 by Ilya Kotov                                 *
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
#include <QDir>
#include <QSettings>
#include <QFontDialog>
#include <QTreeWidgetItem>
#include <QIntValidator>
#include <qmmp/decoder.h>
#include <qmmp/output.h>
#include <qmmp/decoderfactory.h>
#include <qmmp/outputfactory.h>
#include <qmmp/visualfactory.h>
#include <qmmp/effectfactory.h>
#include <qmmp/effect.h>
#include <qmmp/visual.h>
#include <qmmp/soundcore.h>
#include <qmmp/enginefactory.h>
#include <qmmp/abstractengine.h>
#include <qmmp/qmmpsettings.h>
#include <qmmp/inputsource.h>
#include <qmmp/inputsourcefactory.h>
#include "ui_configdialog.h"
#include "pluginitem_p.h"
#include "radioitemdelegate_p.h"
#include "generalfactory.h"
#include "general.h"
#include "uihelper.h"
#include "uiloader.h"
#include "filedialog.h"
#include "mediaplayer.h"
#include "qmmpuisettings.h"
#include "metadataformattermenu.h"
#include "configdialog.h"

class ConfigDialogPrivate : public Ui::ConfigDialog
{
    Q_DECLARE_PUBLIC(::ConfigDialog)
    Q_DECLARE_TR_FUNCTIONS(ConfigDialog)
public:
    ConfigDialogPrivate(::ConfigDialog *dialog) : q_ptr(dialog)
    {}

private:
    void onContentsWidgetCurrentItemChanged(QListWidgetItem *current, QListWidgetItem *previous)
    {
        if(!current)
            current = previous;
        stackedWidget->setCurrentIndex(contentsWidget->row (current));
    }

    void onPreferencesButtonClicked()
    {
        QTreeWidgetItem *item = treeWidget->currentItem();
        if(item && item->type() >= PluginItem::TRANSPORT)
            dynamic_cast<PluginItem *>(item)->showSettings(q_ptr);
    }

    void onInformationButtonClicked()
    {
        QTreeWidgetItem *item = treeWidget->currentItem();
        if(item && item->type() >= PluginItem::TRANSPORT)
            dynamic_cast<PluginItem *>(item)->showAbout(q_ptr);
    }

    void onPriorityActionTriggeted()
    {
        QTreeWidgetItem *item = treeWidget->currentItem();
        if(item && item->type() >= PluginItem::TRANSPORT)
            dynamic_cast<PluginItem *>(item)->showPriority(q_ptr);
    }

    void onTreeWidgetItemChanged(QTreeWidgetItem *item, int column)
    {
        if(column == 0 && item->type() >= PluginItem::TRANSPORT)
            dynamic_cast<PluginItem *>(item)->setEnabled(item->checkState(0) == Qt::Checked);
    }

    void onTreeWidgetCurrentItemChanged(QTreeWidgetItem *current)
    {
        if(current && current->type() >= PluginItem::TRANSPORT)
        {
            preferencesButton->setEnabled(dynamic_cast<PluginItem *>(current)->hasSettings());
            informationButton->setEnabled(dynamic_cast<PluginItem *>(current)->hasAbout());
            priorityAction->setVisible(current->type() == PluginItem::DECODER || current->type() == PluginItem::ENGINE);
        }
        else
        {
            preferencesButton->setEnabled(false);
            informationButton->setEnabled(false);
            priorityAction->setVisible(false);
        }
        preferencesAction->setEnabled(preferencesButton->isEnabled());
        informationAction->setEnabled(informationButton->isEnabled());
    }

    void on_cueFontButton_clicked()
    {
        bool ok = false;
        QFont font = cueFontLabel->font();
        font = QFontDialog::getFont(&ok, font, q_ptr);
        if(ok)
        {
            cueFontLabel->setText(font.family () + QChar::Space + QString::number(font.pointSize ()));
            cueFontLabel->setFont(font);
        }
    }

    void saveSettings()
    {
        if(QmmpUiSettings *guis = QmmpUiSettings::instance())
        {
            guis->setGroupFormat(groupLineEdit->text().trimmed());
            guis->setGroupExtraRowFormat(extraRowFormatLineEdit->text().trimmed());
            guis->setLinesPerGroup(linesPerGroupComboBox->currentData().toInt());
            guis->setGroupCoverVisible(groupsShowCoverCheckBox->isChecked());
            guis->setGroupDividingLineVisible(showDividingLineCheckBox->isChecked());
            guis->setGroupExtraRowVisible(showExtraRowCheckBox->isChecked());
            guis->setUseMetaData(metaDataCheckBox->isChecked());
            guis->setReadMetaDataForPlayLists(plMetaDataCheckBox->isChecked());
            guis->setConvertUnderscore(underscoresCheckBox->isChecked());
            guis->setConvertTwenty(per20CheckBox->isChecked());
            guis->setClearPreviousPlayList(clearPrevPLCheckBox->isChecked());
            guis->setSkipExistingTracks(skipExistingTracksCheckBox->isChecked());
            guis->setStopAfterRemovingOfCurrentTrack(stopAfterRemovingCheckBox->isChecked());
            guis->setResumeOnStartup(continuePlaybackCheckBox->isChecked());
            guis->setRestrictFilters(dirRestrictLineEdit->text());
            guis->setExcludeFilters(dirExcludeLineEdit->text());
            guis->setDefaultPlayList(defaultPlayListLineEdit->text(),
                                     defaultPlayListCheckBox->isChecked());
            guis->setAutoSavePlayList(autoSavePlayListCheckBox->isChecked());
            guis->setUseClipboard(clipboardCheckBox->isChecked());
        }

        QmmpSettings *gs = QmmpSettings::instance();
        //proxy
        QUrl proxyUrl;
        proxyUrl.setHost(hostLineEdit->text());
        proxyUrl.setPort(portLineEdit->text().toInt());
        proxyUrl.setUserName(proxyUserLineEdit->text());
        proxyUrl.setPassword(proxyPasswLineEdit->text());
        gs->setNetworkSettings(enableProxyCheckBox->isChecked(),
                               authProxyCheckBox->isChecked(),
                               static_cast<QmmpSettings::ProxyType>(proxyTypeComboBox->currentData().toInt()),
                               proxyUrl);

        gs->setCoverSettings(coverIncludeLineEdit->text().split(QLatin1Char(',')),
                             coverExcludeLineEdit->text().split(QLatin1Char(',')),
                             coverDepthSpinBox->value(),
                             useCoverFilesCheckBox->isChecked());
        int i = replayGainModeComboBox->currentIndex();
        gs->setReplayGainSettings((QmmpSettings::ReplayGainMode)
                                  replayGainModeComboBox->itemData(i).toInt(),
                                  preampDoubleSpinBox->value(),
                                  defaultGainDoubleSpinBox->value(),
                                  clippingCheckBox->isChecked());
        i = bitDepthComboBox->currentIndex();
        gs->setAudioSettings(softVolumeCheckBox->isChecked(),
                             (Qmmp::AudioFormat)bitDepthComboBox->itemData(i).toInt(),
                             ditheringCheckBox->isChecked());
        gs->setAverageBitrate(abrCheckBox->isChecked());
        gs->setBufferSize(bufferSizeSpinBox->value());
        gs->setDetermineFileTypeByContent(byContentCheckBox->isChecked());
        gs->setVolumeStep(volumeStepSpinBox->value());
        EqSettings eqs = gs->eqSettings();
        eqs.setTwoPasses(twoPassEqCheckBox->isChecked());
        gs->setEqSettings(eqs);
        QList<QVariant> var_sizes = { splitter->sizes().constFirst(), splitter->sizes().constLast() };
        QSettings settings;
        settings.setValue(u"ConfigDialog/splitter_sizes"_s, var_sizes);
        settings.setValue(u"ConfigDialog/window_size"_s, q_ptr->size());
        //User interface language
        int index = langComboBox->currentIndex();
        if(index >= 0)
            Qmmp::setUiLanguageID(langComboBox->itemData(index).toString());
        //fonts
        settings.setValue(u"CueEditor/font"_s, cueFontLabel->font().toString());
        settings.setValue(u"CueEditor/use_system_font"_s, cueSystemFontCheckBox->isChecked());
    }

    void revertSettings()
    {
        for(int i = 0; i < treeWidget->topLevelItemCount(); ++i)
        {
            QTreeWidgetItem *item = treeWidget->topLevelItem(i);

            for(int j = 0; j < item->childCount(); ++j)
            {
                QTreeWidgetItem *pluginItem = item->child(j);
                static_cast<PluginItem *>(pluginItem)->revertState();
            }
        }
    }

    void updateGroupSettings()
    {
        int linesPerGroup = linesPerGroupComboBox->currentData().toInt();
        groupsShowCoverCheckBox->setEnabled(linesPerGroup > 1);
        showExtraRowCheckBox->setEnabled(linesPerGroup > 1);
        extraRowFormatLineEdit->setEnabled(linesPerGroup > 1 && showExtraRowCheckBox->isChecked());
        extraRowFormatButton->setEnabled(linesPerGroup > 1 && showExtraRowCheckBox->isChecked());
        extraRowFormatLabel->setEnabled(linesPerGroup > 1 && showExtraRowCheckBox->isChecked());
    }

    void readSettings()
    {
        if(MediaPlayer::instance())
        {
            //playlist options
            QmmpUiSettings *guis = QmmpUiSettings::instance();
            groupLineEdit->setText(guis->groupFormat());
            extraRowFormatLineEdit->setText(guis->groupExtraRowFormat());
            linesPerGroupComboBox->setCurrentIndex(linesPerGroupComboBox->findData(guis->linesPerGroup()));
            groupsShowCoverCheckBox->setChecked(guis->groupCoverVisible());
            showDividingLineCheckBox->setChecked(guis->groupDividingLineVisible());
            showExtraRowCheckBox->setChecked(guis->groupExtraRowVisible());
            metaDataCheckBox->setChecked(guis->useMetaData());
            plMetaDataCheckBox->setChecked(guis->readMetaDataForPlayLists());
            underscoresCheckBox->setChecked(guis->convertUnderscore());
            per20CheckBox->setChecked(guis->convertTwenty());
            clearPrevPLCheckBox->setChecked(guis->clearPreviousPlayList());
            skipExistingTracksCheckBox->setChecked(guis->skipExistingTracks());
            stopAfterRemovingCheckBox->setChecked(guis->stopAfterRemovingOfCurrentTrack());
            //resume playback on startup
            continuePlaybackCheckBox->setChecked(guis->resumeOnStartup());
            //directory filters
            dirRestrictLineEdit->setText(guis->restrictFilters().join(QLatin1Char(',')).trimmed());
            dirExcludeLineEdit->setText(guis->excludeFilters().join(QLatin1Char(',')).trimmed());
            //default playlist
            defaultPlayListCheckBox->setChecked(guis->useDefaultPlayList());
            defaultPlayListLineEdit->setText(guis->defaultPlayListName());
            //playlist auto-save when modified
            autoSavePlayListCheckBox->setChecked(guis->autoSavePlayList());
            //url dialog
            clipboardCheckBox->setChecked(guis->useClipboard());
        }
        //proxy settings
        QmmpSettings *gs = QmmpSettings::instance();
        enableProxyCheckBox->setChecked(gs->isProxyEnabled());
        authProxyCheckBox->setChecked(gs->useProxyAuth());
        hostLineEdit->setText(gs->proxy().host());
        proxyTypeComboBox->setCurrentIndex(proxyTypeComboBox->findData(gs->proxyType()));
        if (gs->proxy().port(0))
            portLineEdit->setText(QString::number(gs->proxy().port(0)));
        proxyUserLineEdit->setText(gs->proxy().userName());
        proxyPasswLineEdit->setText(gs->proxy().password());

        hostLineEdit->setEnabled(enableProxyCheckBox->isChecked());
        portLineEdit->setEnabled(enableProxyCheckBox->isChecked());
        proxyTypeComboBox->setEnabled(enableProxyCheckBox->isChecked());
        proxyUserLineEdit->setEnabled(authProxyCheckBox->isChecked());
        proxyPasswLineEdit->setEnabled(authProxyCheckBox->isChecked());
        //file type determination
        byContentCheckBox->setChecked(gs->determineFileTypeByContent());
        //cover options
        coverIncludeLineEdit->setText(gs->coverNameFilters(true).join(QLatin1Char(',')));
        coverExcludeLineEdit->setText(gs->coverNameFilters(false).join(QLatin1Char(',')));
        coverDepthSpinBox->setValue(gs->coverSearchDepth());
        useCoverFilesCheckBox->setChecked(gs->useCoverFiles());
        //replay gain
        clippingCheckBox->setChecked(gs->replayGainPreventClipping());
        replayGainModeComboBox->setCurrentIndex(replayGainModeComboBox->findData(gs->replayGainMode()));
        preampDoubleSpinBox->setValue(gs->replayGainPreamp());
        defaultGainDoubleSpinBox->setValue(gs->replayGainDefaultGain());
        //audio
        volumeStepSpinBox->setValue(gs->volumeStep());
        softVolumeCheckBox->setChecked(gs->useSoftVolume());
        bitDepthComboBox->setCurrentIndex(bitDepthComboBox->findData(gs->outputFormat()));
        ditheringCheckBox->setChecked(gs->useDithering());
        bufferSizeSpinBox->setValue(gs->bufferSize());
        abrCheckBox->setChecked(gs->averageBitrate());
        //equalizer
        twoPassEqCheckBox->setChecked(gs->eqSettings().twoPasses());
        //geometry
        QSettings settings;
        q_ptr->resize(settings.value(u"ConfigDialog/window_size"_s, QSize(700,470)).toSize());
        QList<QVariant> var_sizes = settings.value(u"ConfigDialog/splitter_sizes"_s).toList();
        if(var_sizes.count() != 2)
        {
            var_sizes.clear();
            var_sizes << 180 << q_ptr->width() - 180;
        }
        QList<int> sizes = { var_sizes.constFirst().toInt(), var_sizes.constLast().toInt() };
        splitter->setSizes(sizes);
        //fonts
        QFont font;
        font.fromString(settings.value(u"CueEditor/font"_s, qApp->font("QPlainTextEdit").toString()).toString());
        cueFontLabel->setText(font.family() + QChar::Space + QString::number(font.pointSize()));
        cueFontLabel->setFont(font);
        cueSystemFontCheckBox->setChecked(settings.value(u"CueEditor/use_system_font"_s, true).toBool());
    }

    void loadPluginsInfo()
    {
        treeWidget->blockSignals(true);
        //load transport plugin information
        QTreeWidgetItem *item = new QTreeWidgetItem(treeWidget, { tr("Transports") });
        item->setFirstColumnSpanned(true);
        for(InputSourceFactory *factory : InputSource::factories())
        {
            new PluginItem(item, factory,  InputSource::file(factory));
        }
        treeWidget->addTopLevelItem(item);
        item->setExpanded(true);
        //load input plugins information
        item = new QTreeWidgetItem(treeWidget, { tr("Decoders") });
        item->setFirstColumnSpanned(true);
        for(DecoderFactory *factory : Decoder::factories())
        {
            new PluginItem(item, factory,  Decoder::file(factory));
        }
        treeWidget->addTopLevelItem(item);
        item->setExpanded(true);
        //load audio engines information
        item = new QTreeWidgetItem(treeWidget, { tr("Engines") });
        item->setFirstColumnSpanned(true);
        for(EngineFactory *factory : AbstractEngine::factories())
        {
            new PluginItem(item, factory,  AbstractEngine::file(factory));
        }
        treeWidget->addTopLevelItem(item);
        item->setExpanded(true);
        item->setHidden(!item->childCount());
        //load effect plugin information
        item = new QTreeWidgetItem(treeWidget, { tr("Effects") });
        item->setFirstColumnSpanned(true);
        for(EffectFactory *factory : Effect::factories())
        {
            new PluginItem(item, factory, Effect::file(factory));
        }
        treeWidget->addTopLevelItem(item);
        item->setExpanded(true);
        //load visual plugin information
        item = new QTreeWidgetItem(treeWidget, { tr("Visualization") });
        item->setFirstColumnSpanned(true);
        for(VisualFactory *factory : Visual::factories())
        {
            new PluginItem(item, factory, Visual::file(factory));
        }
        treeWidget->addTopLevelItem(item);
        item->setExpanded(true);
        item->setHidden(!item->childCount());
        //load general plugin information
        item = new QTreeWidgetItem(treeWidget, { tr("General") });
        item->setFirstColumnSpanned(true);
        for(GeneralFactory *factory : General::factories())
        {
            new PluginItem(item, factory, General::file(factory));
        }
        treeWidget->addTopLevelItem(item);
        item->setExpanded(true);
        //load output plugins information
        item = new QTreeWidgetItem(treeWidget, { tr("Output") });
        item->setFirstColumnSpanned(true);
        for(OutputFactory *factory : Output::factories())
        {
            new PluginItem(item, factory, Output::file(factory));
        }
        treeWidget->addTopLevelItem(item);
        item->setExpanded(true);
        //load file dialogs information
        item = new QTreeWidgetItem(treeWidget, { tr("File Dialogs") });
        item->setFirstColumnSpanned(true);
        for(FileDialogFactory *factory : FileDialog::factories())
        {
            new PluginItem(item, factory, FileDialog::file(factory));
        }
        treeWidget->addTopLevelItem(item);
        item->setExpanded(true);
        item->setHidden(!item->childCount());
        //load user interfaces information
        item = new QTreeWidgetItem(treeWidget, { tr("User Interfaces") });
        item->setFirstColumnSpanned(true);
        for(UiFactory *factory : UiLoader::factories())
        {
            new PluginItem(item, factory, UiLoader::file(factory));
        }
        treeWidget->addTopLevelItem(item);
        item->setExpanded(true);

        treeWidget->blockSignals(false);
        treeWidget->resizeColumnToContents(0);
        treeWidget->resizeColumnToContents(1);
    }

    void createMenus()
    {
        Q_Q(::ConfigDialog);
        MetaDataFormatterMenu *groupMenu = new MetaDataFormatterMenu(MetaDataFormatterMenu::GROUP_MENU, q);
        groupButton->setMenu(groupMenu);
        groupButton->setPopupMode(QToolButton::InstantPopup);
        q->connect(groupMenu, &MetaDataFormatterMenu::patternSelected, q, [this] (const QString &str) {
            groupLineEdit->insert(groupLineEdit->cursorPosition() < 1 ? str : (u" - "_s + str));
        });

        MetaDataFormatterMenu *extraRowFormatMenu = new MetaDataFormatterMenu(MetaDataFormatterMenu::GROUP_EXTRA_ROW_MENU, q);
        extraRowFormatButton->setMenu(extraRowFormatMenu);
        extraRowFormatButton->setPopupMode(QToolButton::InstantPopup);
        q->connect(extraRowFormatMenu, &MetaDataFormatterMenu::patternSelected, q, [this] (const QString &str) {
            extraRowFormatLineEdit->insert(extraRowFormatLineEdit->cursorPosition() < 1 ? str : (u" - "_s + str));
        });

        treeWidget->setContextMenuPolicy(Qt::ActionsContextMenu);
#if QT_VERSION >= QT_VERSION_CHECK(6, 3, 0)
        preferencesAction = treeWidget->addAction(QIcon::fromTheme(u"configure"_s), tr("Preferences"));
        preferencesAction->setEnabled(false);
        informationAction = treeWidget->addAction(QIcon::fromTheme(u"dialog-information"_s), tr("Information"));
        informationAction->setEnabled(false);
        priorityAction = treeWidget->addAction(QIcon::fromTheme(u"format-list-ordered"_s), tr("Priority"));
        priorityAction->setVisible(false);
#else
        preferencesAction = new QAction(QIcon::fromTheme(u"configure"_s), tr("Preferences"), treeWidget);
        treeWidget->addAction(preferencesAction);
        preferencesAction->setEnabled(false);
        informationAction = new QAction(QIcon::fromTheme(u"dialog-information"_s), tr("Information"));
        treeWidget->addAction(informationAction);
        informationAction->setEnabled(false);
        priorityAction = new QAction(QIcon::fromTheme(u"format-list-ordered"_s), tr("Priority"));
        treeWidget->addAction(priorityAction);
        priorityAction->setVisible(false);
#endif
        q->connect(preferencesAction, &QAction::triggered, q, [this] { onPreferencesButtonClicked(); });
        q->connect(informationAction, &QAction::triggered, q, [this] { onInformationButtonClicked(); });
        q->connect(priorityAction, &QAction::triggered, q, [this] { onPriorityActionTriggeted(); });
    }

    void loadLanguages()
    {
        const QMap<QString, QString> l = {
                                          { u"auto"_s, tr("<Autodetect>") },
                                          { u"pt_BR"_s, tr("Brazilian Portuguese") },
                                          { u"zh_CN"_s, tr("Chinese Simplified") },
                                          { u"zh_TW"_s, tr("Chinese Traditional") },
                                          { u"cs"_s, tr("Czech") },
                                          { u"nl"_s, tr("Dutch") },
                                          { u"en_US"_s, tr("English") },
                                          { u"fr"_s, tr("French") },
                                          { u"gl_ES"_s, tr("Galician") },
                                          { u"de"_s, tr("German") },
                                          { u"el"_s, tr("Greek") },
                                          { u"he"_s, tr("Hebrew") },
                                          { u"hu"_s, tr("Hungarian") },
                                          { u"id"_s, tr("Indonesian") },
                                          { u"it"_s, tr("Italian") },
                                          { u"ja"_s, tr("Japanese") },
                                          { u"kk"_s, tr("Kazakh") },
                                          { u"ko"_s, tr("Korean") },
                                          { u"lt"_s, tr("Lithuanian") },
                                          { u"pl_PL"_s, tr("Polish") },
                                          { u"pt"_s, tr("Portuguese") },
                                          { u"ru_RU"_s, tr("Russian") },
                                          { u"sr_RS"_s, tr("Serbian") },
                                          { u"sk"_s, tr("Slovak") },
                                          { u"sv"_s, tr("Swedish") },
                                          { u"es"_s, tr("Spanish") },
                                          { u"tr"_s, tr("Turkish") },
                                          { u"uk_UA"_s, tr("Ukrainian") },
                                          { u"sr_BA"_s, tr("Serbian (Ijekavian)") },
                                          { u"sr_RS"_s, tr("Serbian (Ekavian)") },
                                          };

        for(auto it = l.cbegin(); it != l.cend(); ++it)
        {
            QString title = it.key() != "auto"_L1 ? QStringLiteral("%1 (%2)").arg(it.value(), it.key()) : it.value();
            langComboBox->addItem(title, it.key());
        }

        QString code = Qmmp::uiLanguageID();
        int index = langComboBox->findData(code);
        if(index < 0)
            index = langComboBox->findData(u"auto"_s);
        langComboBox->setCurrentIndex(index);
    }

    ::ConfigDialog *q_ptr;
    int insertRow = 0;
    QAction *preferencesAction;
    QAction *informationAction;
    QAction *priorityAction;
};

ConfigDialog::ConfigDialog(QWidget *parent) :
    QDialog(parent),
    d_ptr(new ConfigDialogPrivate(this))
{
    Q_D(ConfigDialog);
    d->setupUi(this);
    setAttribute(Qt::WA_QuitOnClose, false);
    setAttribute(Qt::WA_DeleteOnClose, false);
    d->preferencesButton->setEnabled(false);
    d->informationButton->setEnabled(false);
    d->treeWidget->setItemDelegate(new RadioItemDelegate(this));
    d->treeWidget->header()->setSectionsMovable(false);
    d->linesPerGroupComboBox->addItem(tr("1 row"), 1);
    d->linesPerGroupComboBox->addItem(tr("3 rows"), 3);
    d->linesPerGroupComboBox->addItem(tr("4 rows"), 4);
    d->linesPerGroupComboBox->addItem(tr("5 rows"), 5);
    d->replayGainModeComboBox->addItem(tr("Track"), QmmpSettings::REPLAYGAIN_TRACK);
    d->replayGainModeComboBox->addItem(tr("Album"), QmmpSettings::REPLAYGAIN_ALBUM);
    d->replayGainModeComboBox->addItem(tr("Disabled"), QmmpSettings::REPLAYGAIN_DISABLED);
    d->bitDepthComboBox->addItem(u"16"_s, Qmmp::PCM_S16LE);
    d->bitDepthComboBox->addItem(u"24"_s, Qmmp::PCM_S24LE);
    d->bitDepthComboBox->addItem(u"32"_s, Qmmp::PCM_S32LE);
    d->bitDepthComboBox->addItem(u"32 (float)"_s, Qmmp::PCM_FLOAT);
    d->proxyTypeComboBox->addItem(tr("HTTP"), QmmpSettings::HTTP_PROXY);
    d->proxyTypeComboBox->addItem(tr("SOCKS5"), QmmpSettings::SOCKS5_PROXY);
    d->portLineEdit->setValidator(new QIntValidator(0, 65535, this));
    d->readSettings();
    d->loadPluginsInfo();
    d->loadLanguages();
    d->createMenus();
    d->updateGroupSettings();
    //connections
    connect(d->contentsWidget, &QListWidget::currentItemChanged, [d] (QListWidgetItem *current, QListWidgetItem *previous) {
        d->onContentsWidgetCurrentItemChanged(current, previous);
    });
    connect(d->preferencesButton, &QPushButton::clicked, [d] { d->onPreferencesButtonClicked(); });
    connect(d->informationButton, &QPushButton::clicked, [d] { d->onInformationButtonClicked(); });
    connect(d->treeWidget, &QTreeWidget::itemChanged, [d] (QTreeWidgetItem *item, int column) {
        d->onTreeWidgetItemChanged(item, column);
    });
    connect(d->treeWidget, &QTreeWidget::currentItemChanged, [d] (QTreeWidgetItem *current) {
        d->onTreeWidgetCurrentItemChanged(current);
    });
    connect(d->cueFontButton, &QPushButton::clicked, [d] { d->on_cueFontButton_clicked(); });
    connect(this, &QDialog::accepted, this, [d] { d->saveSettings(); });
    connect(this, &QDialog::rejected, this, [d] { d->revertSettings(); });
    connect(d->linesPerGroupComboBox, &QComboBox::currentIndexChanged, this, [d] { d->updateGroupSettings(); });
    connect(d->showExtraRowCheckBox, &QCheckBox::clicked, this, [d] { d->updateGroupSettings(); });
}

ConfigDialog::~ConfigDialog()
{
    delete d_ptr;
}

void ConfigDialog::addPage(const QString &name, QWidget *widget, const QIcon &icon)
{
    Q_D(ConfigDialog);
    d->stackedWidget->insertWidget(d->insertRow, widget);
    d->contentsWidget->insertItem (d->insertRow, name);
    d->contentsWidget->item(d->insertRow)->setIcon(icon);
    d->contentsWidget->setCurrentRow(0);
    d->insertRow++;
}
