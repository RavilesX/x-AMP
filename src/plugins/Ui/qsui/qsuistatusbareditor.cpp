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

#include <QApplication>
#include <QSettings>
#include <QMap>
#include "qsuistatusbar.h"
#include "qsuistatusbareditor.h"
#include "ui_qsuistatusbareditor.h"

QSUiStatusBarEditor::QSUiStatusBarEditor(QWidget *parent) :
    QDialog(parent),
    m_ui(new Ui::QSUiStatusBarEditor)
{
    m_ui->setupUi(this);
    m_ui->upToolButton->setIcon(qApp->style()->standardIcon(QStyle::SP_ArrowUp));
    m_ui->downToolButton->setIcon(qApp->style()->standardIcon(QStyle::SP_ArrowDown));
    m_ui->addToolButton->setIcon(qApp->style()->standardIcon(QStyle::SP_ArrowRight));
    m_ui->removeToolButton->setIcon(qApp->style()->standardIcon(QStyle::SP_ArrowLeft));

    connect(m_ui->buttonBox, &QDialogButtonBox::clicked, this, [this](QAbstractButton *button) {
        if(m_ui->buttonBox->standardButton(button) == QDialogButtonBox::RestoreDefaults)
           populateLabelList(true);
    });

    populateLabelList(false);
}

QSUiStatusBarEditor::~QSUiStatusBarEditor()
{
    delete m_ui;
}

void QSUiStatusBarEditor::accept()
{
    QVariantList labels;

    for(int i = 0; i < m_ui->enabledLabelsListWidget->count(); ++i)
        labels << m_ui->enabledLabelsListWidget->item(i)->data(Qt::UserRole);

    QSettings settings;
    settings.setValue("Simple/toolbar_labels"_L1, labels);

    QDialog::accept();
}

void QSUiStatusBarEditor::on_addToolButton_clicked()
{
    int row = m_ui->labelsListWidget->currentRow();
    if(row > -1)
    {
        QListWidgetItem *item = m_ui->labelsListWidget->takeItem(row);
        m_ui->enabledLabelsListWidget->addItem(item);
    }
}

void QSUiStatusBarEditor::on_removeToolButton_clicked()
{
    int row = m_ui->enabledLabelsListWidget->currentRow();
    if(row > -1)
    {
        QListWidgetItem *item = m_ui->enabledLabelsListWidget->takeItem(row);
        m_ui->labelsListWidget->addItem(item);
    }
}

void QSUiStatusBarEditor::populateLabelList(bool reset)
{
    m_ui->labelsListWidget->clear();
    m_ui->enabledLabelsListWidget->clear();

    QVariantList labels;

    if(reset)
    {
        labels = QSUiStatusBar::defaultLabels();
    }
    else
    {
        QSettings settings;
        labels = settings.value("Simple/toolbar_labels"_L1, QSUiStatusBar::defaultLabels()).toList();
    }

    const QMap<int, QString> names = {
        { QSUiStatusBar::StatusLabel, tr("Status") },
        { QSUiStatusBar::SampleSizeLabel, tr("Sample Size") },
        { QSUiStatusBar::ChannelsLabel, tr("Number of Channels") },
        { QSUiStatusBar::SampleRateLabel, tr("Sample Rate") },
        { QSUiStatusBar::TrackCountLabel, tr("Track Count") },
        { QSUiStatusBar::TotalTimeLabel, tr("Total Time") },
        { QSUiStatusBar::BitrateLabel, tr("Bitrate") },
        { QSUiStatusBar::ElapsedTimeLabel, tr("Elapsed Time") },
        { QSUiStatusBar::RemainingTimeLabel, tr("Remaining Time") },
        { QSUiStatusBar::DurationLabel, tr("Duration") },
        { QSUiStatusBar::ElapsedAndDurationLabel, tr("Elapsed Time + Duration") },
        { QSUiStatusBar::FormatLabel, tr("Format") },
        { QSUiStatusBar::DecoderLabel, tr("Decoder") },
    };

    for(const QVariant &id : std::as_const(labels))
    {
        QListWidgetItem *item = new QListWidgetItem();
        item->setData(Qt::UserRole, id);
        item->setText(names.value(id.toInt()));
        m_ui->enabledLabelsListWidget->addItem(item);
    }

    for(auto it = names.cbegin(); it != names.cend(); ++it)
    {
        QListWidgetItem *item = new QListWidgetItem();
        item->setData(Qt::UserRole, it.key());
        item->setText(it.value());

        if(!labels.contains(it.key()))
            m_ui->labelsListWidget->addItem(item);
    }
}

void QSUiStatusBarEditor::on_upToolButton_clicked()
{
    int row = m_ui->enabledLabelsListWidget->currentRow();
    if(row > 0)
    {
        QListWidgetItem *item = m_ui->enabledLabelsListWidget->takeItem(row);
        m_ui->enabledLabelsListWidget->insertItem(row - 1, item);
        m_ui->enabledLabelsListWidget->setCurrentItem(item);
    }
}

void QSUiStatusBarEditor::on_downToolButton_clicked()
{
    int row = m_ui->enabledLabelsListWidget->currentRow();
    if(row > -1 && row < m_ui->enabledLabelsListWidget->count())
    {
        QListWidgetItem *item = m_ui->enabledLabelsListWidget->takeItem(row);
        m_ui->enabledLabelsListWidget->insertItem(row + 1, item);
        m_ui->enabledLabelsListWidget->setCurrentItem(item);
    }
}
