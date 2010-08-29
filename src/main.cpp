#ifdef QTOPIA

#include <qtopiaapplication.h>
#include "melodiq.h"

QTOPIA_ADD_APPLICATION(QTOPIA_TARGET, MelodiqMainWindow)
QTOPIA_MAIN

#else // QTOPIA

#include <QtGui/QApplication>
#include "melodiq.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MelodiqMainWindow w;
    w.show();
    return a.exec();
}

#endif // QTOPIA
