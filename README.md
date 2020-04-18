# Web Socket

A lightweight RFC 6455 (Web Socket) implementation for Qt5 by Calin Culianu <calin.culianu@gmail.com>

Key highlights:
  - Supports both `ws://` and `wss://` Web Sockets.
  - Support both client-side and server-side mode of operation.
  - Easy to integrate: just copy `WebSocket.h` and `WebSocket.cpp` into your project
  - Easy to use with existing codebases:
    - Unlike the `QWebSocket` module from Qt, this code's main class, `WebSocket::Wrapper` inherits from `QTcpSocket` and thus can easily be integrated into existing code.
    - In other words, the key paradigm offered is basically a generic wrapper for a regular `QTcpSocket` that itself inherits from `QTcpSocket`.
  - Asynchronous mode of operation (requires an event loop in the thread the `WebSocket::Wrapper` lives in).
  - Requires C++17.

### How to use in your project

1. Copy `WebSocket.h` and `WebSocket.cpp` into your project.
2. Enjoy!  (The license here is MIT so you can use this in any project, commercial or open source).

### Quick Example (Client Side)
Note that this example is to illustrate how to use `WebSocket::Wrapper`, and some key things have been omitted (such as handling all possible failures and/or cleaning up resources on all possible failure modes, etc). [Syntax highlighted version here](example/client-side.cpp).

```
#include "WebSocket.h"

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
```

### Quick Example (Server Side)
Note that this example is to illustrate how to use `WebSocket::Wrapper`, and some key things have been omitted (such as handling all possible failures and/or cleaning up resources on all possible failure modes, etc). [Syntax highlighted version here](example/server-side.cpp).

```
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
```

### TODO

 - Offer up some more concrete examples.cpp and perhaps a sample application.

License
----

MIT

### Donations
Yes please!  **Bitcoin** or **Bitcoin Cash** to this address please: **`1Ca1inCimwRhhcpFX84TPRrPQSryTgKW6N`**
