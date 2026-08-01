/***************************************************************************
 *   Copyright (C) 2010-2026 by Ilya Kotov                                 *
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

#include "metadataformattermenu.h"
#include "ui_templateeditor.h"
#include "templateeditor.h"

class TemplateEditorPrivate : public Ui::TemplateEditor
{
    Q_DECLARE_PUBLIC(::TemplateEditor)
public:
    TemplateEditorPrivate(::TemplateEditor *editor) : q_ptr(editor)
    {
        setupUi(editor);
    }

private:
    void createMenu()
    {
        Q_Q(::TemplateEditor);
        MetaDataFormatterMenu *menu = new MetaDataFormatterMenu(MetaDataFormatterMenu::TITLE_MENU, q);
        insertButton->setMenu(menu);
        q->connect(menu, &MetaDataFormatterMenu::patternSelected, textEdit, &QPlainTextEdit::insertPlainText);
    }

    ::TemplateEditor *q_ptr;
    QString defaultTemplate;
};

TemplateEditor::TemplateEditor(QWidget *parent) :
    QDialog(parent),
    d_ptr(new TemplateEditorPrivate(this))
{
    Q_D(TemplateEditor);
    d->createMenu();
    connect(d->buttonBox, &QDialogButtonBox::clicked, this, [d](QAbstractButton *button) {
        if(d->buttonBox->standardButton(button) == QDialogButtonBox::RestoreDefaults)
            d->textEdit->setPlainText(d->defaultTemplate);
    });
}

TemplateEditor::~TemplateEditor()
{
    delete d_ptr;
}

QString TemplateEditor::currentTemplate() const
{
    return d_ptr->textEdit->toPlainText();
}

void TemplateEditor::setTemplate(const QString &text)
{
    d_ptr->textEdit->setPlainText(text);
}

void TemplateEditor::setDefaultTemplate(const QString &text)
{
    d_ptr->defaultTemplate = text;
}

QString TemplateEditor::getTemplate(QWidget *parent, const QString &title, const QString &text,
                                    const QString &default_template, bool *ok)
{
    TemplateEditor *editor = new TemplateEditor(parent);
    editor->setWindowTitle(title);
    editor->setTemplate(text);
    editor->setDefaultTemplate(default_template);
    if(editor->exec() == QDialog::Accepted)
    {
        if(ok)
            *ok = true;
        QString t = editor->currentTemplate();
        editor->deleteLater();
        return t;
    }

    if(ok)
        *ok = false;
    editor->deleteLater();
    return QString();
}
