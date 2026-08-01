/***************************************************************************
 *   Copyright (C) 2021-2026 by Ilya Kotov                                 *
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
#include <QSettings>
#include <QFileInfo>
#include <QDir>
#include <qmmp/metadatamodel.h>
#include "lyricseditor_p.h"
#include "filedialog.h"
#include "ui_texteditor.h"

LyricsEditor::LyricsEditor(MetaDataModel *model, const TrackInfo &info, QWidget *parent) :
    EditorBase(parent),
    m_ui(new Ui::TextEditor),
    m_model(model),
    m_info(info)
{
    m_ui->setupUi(this);
    m_ui->plainTextEdit->setPlainText(model->lyrics());
    QSettings settings;
    m_lastDir = settings.value(u"LyricsEditor/last_dir"_s,  QDir::homePath()).toString();
    m_editable = m_model && (m_model->dialogHints() & MetaDataModel::IsLyricsEditable) && !m_model->isReadOnly();

    if(!m_editable)
    {
        m_ui->deleteButton->setEnabled(false);
        m_ui->loadButton->setEnabled(false);
        m_ui->plainTextEdit->setReadOnly(true);
    }

    connect(m_ui->plainTextEdit, &QPlainTextEdit::modificationChanged, this, &LyricsEditor::setModified);
}

LyricsEditor::~LyricsEditor()
{
    QSettings settings;
    settings.setValue(u"LyricsEditor/last_dir"_s, m_lastDir);

    delete m_ui;
}

void LyricsEditor::save()
{
    QString data = m_ui->plainTextEdit->toPlainText().trimmed();
    if(data.isEmpty())
    {
        m_model->removeLyrics();
        setModified(false);
    }
    else
    {
        data.append(QChar::LineFeed);
        m_model->setLyrics(data);
    }
    m_ui->plainTextEdit->document()->setModified(false);
}

bool LyricsEditor::isEditable() const
{
    return m_editable;
}

void LyricsEditor::on_loadButton_clicked()
{
    QString path = FileDialog::getOpenFileName(this, tr("Load Lyrics"), m_lastDir, tr("Text Files") + u" (*.txt)"_s);
    if(!path.isEmpty())
    {
        m_lastDir = QFileInfo(path).absoluteDir().path();
        QFile file(path);
        if(file.open(QIODevice::ReadOnly))
            m_ui->plainTextEdit->setPlainText(QString::fromUtf8(file.readAll()));
        else
            m_ui->plainTextEdit->clear();
    }
}

void LyricsEditor::on_deleteButton_clicked()
{
    m_ui->plainTextEdit->clear();
}

void LyricsEditor::on_saveAsButton_clicked()
{
    QString path = FileDialog::getSaveFileName(this, tr("Save Lyrics"),
                                               m_lastDir + QLatin1Char('/') + m_info.value(Qmmp::TITLE) + u".cue"_s,
                                               tr("Text Files") + u" (*.cue)"_s);


    if(!path.isEmpty())
    {
        m_lastDir = QFileInfo(path).absoluteDir().path();
        QString data = m_ui->plainTextEdit->toPlainText().trimmed();
        data.append(QChar::LineFeed);

        QFile file(path);
        if(file.open(QIODevice::WriteOnly))
            file.write(data.toUtf8());
        else
            qCWarning(core) << "unable to save cue file; error:" << file.errorString();
    }
}
