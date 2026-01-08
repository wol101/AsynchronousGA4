#ifndef CLEARABLEPLAINTEXTEDIT_H
#define CLEARABLEPLAINTEXTEDIT_H

#include <QPlainTextEdit>
#include <QMenu>
#include <QAction>
#include <QStyle>

class ClearablePlainTextEdit : public QPlainTextEdit
{
    Q_OBJECT

public:
    explicit ClearablePlainTextEdit(QWidget *parent = nullptr)
        : QPlainTextEdit(parent)
    {
        clearAction = new QAction(this);
        QIcon clearIcon = style()->standardIcon(QStyle::SP_LineEditClearButton); // Use the platform's standard "clear" icon
        clearAction->setIcon(clearIcon);
        clearAction->setText("Clear");
        connect(clearAction, &QAction::triggered, this, &QPlainTextEdit::clear);
    }

protected:
    void contextMenuEvent(QContextMenuEvent *event) override
    {
        // Get the default context menu
        QMenu *menu = createStandardContextMenu();

        // Add a separator and our custom action
        menu->addSeparator();
        menu->addAction(clearAction);

        // Show the menu
        menu->exec(event->globalPos());

        delete menu; // Important: Qt docs specify caller owns the menu
    }

private:
    QAction *clearAction;
};

#endif // CLEARABLEPLAINTEXTEDIT_H
