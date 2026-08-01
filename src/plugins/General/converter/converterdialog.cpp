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
#include <QStandardPaths>
#include <QMenu>
#include <QFile>
#include <QDir>
#include <QProgressBar>
#include <QThreadPool>
#include <QProcess>
#include <QMessageBox>
#include <qmmpui/playlistitem.h>
#include <qmmpui/metadataformatter.h>
#include <qmmpui/filedialog.h>
#include <qmmp/metadatamanager.h>
#include <qmmpui/metadataformattermenu.h>
#include "converter.h"
#include "converterpreseteditor.h"
#include "ui_converterdialog.h"
#include "converterdialog.h"

ConverterDialog::ConverterDialog(const QList<PlayListTrack *> &tracks,  QWidget *parent) : QDialog(parent),
    m_ui(new Ui::ConverterDialog)
{
    m_ui->setupUi(this);
    m_ui->tableWidget->verticalHeader()->setDefaultSectionSize(fontMetrics().height() + 3);
    m_ui->tableWidget->verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);

    QStringList paths;
    MetaDataFormatter formatter(u"%if(%p&%t,%p - %t,%f) - %l"_s);
    for(const PlayListTrack *track : std::as_const(tracks))
    {
        //skip streams
        if(track->duration() == 0 && track->path().contains(u"://"_s))
            continue;
        //skip duplicates
        if(paths.contains(track->path()))
            continue;
        //skip unsupported files
        if(track->path().contains(u"://"_s))
        {
            if(!Decoder::findByProtocol(track->path().section(u"://"_s, 0, 0)))
                continue;
        }
        else if(!MetaDataManager::instance()->supports(track->path()))
            continue;

        paths.append(track->path());
        QString name = formatter.format(track);
        QTableWidgetItem *item = new QTableWidgetItem(name);
        item->setData(Qt::UserRole, track->path());
        item->setData(Qt::ToolTipRole, track->path());
        m_ui->tableWidget->insertRow(m_ui->tableWidget->rowCount());
        m_ui->tableWidget->setItem(m_ui->tableWidget->rowCount() - 1, 0, item);
        QProgressBar *progressBar = new QProgressBar(this);
        progressBar->setRange(0, 100);
        m_ui->tableWidget->setCellWidget(m_ui->tableWidget->rowCount() - 1, 1, progressBar);
        m_ui->tableWidget->setItem(m_ui->tableWidget->rowCount() - 1, 2, new QTableWidgetItem());
    }
    m_ui->tableWidget->resizeColumnsToContents();

    QSettings settings;
    settings.beginGroup(u"Converter"_s);
    QString music_path = QStandardPaths::writableLocation(QStandardPaths::MusicLocation);
    m_ui->outDirEdit->setText(settings.value(u"out_dir"_s, music_path).toString());
    m_ui->outFileEdit->setText(settings.value(u"file_name"_s, u"%p - %t"_s).toString());
    m_ui->overwriteCheckBox->setChecked(settings.value(u"overwrite"_s, false).toBool());
    restoreGeometry(settings.value(u"geometry"_s).toByteArray());
    settings.endGroup();
    createMenus();

    readPresets(u":/converter/presets.conf"_s);
    readPresets(Qmmp::configDir() + u"/converter/presets.conf"_s);
}

ConverterDialog::~ConverterDialog()
{
    savePresets();
    on_stopButton_clicked();
    delete m_ui;
}

QVariantHash ConverterDialog::preset() const
{
    if(m_ui->presetComboBox->currentIndex() == -1)
        return QVariantHash();
    int index = m_ui->presetComboBox->currentIndex();
    //aditional parameters
    QVariantHash preset = m_ui->presetComboBox->itemData(index).toHash();
    preset[u"out_dir"_s] = m_ui->outDirEdit->text();
    preset[u"file_name"_s] = m_ui->outFileEdit->text();
    preset[u"overwrite"_s] = m_ui->overwriteCheckBox->isChecked();
    return preset;
}

void ConverterDialog::on_dirButton_clicked()
{
    QString dir = FileDialog::getExistingDirectory(this, tr("Choose a directory"), m_ui->outDirEdit->text());
    if(!dir.isEmpty())
        m_ui->outDirEdit->setText(dir);
}

void ConverterDialog::on_convertButton_clicked()
{
    if(!checkPreset(preset()))
        return;

    m_ui->convertButton->setEnabled(false);
    m_converters.clear();
    for(int i = 0; i < m_ui->tableWidget->rowCount(); ++i)
    {
        QString url = m_ui->tableWidget->item(i, 0)->data(Qt::UserRole).toString();
        Converter *converter = new Converter();


        if(!converter->prepare(url, i, preset()))
        {
            m_ui->tableWidget->item(i, 2)->setText(tr("Error"));
            delete converter;
            continue;
        }

        m_ui->tableWidget->item(i, 2)->setText(tr("Waiting"));

        converter->setAutoDelete(false);
        m_converters.append(converter);
        connect(converter, &Converter::progress, static_cast<QProgressBar *>(m_ui->tableWidget->cellWidget(i, 1)), &QProgressBar::setValue);
        connect(converter, &Converter::finished, this, &ConverterDialog::onConvertFinished);
        connect(converter, &Converter::message, this, &ConverterDialog::onStateChanged);
        QThreadPool::globalInstance()->start(converter);
    }
    m_ui->tableWidget->resizeColumnsToContents();
}

void ConverterDialog::on_stopButton_clicked()
{
    if(m_converters.isEmpty())
        return;

    for(Converter *c : std::as_const(m_converters))
        c->stop();
    QThreadPool::globalInstance()->waitForDone();
    qDeleteAll(m_converters);
    m_converters.clear();
    m_ui->convertButton->setEnabled(true);
}

void ConverterDialog::onStateChanged(int row, const QString &message)
{
    m_ui->tableWidget->item(row, 2)->setText(message);
    m_ui->tableWidget->resizeColumnsToContents();
}

void ConverterDialog::onConvertFinished(Converter *c)
{
    if(m_converters.contains(c))
    {
        m_converters.removeAll(c);
        delete c;
    }
    if(m_converters.isEmpty())
        m_ui->convertButton->setEnabled(true);
}

void ConverterDialog::reject()
{
    QSettings settings;
    settings.beginGroup(u"Converter"_s);
    settings.setValue(u"out_dir"_s, m_ui->outDirEdit->text());
    settings.value(u"file_name"_s, m_ui->outFileEdit->text());
    settings.setValue(u"overwrite"_s, m_ui->overwriteCheckBox->isChecked());
    settings.setValue(u"geometry"_s, saveGeometry());
    settings.endGroup();
    QDialog::reject();
}

void ConverterDialog::createMenus()
{
    MetaDataFormatterMenu *fileNameMenu = new MetaDataFormatterMenu(MetaDataFormatterMenu::TITLE_MENU, this);
    m_ui->fileNameButton->setMenu(fileNameMenu);
    m_ui->fileNameButton->setPopupMode(QToolButton::InstantPopup);
    connect(fileNameMenu, &MetaDataFormatterMenu::patternSelected, this, &ConverterDialog::addTitleString);

    QMenu *presetMenu = new QMenu(this);
    presetMenu->addAction(tr("Create"), this, &ConverterDialog::createPreset);
    presetMenu->addAction(tr("Edit"), this, &ConverterDialog::editPreset);
    presetMenu->addAction(tr("Create a Copy"), this, &ConverterDialog::copyPreset);
    presetMenu->addAction(tr("Delete"), this, &ConverterDialog::deletePreset);
    m_ui->presetButton->setMenu(presetMenu);
    m_ui->presetButton->setPopupMode(QToolButton::InstantPopup);
}

void ConverterDialog::addTitleString(const QString &str)
{
    if (m_ui->outFileEdit->cursorPosition () < 1)
        m_ui->outFileEdit->insert(str);
    else
        m_ui->outFileEdit->insert(u" - "_s + str);
}

void ConverterDialog::createPreset()
{
    PresetEditor *editor = new PresetEditor(QVariantHash(), this);
    if(editor->exec() == QDialog::Accepted)
    {
        QVariantHash data = editor->data();
        data[u"name"_s] = uniqueName(data[u"name"_s].toString());
        if(data[u"name"_s].isValid() && data[u"ext"_s].isValid() && data[u"command"_s].isValid())
            m_ui->presetComboBox->addItem (data[u"name"_s].toString(), data);
    }
    editor->deleteLater();
}

void ConverterDialog::editPreset()
{
    if(m_ui->presetComboBox->currentIndex() == -1)
        return;
    int index = m_ui->presetComboBox->currentIndex();

    PresetEditor *editor = new PresetEditor(m_ui->presetComboBox->itemData(index).toHash(), this);
    if(editor->exec() == QDialog::Accepted)
    {
        QVariantHash data = editor->data();
        if(m_ui->presetComboBox->currentText() != data[u"name"_s].toString())
            data[u"name"_s] = uniqueName(data[u"name"_s].toString());
        if(data[u"name"_s].isValid() && data[u"ext"_s].isValid() && data[u"command"_s].isValid())
        {
            m_ui->presetComboBox->setItemText(index, data[u"name"_s].toString());
            m_ui->presetComboBox->setItemData(index, data);
        }
    }
    editor->deleteLater();
}

void ConverterDialog::copyPreset()
{
    if(m_ui->presetComboBox->currentIndex() == -1)
        return;
    int index = m_ui->presetComboBox->currentIndex();
    QVariantHash data = m_ui->presetComboBox->itemData(index).toHash();
    data[u"name"_s] = uniqueName(data[u"name"_s].toString());
    data[u"read_only"_s] = false;
    m_ui->presetComboBox->addItem (data[u"name"_s].toString(), data);
}

void ConverterDialog::deletePreset()
{
    if(m_ui->presetComboBox->currentIndex() == -1)
        return;
    if(m_ui->presetComboBox->itemData(m_ui->presetComboBox->currentIndex()).toHash()[u"read_only"_s].toBool())
        return;
    m_ui->presetComboBox->removeItem(m_ui->presetComboBox->currentIndex());
}

void ConverterDialog::readPresets(const QString &path)
{
    QFile file(path);
    if(!file.open(QIODevice::ReadOnly))
        return;

    QList<QVariantHash> dataList;
    while(!file.atEnd())
    {
        QString line = QString::fromUtf8(file.readLine().trimmed());
        QStringList list = line.split(QLatin1Char('='));

        if(list.size() < 2)
            continue;

        QString key = list.at(0);
        QString value = list.at(1);
        if(key == "name"_L1)
            dataList.append(QVariantHash());
        if(dataList.isEmpty())
            continue;
        if(key == "use_16bit"_L1 && key == "tags"_L1) //boolean keys
            dataList.last()[key] = (value == "true"_L1);
        else
            dataList.last()[key] = value;
    }

    for(QVariantHash data : std::as_const(dataList))
    {
        data[u"read_only"_s] = path.startsWith(u":/"_s);
        QString title = data[u"name"_s].toString();
        if(data[u"read_only"_s].toBool())
            title += u" *"_s;
        m_ui->presetComboBox->addItem(title, data);
    }
}

void ConverterDialog::savePresets()
{
    QDir dir(Qmmp::configDir());
    dir.mkdir(u"converter"_s);

    QFile file(Qmmp::configDir() + u"/converter/presets.conf"_s);
    if(!file.open(QIODevice::WriteOnly | QIODevice::Truncate))
    {
        qCWarning(plugin, "unable to save presets; error %s", qPrintable(file.errorString()));
        return;
    }

    for(int i = 0; i < m_ui->presetComboBox->count(); ++i)
    {
        QVariantHash data = m_ui->presetComboBox->itemData(i).toHash();
        if(data[u"read_only"_s].toBool())
            continue;
        file.write(QStringLiteral("%1=%2\n").arg(u"name"_s, data[u"name"_s].toString()).toUtf8());
        file.write(QStringLiteral("%1=%2\n").arg(u"ext"_s, data[u"ext"_s].toString()).toUtf8());
        file.write(QStringLiteral("%1=%2\n").arg(u"command"_s, data[u"command"_s].toString()).toUtf8());
        file.write(QStringLiteral("%1=%2\n").arg(u"use_16bit"_s, data[u"use_16bit"_s].toBool() ? u"true"_s : u"false"_s).toUtf8());
        file.write(QStringLiteral("%1=%2\n").arg(u"tags"_s, data[u"tags"_s].toBool() ? u"true"_s : u"false"_s).toUtf8());
        file.write("\n");
    }
}

QString ConverterDialog::uniqueName(const QString &name)
{
    QString unique_name = name;
    int i = 0;
    while (m_ui->presetComboBox->findText(unique_name) >= 0)
        unique_name = name + QStringLiteral("_%1").arg(++i);

    return unique_name;
}

bool ConverterDialog::checkPreset(const QVariantHash &preset)
{
    QStringList programAndArgs = preset[u"command"_s].toString().split(QChar::Space, Qt::SkipEmptyParts);
    if(programAndArgs.isEmpty())
        return false;

    QString program = programAndArgs.constFirst();

    int result = QProcess::execute(program, QStringList());

    if(result == -2)
    {
        QMessageBox::warning(this, tr("Error"), tr("Unable to execute \"%1\". Program not found.")
                             .arg(program));
        return false;
    }

    if(result < 0)
    {
        QMessageBox::warning(this, tr("Error"), tr("Process \"%1\" finished with error.")
                             .arg(program));
        return false;
    }
    return true;
}
