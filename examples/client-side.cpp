#include "WebSocket.h"

void MyClass::example()
{
    // ... and somewhere in your code:

    // create the to-be-wrapped socket (this is the real socket)
    QTcpSocket *sock = new QTcpSocket(this);

    connect(sock, &QTcpSocket::connected, this, [this, sock]{
        // create the socket wrapper
        auto websock = new WebSocket::Wrapper(sock, this);
        // 'sock' is now reparented to 'websock', and 'websock' wraps 'sock'.
        // (do not use 'sock' directly anymore, instead use 'websock').

        // We will start the handshake process below:

        // register the success signal (emitted if the handshake succeeds)
        connect(websock, &WebSocket::Wrapper::handshakeSuccess, this, [this, websock]{
           // save the active socket here and use it...
           // At this point the conneciton is "speaking" the web socket
           // You can call websock->write(), to send
           // Use the `messagesReady()` or `readyReady()` signal to receive
           this->connectedSock = websock;
           // use it.... emit a signal here, etc...
        });
        // we need to also handle failures
        connect(websock, &WebSocket::Wrapper::handshakeFailed, this, [this, websock](const QString & reason){
            // handle failure, delete the socket, etc
            qWarning() << "WebSocket handshake failed:" << reason;
            // below will also delete child wrapped socket...
            websock->deleteLater();
        });
        auto res = websock->startClientHandshake("/", "somehost.com");
        // ^ some time later either handshakeSuccess() or handshakeFailed() will be emitted by 'websock'

        if (!res)
            // this should not normally happen, but it pays to be careful
            websock->deleteLater();
    });

    sock->connectToHost(someHost, somePort);

    // be sure to add code to handle connection failures here too (omitted from this example).
}
