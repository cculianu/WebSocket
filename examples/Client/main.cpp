#include "client.h"
#include <QCoreApplication>

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    Client client;
    client.init();
    QObject::connect(&client, &Client::failed, &a, [&a]{ a.exit(1); });

    return a.exec();
}
