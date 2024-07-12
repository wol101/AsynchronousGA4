#ifndef TEXTEDITDIALOG_H
#define TEXTEDITDIALOG_H

#include <QDialog>

class BasicXMLSyntaxHighlighter;

namespace Ui {
class TextEditDialog;
}

class TextEditDialog : public QDialog
{
    Q_OBJECT

public:
    explicit TextEditDialog(QWidget *parent = 0);
    ~TextEditDialog();

    QString editorText() const;
    void setEditorText(const QString &editorText);
    void useXMLSyntaxHighlighter();

private slots:
    void find();
    void replace();

protected:
    void closeEvent(QCloseEvent *event) Q_DECL_OVERRIDE;

private:
    void setEditorFonts();

    Ui::TextEditDialog *ui;

    QString m_editorText;
    BasicXMLSyntaxHighlighter *m_basicXMLSyntaxHighlighter;
    QFont m_editorFont;
};

#endif // TEXTEDITDIALOG_H
