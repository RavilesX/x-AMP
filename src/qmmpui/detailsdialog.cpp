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
#include <QDesktopServices>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QPushButton>
#include <QStyle>
#include <qmmp/metadatamanager.h>
#include <qmmp/metadatamodel.h>
#include <qmmp/tagmodel.h>
#include <qmmp/soundcore.h>
#include "ui_detailsdialog.h"
#include "metadataformatter.h"
#include "playlisttrack.h"
#include "tageditor_p.h"
#include "covereditor_p.h"
#include "cueeditor_p.h"
#include "lyricseditor_p.h"
#include "detailsdialog.h"

class DetailsDialogPrivate : public Ui::DetailsDialog
{
    Q_DECLARE_PUBLIC(::DetailsDialog)
    Q_DECLARE_TR_FUNCTIONS(DetailsDialog)
public:
    DetailsDialogPrivate(const QList<PlayListTrack *> &t, ::DetailsDialog *dialog) :
        q_ptr(dialog),
        tracks(t)
    {
        for(PlayListTrack *t : std::as_const(tracks))
            t->beginUsage();
    }

    ~DetailsDialogPrivate()
    {
        for(PlayListTrack *t : std::as_const(tracks))
        {
            t->endUsage();
            if (!t->isUsed() && t->isSheduledForDeletion())
            {
                delete t;
                t = nullptr;
            }
        }

        if(!modifiedPaths.isEmpty())
            emit q_ptr->metaDataChanged(modifiedPaths.values());

        if(metaDataModel)
        {
            delete metaDataModel;
            metaDataModel = nullptr;
        }
    }

    void updatePage()
    {
        Q_Q(::DetailsDialog);
        if(metaDataModel)
        {
            delete metaDataModel;
            metaDataModel = nullptr;
        }

        while(tabWidget->count() > 1)
        {
            int index = tabWidget->count() - 1;
            QWidget *w = tabWidget->widget(index);
            tabWidget->removeTab(index);
            w->deleteLater();
        }

        pageLabel->setText(tr("%1/%2").arg(page + 1).arg(tracks.count()));
        if(page >= 0 && page < tracks.count())
            info = *tracks.at(page);
        else
            info.clear();

        q->setWindowTitle(info.path().section(QLatin1Char('/'),-1));
        pathEdit->setText(info.path());

        //load metadata and create metadata model
        QList<TrackInfo> infoList = MetaDataManager::instance()->createPlayList(info.path());
        if(!infoList.isEmpty())
        {
            if(infoList.constFirst().parts() & TrackInfo::MetaData)
                info.setValues(infoList.constFirst().metaData());
            if(infoList.constFirst().parts() & TrackInfo::Properties)
            {
                info.updateValues(infoList.constFirst().properties());
                info.setDuration(infoList.constFirst().duration());
            }
        }
        infoList.clear();

        QString coverPath;
        QImage coverImage;
        bool readOnly = false;

        if(info.path().contains(u"://"_s) && info.path().contains(QLatin1Char('#'))) //track of multi-track file
        {
            QString filePath = TrackInfo::pathFromUrl(info.path());
            if(QFileInfo(filePath).isFile())
                readOnly = !QFileInfo(filePath).isWritable() || !QFile::exists(filePath);
        }
        else if(!info.path().contains(u"://"_s)) //local file
        {
            coverPath = MetaDataManager::instance()->findCoverFile(info.path());
            readOnly = !QFileInfo(info.path()).isWritable() || !QFile::exists(info.path());
        }

        metaDataModel = MetaDataManager::instance()->createMetaDataModel(info.path(), readOnly);

        if(metaDataModel)
        {
            coverPath = coverPath.isEmpty() ? metaDataModel->coverPath() : coverPath;
            coverImage = metaDataModel->cover();
        }

        if((metaDataModel && (metaDataModel->dialogHints() & MetaDataModel::IsCoverEditable)) ||
            !coverPath.isEmpty() ||
            !coverImage.isNull())
        {
            CoverEditor *coverEditor = new CoverEditor(metaDataModel, coverPath, q);
            tabWidget->addTab(coverEditor, tr("Cover"));
        }

        if(metaDataModel)
        {
            for(TagModel *tagModel : metaDataModel->tags())
            {
                TagEditor *editor = new TagEditor(tagModel, metaDataModel->isReadOnly(), q);
                editor->setEnabled(!metaDataModel->isReadOnly());
                tabWidget->addTab(editor, tagModel->name());
            }

            for(const MetaDataItem &item : metaDataModel->descriptions())
            {
                QTextEdit *textEdit = new QTextEdit(q);
                textEdit->setReadOnly(true);
                textEdit->setPlainText(item.value().toString());
                tabWidget->addTab(textEdit, item.name());
            }

            QString lyrics = metaDataModel->lyrics();
            if(!lyrics.isEmpty() || metaDataModel->dialogHints() & MetaDataModel::IsLyricsEditable)
            {
                LyricsEditor *lyricsEditor = new LyricsEditor(metaDataModel, info, q);
                tabWidget->addTab(lyricsEditor, tr("Lyrics"));
            }

            if(metaDataModel->dialogHints() & MetaDataModel::IsCueEditable)
            {
                CueEditor *cueEditor = new CueEditor(metaDataModel, info, q);
                tabWidget->addTab(cueEditor, u"CUE"_s);
            }
        }

        for(int i = 0; i < tabWidget->count(); ++i) {
            EditorBase *editor = qobject_cast<EditorBase *>(tabWidget->widget(i));
            if(editor)
            {
                CoverEditor::connect(editor, &EditorBase::modifiedChanged, q, [this] { updateDialogButtons(); });
            }
        }

        printInfo();
    }

    void printInfo()
    {
        Q_Q(::DetailsDialog);
        SoundCore *core = SoundCore::instance();
        QString formattedText, metaDataRows, streamInfoRows, propertyRows;
        QStringList tableParts;

        //tags
        metaDataRows += formatRow(tr("Title"), info.value(Qmmp::TITLE));
        metaDataRows += formatRow(tr("Artist"), info.value (Qmmp::ARTIST));
        metaDataRows += formatRow(tr("Album artist"), info.value(Qmmp::ALBUMARTIST));
        metaDataRows += formatRow(tr("Album"), info.value(Qmmp::ALBUM));
        metaDataRows += formatRow(tr("Comment"), info.value(Qmmp::COMMENT));
        metaDataRows += formatRow(tr("Genre"), info.value(Qmmp::GENRE));
        metaDataRows += formatRow(tr("Composer"), info.value(Qmmp::COMPOSER));
        metaDataRows += formatRow(tr("Year"), info.value(Qmmp::YEAR));
        metaDataRows += formatRow(tr("Track"), info.value(Qmmp::TRACK));
        metaDataRows += formatRow(tr("Disc number"), info.value(Qmmp::DISCNUMBER));
        metaDataRows = metaDataRows.trimmed();
        if(!metaDataRows.isEmpty())
            tableParts << metaDataRows;

        //stream information
        if(core->state() == Qmmp::Playing && core->path() == info.path())
        {
            const QHash<QString, QString> &streamInfo = core->streamInfo();
            for(auto it = streamInfo.cbegin(); it != streamInfo.cend(); ++it)
                streamInfoRows += formatRow(it.key(), it.value());
        }
        streamInfoRows = streamInfoRows.trimmed();
        if(!streamInfoRows.isEmpty())
            tableParts << streamInfoRows;

        //properties
        QList<MetaDataItem> items;
        if(info.duration() > 0)
            items << MetaDataItem(tr("Duration"), MetaDataFormatter::formatDuration(info.duration()));
        if(!metaDataModel || !(metaDataModel->dialogHints() & MetaDataModel::CompletePropertyList))
        {
            items << MetaDataItem(tr("Bitrate"), info.value(Qmmp::BITRATE).toInt(), tr("kbps"));
            items << MetaDataItem(tr("Sample rate"), info.value(Qmmp::SAMPLERATE).toInt(), tr("Hz"));
            items << MetaDataItem(tr("Channels"), info.value(Qmmp::CHANNELS).toInt());
            items << MetaDataItem(tr("Sample size"), info.value(Qmmp::BITS_PER_SAMPLE).toInt(), tr("bits"));
            items << MetaDataItem(tr("Format name"), info.value(Qmmp::FORMAT_NAME));
            items << MetaDataItem(tr("File size"), info.value(Qmmp::FILE_SIZE).toInt() / 1024, tr("KiB"));
        }
        if(metaDataModel)
            items << metaDataModel->extraProperties();
        for(const MetaDataItem &item : std::as_const(items))
            propertyRows += formatRow(item);
        propertyRows = propertyRows.trimmed();
        if(!propertyRows.isEmpty())
            tableParts << propertyRows;

        //create table
        if(q->layoutDirection() == Qt::RightToLeft)
            formattedText.append(u"<DIV align=\"right\" dir=\"rtl\">"_s);
        else
            formattedText.append(u"<DIV>"_s);
        formattedText.append(u"<TABLE>"_s);

        formattedText += tableParts.join(u"<tr><td colspan=2><hr></td></tr>"_s);

        formattedText.append(u"</TABLE>"_s);
        formattedText.append(u"</DIV>"_s);
        textEdit->setHtml(formattedText);
    }

    QString formatRow(const QString &key, const QString &value) const
    {
        Q_Q(const ::DetailsDialog);
        if(value.isEmpty() || key.isEmpty())
            return QString();
        QString str(u"<tr>"_s);
        if(q->layoutDirection() == Qt::RightToLeft)
            str.append(u"<td>"_s + value + u"</td> <td style=\"padding-left: 15px;\"><b>"_s + key + u"</b></td>"_s);
        else
            str.append(u"<td><b>"_s + key + u"</b></td> <td style=\"padding-left: 15px;\">"_s + value + u"</td>"_s);
        str.append(u"</tr>"_s);
        return str;
    }

    QString formatRow(const MetaDataItem &item) const
    {
        if(item.value().isNull() || item.name().isEmpty() || !item.value().isValid())
            return QString();

        QString value;
        if(item.value().typeId() == QMetaType::Bool)
            value = item.value().toBool() ? tr("Yes") : tr("No");
        else if(item.value().typeId() == QMetaType::Double)
            value = QStringLiteral("%1").arg(item.value().toDouble(), 0, 'f', 4);
        else
            value = item.value().toString();

        if(value.isEmpty() || value == "0"_L1 || value == "0.0000"_L1)
            return QString();

        if(!item.suffix().isEmpty())
            value += QChar::Space + item.suffix();

        return formatRow(item.name(), value);
    }

    void onDirectoryButtonClicked()
    {
        QString dir_path;
        if(!info.path().contains(u"://"_s)) //local file
            dir_path = QFileInfo(info.path()).absolutePath();
        else if (info.path().contains(u":///"_s)) //pseudo-protocol
        {
            dir_path = QUrl(info.path()).path();
            dir_path.replace(QString::fromLatin1(QUrl::toPercentEncoding(u"#"_s)), u"#"_s);
            dir_path.replace(QString::fromLatin1(QUrl::toPercentEncoding(u"?"_s)), u"?"_s);
            dir_path.replace(QString::fromLatin1(QUrl::toPercentEncoding(u"%"_s)), u"%"_s);
            dir_path = QFileInfo(dir_path).absolutePath();
        }
        else
            return;

        QDesktopServices::openUrl(QUrl::fromLocalFile(dir_path));
    }

    void onButtonBoxClicked(QAbstractButton *button)
    {
        QDialogButtonBox::StandardButton b = buttonBox->standardButton(button);

        if(b == QDialogButtonBox::Save || b == QDialogButtonBox::Ok)
        {
            EditorBase *editor = qobject_cast<EditorBase *>(tabWidget->currentWidget());

            if(editor && editor->isModified())
            {
                editor->save();
                modifiedPaths.insert(info.path());

                CueEditor *cueEditor = qobject_cast<CueEditor *>(editor);
                if(cueEditor)
                {
                    //update all cue tracks
                    static const QRegularExpression trackNumber(u"#\\d+$"_s);
                    int count = cueEditor->trackCount();
                    QString path = info.path();
                    path.remove(trackNumber);
                    for(int i = 0; i < count; ++i)
                        modifiedPaths.insert(QStringLiteral("%1#%2").arg(path).arg(i + 1));
                    modifiedPaths.insert(info.path());
                }

                updateDialogButtons();
            }
        }

        if(b == QDialogButtonBox::Ok || b == QDialogButtonBox::Close)
        {
            //close all files before closing dialog
            if(metaDataModel)
            {
                delete metaDataModel;
                metaDataModel = nullptr;
            }
            q_ptr->reject();
        }
    }

    void updateDialogButtons()
    {
        EditorBase *editor = qobject_cast<EditorBase *>(tabWidget->currentWidget());
        buttonBox->button(QDialogButtonBox::Save)->setEnabled(editor && editor->isModified());
    }

    void onPrevButtonClicked()
    {
        if(page == 0)
            page = tracks.count() - 1;
        else
            page--;
        updatePage();
    }

    void onNextButtonClicked()
    {
        if(page >= tracks.count() - 1)
            page = 0;
        else
            page++;
        updatePage();
    }

private:
    ::DetailsDialog *q_ptr;
    MetaDataModel *metaDataModel = nullptr;
    QList<PlayListTrack *> tracks;
    TrackInfo info;
    int page = 0;
    QSet<QString> modifiedPaths;
};

DetailsDialog::DetailsDialog(const QList<PlayListTrack *> &tracks, QWidget *parent) :
    QDialog(parent),
    d_ptr(new DetailsDialogPrivate(tracks, this))
{
    Q_D(DetailsDialog);
    d->setupUi(this);
    setAttribute(Qt::WA_QuitOnClose, false);
    d->directoryButton->setIcon(QApplication::style()->standardIcon(QStyle::SP_DirOpenIcon));
    d->nextButton->setIcon(QApplication::style()->standardIcon(QStyle::SP_ArrowRight));
    d->prevButton->setIcon(QApplication::style()->standardIcon(QStyle::SP_ArrowLeft));
    d->updatePage();
    d->updateDialogButtons();

    connect(d->buttonBox, &QDialogButtonBox::clicked, this, [d](QAbstractButton *button) { d->onButtonBoxClicked(button); });
    connect(d->tabWidget, &QTabWidget::currentChanged, this, [d] { d->updateDialogButtons(); });
    connect(d->directoryButton, &QToolButton::clicked, this, [d] { d->onDirectoryButtonClicked(); });
    connect(d->prevButton,  &QToolButton::clicked, this, [d] { d->onPrevButtonClicked(); });
    connect(d->nextButton,  &QToolButton::clicked, this, [d] { d->onNextButtonClicked(); });
}

DetailsDialog::DetailsDialog(PlayListTrack *track, QWidget *parent) :
    DetailsDialog(QList<PlayListTrack *>{ track }, parent)
{}

DetailsDialog::~DetailsDialog()
{
    delete d_ptr;
}

QStringList DetailsDialog::modifiedPaths() const
{
    return d_ptr->modifiedPaths.values();
}

void DetailsDialog::closeEvent(QCloseEvent *)
{
    Q_D(DetailsDialog);
    //close all files before closing dialog
    if(d->metaDataModel)
    {
        delete d->metaDataModel;
        d->metaDataModel = nullptr;
    }
}
