#include "client.h"
#include "myserver.h"

#include <QDebug>

#include <memory>


MyServer::MyServer(QObject *parent)
    : QTcpServer(parent)
{
    connect(this, &MyServer::newConnection, this, &MyServer::on_newConnection);
}

MyServer::~MyServer()
{
    qDebug() << "MyServer deleted";
}

void MyServer::on_newConnection()
{
    while (hasPendingConnections()) {
        QTcpSocket *socket = nextPendingConnection();
        qDebug() << "new connection:" << socket->peerAddress().toString() << "port" << socket->peerPort();
        auto client = std::make_unique<Client>(socket, this);
        connect(client.get(), &Client::disconnected, client.get(), &QObject::deleteLater);
        client.release(); // release unique_ptr since this now "owns" it and manages it
    }
}
