#include "MainWindow.h"

#include <QApplication>
#include <QStyle>
#include <QStyleFactory>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    a.setStyle(QStyleFactory::create("Fusion"));

#if defined(Q_OS_LINUX)
    // Fix Fusion disabled-text contrast on Linux
    if (QApplication::style()->objectName() == "fusion")
    {
        QPalette pal = a.palette();
        pal.setColor(QPalette::Disabled, QPalette::Text, QColor("#A0A0A0"));
        pal.setColor(QPalette::Disabled, QPalette::ButtonText, QColor("#A0A0A0"));
        pal.setColor(QPalette::Disabled, QPalette::WindowText, QColor("#A0A0A0"));
        a.setPalette(pal);
    }
#endif

    MainWindow w;
    w.show();
    return a.exec();
}
