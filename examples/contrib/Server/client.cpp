#include "client.h"
#include <WebSocket.h>

#include <QDateTime>
#include <QDebug>
#include <QTimer>

#include <memory>


Client::Client(QTcpSocket *sock, QObject *parent)
    : QObject(parent)
{
    p_wrapper = new WebSocket::Wrapper(sock /* ownership of socket passed to wrapper object */, this);
    connect(p_wrapper, &WebSocket::Wrapper::messagesReady, this, &Client::clientReadyRead);
    connect(p_wrapper, &WebSocket::Wrapper::handshakeFinished, this, &Client::handshakeSuccess, Qt::QueuedConnection);
    connect(p_wrapper, &WebSocket::Wrapper::handshakeFailed, this, &Client::handshakeFailed);
    connect(p_wrapper, &WebSocket::Wrapper::disconnected, this, &Client::disconnected);
    p_wrapper->startServerHandshake();
}

Client::~Client()
{
    qDebug() << "client deleted";
}

void Client::handshakeSuccess()
{
    qDebug() << "ws ok!";

    // send a message after 500 msec
    QTimer::singleShot(500, this, [this]{
        const QString msg = QString("hellow world from server (%1)").arg(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss"));
        p_wrapper->sendText(msg.toUtf8());
    });
}

void Client::handshakeFailed()
{
    qDebug() << "ws error!";
    deleteLater();
}

void Client::clientReadyRead()
{
    while (p_wrapper->messagesAvailable() > 0) {
        auto msg = p_wrapper->readNextMessage();
        qDebug() << "Msg received:" << QString::fromUtf8(msg);
    }
}
