#include "client.h"
#include <QDebug>
#include <QDateTime>

Client::Client(QObject *parent)
    : QObject(parent)
{
}

Client::~Client()
{
    qDebug() << "client deleted";
}

void Client::init()
{
    QTcpSocket *socket = new QTcpSocket(this);
    connect(socket, &QTcpSocket::connected, this, [this, socket]
    {
        p_wrapper = new WebSocket::Wrapper(socket, this);
        connect(p_wrapper, &WebSocket::Wrapper::messagesReady, this ,&Client::clientReadyRead);
        connect(p_wrapper, &WebSocket::Wrapper::handshakeFinished, this, &Client::handshakeSuccess);
        connect(p_wrapper, &WebSocket::Wrapper::handshakeFailed, this, &Client::handshakeFailed);
        connect(p_wrapper, &WebSocket::Wrapper::disconnected, this, &Client::failed);

        auto res = p_wrapper->startClientHandshake("/", "localhost.com");
        if (!res) p_wrapper->deleteLater();
    });
    connect(socket, &QTcpSocket::disconnected, this, &Client::failed);

    socket->connectToHost("127.0.0.1", 9999);
}

void Client::handshakeSuccess()
{
    qDebug() << "ws ok!";
    // send a message immediately
    const QString msg  = QString("hellow world from client (%1)").arg(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss"));
    p_wrapper->sendText(msg.toUtf8());
}

void Client::handshakeFailed()
{
    qDebug() << "ws error!";
    emit failed();
}

void Client::clientReadyRead()
{
    while (p_wrapper->messagesAvailable() > 0) {
        auto msg = p_wrapper->readNextMessage();
        qDebug() << "Msg received:" << QString::fromUtf8(msg);
    }
}
