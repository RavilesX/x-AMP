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
#include <QSyntaxHighlighter>
#include <QRegularExpression>
#include <QRegularExpressionMatch>
#include <QRegularExpressionMatchIterator>
#include <qmmp/metadatamodel.h>
#include "cueeditor_p.h"
#include "filedialog.h"
#include "ui_texteditor.h"

class CueSyntaxHighlighter : public QSyntaxHighlighter
{
public:
    CueSyntaxHighlighter(QTextDocument *parent) :  QSyntaxHighlighter(parent) {}

private:
    void highlightBlock(const QString &text) override
    {
        QTextCharFormat textFormat;
        textFormat.setForeground(Qt::darkGreen);

        static const QRegularExpression textExpr(u"\\\".*\\\""_s);
        QRegularExpressionMatchIterator i = textExpr.globalMatch(text);
        while(i.hasNext())
        {
            QRegularExpressionMatch match = i.next();
            setFormat(match.capturedStart(), match.capturedLength(), textFormat);
        }

        QTextCharFormat trackFormat;
        trackFormat.setFontWeight(QFont::Bold);

        static const QRegularExpression trackExpr(u"TRACK\\s+\\d+\\s*\\D*"_s);
        i = trackExpr.globalMatch(text);
        while(i.hasNext())
        {
            QRegularExpressionMatch match = i.next();
            setFormat(match.capturedStart(), match.capturedLength(), trackFormat);
        }
    }
};

CueEditor::CueEditor(MetaDataModel *model, const TrackInfo &info, QWidget *parent) :
    EditorBase(parent),
    m_ui(new Ui::TextEditor),
    m_model(model),
    m_info(info)
{
    m_ui->setupUi(this);
    m_ui->plainTextEdit->setPlainText(model->cue());
    m_parser.loadData(model->cue().toUtf8());
    QSettings settings;
    m_lastDir = settings.value(u"CueEditor/last_dir"_s,  QDir::homePath()).toString();
    if(!settings.value(u"CueEditor/use_system_font"_s, true).toBool())
    {
        QFont font;
        font.fromString(settings.value(u"CueEditor/font"_s, qApp->font("QPlainTextEdit").toString()).toString());
        m_ui->plainTextEdit->setFont(font);
    }

    m_editable = m_model && (m_model->dialogHints() & MetaDataModel::IsCueEditable) && !m_model->isReadOnly();
    new CueSyntaxHighlighter(m_ui->plainTextEdit->document());

    if(!m_editable)
    {
        m_ui->deleteButton->setEnabled(false);
        m_ui->loadButton->setEnabled(false);
        m_ui->plainTextEdit->setReadOnly(true);
    }

    connect(m_ui->plainTextEdit, &QPlainTextEdit::modificationChanged, this, &CueEditor::setModified);
}

CueEditor::~CueEditor()
{
    QSettings settings;
    settings.setValue(u"CueEditor/last_dir"_s, m_lastDir);

    delete m_ui;
}

void CueEditor::save()
{
    QString data = m_ui->plainTextEdit->toPlainText().trimmed();
    if(data.isEmpty())
    {
        m_model->removeCue();
        m_parser.clear();
        setModified(false);
    }
    else
    {
        data.append(QChar::LineFeed);
        m_model->setCue(data);
        m_parser.loadData(data.toUtf8());
    }
    m_ui->plainTextEdit->document()->setModified(false);
}

bool CueEditor::isEditable() const
{
    return m_editable;
}

int CueEditor::trackCount() const
{
    return m_parser.count();
}

void CueEditor::on_loadButton_clicked()
{
    QString path = FileDialog::getOpenFileName(this, tr("Open CUE File"), m_lastDir, tr("CUE Files") + u" (*.cue)"_s);
    if(!path.isEmpty())
    {
        m_lastDir = QFileInfo(path).absoluteDir().path();
        QFile file(path);
        if(file.open(QIODevice::ReadOnly))
            m_ui->plainTextEdit->setPlainText(QString::fromUtf8(file.readAll()));
        else
            m_ui->plainTextEdit->clear();
        setModified(true);
    }
}

void CueEditor::on_deleteButton_clicked()
{
    m_ui->plainTextEdit->clear();
    setModified(true);
}

void CueEditor::on_saveAsButton_clicked()
{
    QString path = FileDialog::getSaveFileName(this, tr("Save CUE File"),
                                               m_lastDir + QLatin1Char('/') + m_info.value(Qmmp::TITLE) + u".cue"_s,
                                               tr("CUE Files") + u" (*.cue)"_s);


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
