#include <QEventLoop>
#include <QThread>
#include <QProgressDialog>
#include <QMetaObject>
#include <qmmp/metadatamanager.h>
#include <unistd.h>
#include "batchtageditordialog.h"
#include "ui_batchtageditordialog.h"

BatchTagEditorDialog::BatchTagEditorDialog(QWidget *parent) :
    QDialog(parent),
    m_ui(new Ui::BatchTagEditorDialog)
{
    m_ui->setupUi(this);

    m_ui->tagComboBox->addItem(tr("Artist"), Qmmp::ARTIST);
    m_ui->tagComboBox->addItem(tr("Album Artist"), Qmmp::ALBUMARTIST);
    m_ui->tagComboBox->addItem(tr("Album"), Qmmp::ALBUM);
    m_ui->tagComboBox->addItem(tr("Comment"), Qmmp::COMMENT);
    m_ui->tagComboBox->addItem(tr("Genre"), Qmmp::GENRE);
    m_ui->tagComboBox->addItem(tr("Composer"), Qmmp::COMPOSER);
    m_ui->tagComboBox->addItem(tr("Year"), Qmmp::YEAR);
    m_ui->tagComboBox->addItem(tr("Disc Number"), Qmmp::DISCNUMBER);
}

BatchTagEditorDialog::~BatchTagEditorDialog()
{
    delete m_ui;
}

void BatchTagEditorDialog::setFiles(const QStringList &paths)
{
    if(paths.isEmpty())
        return;

    m_paths = paths;
    MetaDataModel *model = nullptr;

    //find first editable tag model
    for(const QString &path : std::as_const(paths))
    {
        model = MetaDataManager::instance()->createMetaDataModel(path, false);
        if(!model || model->isReadOnly() || model->tags().isEmpty())
        {
            delete model;
            model = nullptr;
            continue;
        }
        break;
    }

    if(!model)
        return;

    TagModel *tag = model->tags().constFirst();

    for(int i = 0; i < m_ui->tagComboBox->count(); ++i)
    {
        Qmmp::MetaData key = static_cast<Qmmp::MetaData>(m_ui->tagComboBox->itemData(i).toInt());
        m_defaultValues.insert(key, tag->value(key));
    }

    delete model;

    on_tagComboBox_currentIndexChanged(0);
}

void BatchTagEditorDialog::accept()
{
    Qmmp::MetaData key = static_cast<Qmmp::MetaData>(m_ui->tagComboBox->currentData().toInt());
    QString value;
    if(key == Qmmp::YEAR || key == Qmmp::DISCNUMBER)
        value = QString::number(m_ui->tagSpinBox->value());
    else
        value = m_ui->tagValueLineEdit->text();

    hide();

    QProgressDialog progressDialog(tr("Writing tags..."), tr("Interrupt"), 0, m_paths.count(), this);
    progressDialog.setWindowTitle(windowTitle());
    QEventLoop eventLoop(this);

    QThread *thread = QThread::create([this,&progressDialog,key,&value] {
        int i = 0;
        for(const QString &path : std::as_const(m_paths))
        {
            if(QThread::currentThread()->isInterruptionRequested())
                break;

            i++;
            QMetaObject::invokeMethod(&progressDialog, [&progressDialog,i] { progressDialog.setValue(i); });
            MetaDataModel *model = MetaDataManager::instance()->createMetaDataModel(path, false);
            if(!model || model->isReadOnly() || model->tags().isEmpty())
                continue;

            for(TagModel *tag : model->tags())
            {
                if(tag->exists())
                {
                    tag->setValue(key, value);
                    tag->save();
                }
            }
            delete model;
        };
    });

    connect(thread, &QThread::finished, &eventLoop, &QEventLoop::quit);
    connect(&progressDialog, &QProgressDialog::canceled, thread, &QThread::requestInterruption);

    thread->start();
    eventLoop.exec();

    thread->deleteLater();
    QDialog::accept();
}

void BatchTagEditorDialog::on_tagComboBox_currentIndexChanged(int index)
{
    Qmmp::MetaData key = static_cast<Qmmp::MetaData>(m_ui->tagComboBox->itemData(index).toInt());
    QString value = m_defaultValues.value(key);
    if(key == Qmmp::YEAR || key == Qmmp::DISCNUMBER)
    {
        m_ui->tagSpinBox->setVisible(true);
        m_ui->tagSpinBox->setValue(value.toInt());
        m_ui->tagValueLineEdit->setVisible(false);
    }
    else
    {
        m_ui->tagSpinBox->setVisible(false);
        m_ui->tagValueLineEdit->setVisible(true);
        m_ui->tagValueLineEdit->setText(m_defaultValues.value(key));
    }
}
