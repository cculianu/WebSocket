#include "myserver.h"
#include <QCoreApplication>
#include <cstdlib>

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    MyServer server;
    if (!server.listen(QHostAddress::Any, 9999)) {
        qDebug() << "unable to bind to port 9999";
        return EXIT_FAILURE;
    }

    qDebug() << "listening on port: 9999";

    return a.exec();
}
