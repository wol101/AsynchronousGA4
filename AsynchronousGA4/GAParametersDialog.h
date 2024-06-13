#ifndef GAPARAMETERSDIALOG_H
#define GAPARAMETERSDIALOG_H

#include <QDialog>

namespace Ui {
class GAParametersDialog;
}

class GAParametersDialog : public QDialog
{
    Q_OBJECT

public:
    explicit GAParametersDialog(QWidget *parent = nullptr);
    ~GAParametersDialog();

    QString editorText() const;
    void setEditorText(const QString &newEditorText);

private slots:
    void activateButtons();

protected:
    void closeEvent(QCloseEvent *event) Q_DECL_OVERRIDE;

private:
    Ui::GAParametersDialog *ui;

};

#endif // GAPARAMETERSDIALOG_H
