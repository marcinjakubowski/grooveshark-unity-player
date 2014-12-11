#include "grooveshark.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    Grooveshark w;
    w.show();

    return a.exec();
}
