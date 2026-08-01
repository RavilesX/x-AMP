#ifndef BATCHTAGEDITORDIALOG_H
#define BATCHTAGEDITORDIALOG_H

#include <QDialog>
#include <QMap>
#include <qmmp/qmmp.h>

namespace Ui {
class BatchTagEditorDialog;
}

class BatchTagEditorDialog : public QDialog
{
    Q_OBJECT

public:
    explicit BatchTagEditorDialog(QWidget *parent = nullptr);
    ~BatchTagEditorDialog();

    void setFiles(const QStringList &paths);

public slots:
    void accept() override;

private slots:
    void on_tagComboBox_currentIndexChanged(int index);

private:
    Ui::BatchTagEditorDialog *m_ui;
    QMap<Qmmp::MetaData, QString> m_defaultValues;
    QStringList m_paths;
};

#endif // BATCHTAGEDITORDIALOG_H
