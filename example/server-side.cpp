#include <QTcpServer>
#include "WebSocket.h"

// You must inherit QTcpServer and override incomingConnection(qintptr)
void MyTcpServer::incomingConnection(qintptr socketDescriptor)
{
    QTcpSocket *socket = new QTcpSocket(this);
    socket->setSocketDescriptor(socketDescriptor);
    // the below wrapper `ws` becomes parent of the socket,
    // and `this` is now parent of the wrapper.
    auto ws = new WebSocket::Wrapper(socket, this);

    // do not access `socket` below this line, use `ws` instead.

    connect(ws, &WebSocket::Wrapper::handshakeSuccess, this, [this, ws] {
        addPendingConnection(ws);
        // We must emit newConnection() here because we went asynch and are doing this 'some time later', and the calling code emitted a spurous newConnection() on our behalf previously.. and this is the *real* newConnection()
        emit newConnection();
    });
    // handle handshake failure as well
    connect(ws, &WebSocket::Wrapper::handshakeFailed, this, [ws](const QString &reason) {
        qWarning() << "WebSocket handshake failed:" <<  reason;
        ws->deleteLater();
    });

    auto res = ws->startServerHandshake();
    // ^ some time later either handshakeSuccess() or handshakeFailed() will be emitted by 'ws'

    if (!res)
        // this should not normally happen
        ws->deleteLater();
}
