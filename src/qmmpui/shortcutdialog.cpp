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

#include <QKeyEvent>
#include <QPushButton>
#include "ui_shortcutdialog.h"
#include "shortcutdialog.h"

class ShortcutDialogPrivate : public Ui::ShortcutDialog
{
public:
    ShortcutDialogPrivate(const QKeySequence &k) : key(k) {}
    QKeySequence key;
};

ShortcutDialog::ShortcutDialog(const QKeySequence &key, QWidget *parent) :
    QDialog(parent),
    d_ptr(new ShortcutDialogPrivate(key))
{
    Q_D(ShortcutDialog);
    d->setupUi(this);
    d->keyLineEdit->setText(key.toString(QKeySequence::NativeText));
    QPushButton *button = d->buttonBox->addButton(tr("Clear"), QDialogButtonBox::ResetRole);
    connect(button, &QPushButton::clicked, d->keyLineEdit, &QLineEdit::clear);
    connect(button, &QPushButton::clicked, this, [d] { d->key = QKeySequence(); });
    connect(this, &QDialog::accepted, this, [this] { releaseKeyboard(); });
    connect(this, &QDialog::rejected, this, [this] { releaseKeyboard(); });
}

ShortcutDialog::~ShortcutDialog()
{
    delete d_ptr;
}

void ShortcutDialog::keyPressEvent(QKeyEvent *event)
{
    Q_D(ShortcutDialog);
    int key = event->key();
    switch(key)
    {
    case Qt::Key_Shift:
    case Qt::Key_Control:
    case Qt::Key_Meta:
    case Qt::Key_Alt:
    case Qt::Key_AltGr:
    case Qt::Key_Super_L:
    case Qt::Key_Super_R:
    case Qt::Key_Menu:
    case 0:
    case Qt::Key_unknown:
        d->keyLineEdit->clear();
        d->key = QKeySequence();
        QWidget::keyPressEvent(event);
        return;
    }
    d->key = QKeySequence(event->keyCombination());
    d->keyLineEdit->setText(d->key.toString(QKeySequence::NativeText));
    QWidget::keyPressEvent(event);
}

void ShortcutDialog::showEvent(QShowEvent *event)
{
    Q_UNUSED(event);
    grabKeyboard();
}

QKeySequence ShortcutDialog::key() const
{
    return d_ptr->key;
}
