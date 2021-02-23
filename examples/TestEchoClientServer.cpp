#include "../WebSocket.h"
#include <QFile>
#include <QHostAddress>
#include <QRegExp>
#include <QTcpServer>
#include <QSslCertificate>
#include <QSslConfiguration>
#include <QSslKey>

#include <QtCore>
#include <QtDebug>

#include <array>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <typeinfo>

namespace {
    using FramesList = std::list<WebSocket::Deser::Frame>;
    void printFrames(const FramesList & frames)
    {
        std::cout << "Parsed:\n";
        int i = 0;
        if (frames.empty())
            std::cout << "(Nothing parsed)\n";
        for (const auto & f : frames) {
            std::cout << "Frame: " << i++
                      << "  Type: " << WebSocket::frameTypeName(f.type).toUtf8().constData()
                      << "  masked: " << std::boolalpha << f.masked << std::noboolalpha
                      << "\n";
            //std::cout << "Hex:\n";
            //std::cout << f.payload.toHex().constData() << "\n";
            std::cout << "Decoded:\n";
            std::cout << f.payload.constData() << "\n";
        }
    }
}

// Usage:
//
// To run the echo server:
//
//      $ ./TestEchoClientServer /path/to/your_ssl_cert.crt /path/to/your_ssl_key.key 127.0.0.1 1234
//
//
// To run the echo client:
//
//      $ ./TestEchoClientServer 127.0.0.1 1234
//
//
// To run the self-test (for internal dev troubleshooting:
//
//      $ ./TestEchoClientServer
//

int main(int argc, char *argv[])
{
    QLoggingCategory::defaultCategory()->setEnabled(QtDebugMsg, true);
    if (argc < 3) {
        // stand alone test encoding / decoding
        std::cout << "Enter text:\n";
        std::array<char, 65536> linebuf;
        const QRegExp hexRE("^[0-9a-fA-F]+$");
        while ( std::cin.getline(linebuf.data(), linebuf.size()) ) {
            QByteArray b = QByteArray(linebuf.data()).trimmed();
            QByteArray frameData;

            try {
                if (hexRE.exactMatch(b)) {
                    frameData = QByteArray::fromHex(b);
                } else {
                    frameData = WebSocket::Ser::wrapText(b, true, 260);
                    std::cout << "Generated hex:\n" << frameData.toHex().constData() << "\n";
                }
                const auto frames = WebSocket::Deser::parseBuffer(frameData);
                printFrames(frames);
                std::cout << "Leftovers: [" << frameData.toHex().constData() << "]\n";
            } catch (const std::exception & e) {
                std::cout << "caught exception: " << e.what() << "\n";
                return 1;
            }
        }
        return 0;
    } else if (argc == 3) {
        // connect to SSL host test
        const QString host = argv[1];
        const quint16 port = quint16(QString(argv[2]).toUInt());
        QCoreApplication app(argc, argv);
        QSslSocket *ssock = new QSslSocket(&app); // will end up owned by the WebSocket::Wrapper
        QThread thr;
        QTimer::singleShot(10, &app, [&]{
            thr.start();
            QObject::connect(&app, &QCoreApplication::aboutToQuit, &app, [&]{
                thr.quit();
                thr.wait();
                qDebug("App quit");
            });
            QObject::connect(&thr, &QThread::finished, &app, []{
               qDebug("Thread finished");
            }, Qt::DirectConnection);
            std::cout << "Connecting to " << host.toUtf8().constData() << ":" << port << "\n";
            //QObject::connect(ssock, &QAbstractSocket::connected, &app, [&]{
            QObject::connect(ssock, &QSslSocket::encrypted, &app, [&app, ssock, &thr, &host]{
                WebSocket::Wrapper *sock = new WebSocket::Wrapper(ssock, &app);
                QObject::connect(sock, &QSslSocket::stateChanged, [](auto state){
                    qDebug("Wrapper state: %d", int(state));
                });
                QObject::connect(sock, &WebSocket::Wrapper::handshakeFailed, &app, [&](const QString &reason){
                    qDebug("Handshake failed: %s", reason.toUtf8().constData());
                    app.exit(1);
                });
                QObject::connect(sock, &WebSocket::Wrapper::handshakeSuccess, &app, [sock, &thr, &app]{
                    std::cout << "Handshake ok!\n";
                    QObject::connect(sock, &WebSocket::Wrapper::messagesReady, sock, [sock] {
                        const auto frames = sock->readAllMessages();
                        printFrames(frames);
                    });
                    QTimer *timer = new QTimer(nullptr);
                    const auto readLine = [sock, timer, &app]() mutable{
                        std::cout << "Enter text to send:\n";
                        std::array<char, 16384> linebuf;
                        if ( std::cin.getline(linebuf.data(), linebuf.size()) ) {
                            QTimer::singleShot(0, sock, [data = QByteArray(linebuf.data()).trimmed(), sock]() mutable {
                                if (data == "!")
                                    sock->disconnectFromHost(WebSocket::CloseCode::GoingAway, "Bye");
                                else if (data == "~")
                                    sock->sendPing("pingtest");
                                else
                                    sock->sendText(data);
                            });
                        } else {
                            if (timer) {
                                timer->stop();
                                timer->deleteLater();
                                timer = nullptr;
                            }
                            QTimer::singleShot(0, &app, [&app] {app.quit();});
                        }
                    };
                    QObject::connect(timer, &QTimer::timeout, timer, readLine);
                    timer->start(10);
                    timer->moveToThread(&thr);
                });
                QObject::connect(sock, &QAbstractSocket::disconnected, &app, [&app, sock]{
                    sock->deleteLater();
                    app.exit(2);
                });
                sock->startClientHandshake("/electrum", host, "bitcoin.com");
            });
            QObject::connect(ssock, &QSslSocket::stateChanged, [](auto state){
                qDebug("Socket state: %d", int(state));
            });
            QObject::connect(ssock, qOverload<const QList<QSslError> &>(&QSslSocket::sslErrors), [&](auto errs){
                for (const auto & err : errs) {
                    qDebug("SSL Error: %s", err.errorString().toUtf8().constData());
                }
                ssock->ignoreSslErrors();
            });
            //ssock->connectToHost(host, port);
            ssock->connectToHostEncrypted(host, port, host);
        });
        return app.exec();
    } else if (argc == 5) {
#ifdef Q_OS_DARWIN
        // workaround for annoying macos keychain access prompt. see: https://doc.qt.io/qt-5/qsslsocket.html#setLocalCertificate
        setenv("QT_SSL_USE_TEMPORARY_KEYCHAIN", "1", 1);
#endif
        // echo WSS server mode
        class EchoServer : public QTcpServer {
            QSslCertificate cert;
            QSslKey key;
        public:
            EchoServer(const QSslCertificate & cert, const QSslKey & key, QObject *parent=nullptr)
                : QTcpServer(parent), cert(cert), key(key) {}
            void incomingConnection(qintptr socketDescriptor) override {
                // this is taken from ServerSSL in Servers.cpp
                QSslSocket *socket = new QSslSocket(this);
                if (socket->setSocketDescriptor(socketDescriptor)) {
                    socket->setLocalCertificate(cert);
                    socket->setPrivateKey(key);
                    socket->setProtocol(QSsl::SslProtocol::AnyProtocol);
                    const auto peerName = QStringLiteral("%1:%2").arg(socket->peerAddress().toString()).arg(socket->peerPort());
                    if (socket->state() != QAbstractSocket::SocketState::ConnectedState || socket->isEncrypted()) {
                        qWarning() << peerName << " socket had unexpected state (must be both connected and unencrypted), deleting socket";
                        delete socket;
                        return;
                    }
                    QTimer *timer = new QTimer(socket);
                    timer->setObjectName(QStringLiteral("ssl handshake timer"));
                    timer->setSingleShot(true);
                    connect(timer, &QTimer::timeout, this, [socket, timer, peerName]{
                        qWarning() << peerName << " SSL handshake timed out after " << QString::number(timer->interval()/1e3, 'f', 1) << " secs, deleting socket";
                        socket->abort();
                        socket->deleteLater();
                    });
                    auto tmpConnections = std::make_shared<QList<QMetaObject::Connection>>();
                    *tmpConnections += connect(socket, &QSslSocket::disconnected, this, [socket, peerName]{
                        qDebug() << peerName << " SSL handshake failed due to disconnect before completion, deleting socket";
                        socket->deleteLater();
                    });
                    *tmpConnections += connect(socket, &QSslSocket::encrypted, this, [this, timer, tmpConnections, socket, peerName] {
                        timer->stop();
                        timer->deleteLater();
                        if (tmpConnections) {
                            // tmpConnections will get auto-deleted after this lambda returns because the QObject connection holding
                            // it alive will be disconnected.
                            for (const auto & conn : *tmpConnections)
                                disconnect(conn);
                        }
                        qDebug() << "Encrypted ok, wrapping with WebSocket and initiating handshake";
                        WebSocket::Wrapper *wsock = new WebSocket::Wrapper(socket, this);
                        connect(wsock, &QAbstractSocket::disconnected, this, [wsock]{
                            auto peerName = QString::asprintf("%s:%hu", wsock->peerAddress().toString().toUtf8().constData(), wsock->peerPort());
                            qDebug() << peerName << "disconnected, deleting";
                            wsock->deleteLater();
                        });
                        connect(wsock, &WebSocket::Wrapper::handshakeSuccess, this, [wsock, this]{
                            qDebug() << "Handshake ok, calling addPendingConnection";
                            addPendingConnection(wsock);
                            emit newConnection();
                        });
                        wsock->startServerHandshake();
                    });
                    *tmpConnections +=
                    connect(socket, qOverload<const QList<QSslError> &>(&QSslSocket::sslErrors), this, [socket, peerName](const QList<QSslError> & errors) {
                        for (const auto & e : errors)
                            qWarning() << peerName << " SSL error: " << e.errorString();
                        qDebug() << peerName << " Aborting connection due to SSL errors";
                        socket->deleteLater();
                    });
                    timer->start(10000); // give the handshake 10 seconds to complete
                    socket->startServerEncryption();
                } else {
                    qWarning() << "setSocketDescriptor returned false -- unable to initiate SSL for client: " << socket->errorString();
                    delete socket;
                }
            }
        };
        QFile certf(argv[1]), keyf(argv[2]);
        if (!certf.open(QIODevice::ReadOnly)) {
            qCritical() << "Failed to open certificate file:" << argv[1];
            return 1;
        }
        if (!keyf.open(QIODevice::ReadOnly)) {
            qCritical() << "Failed to open key file:" << argv[2];
            return 2;
        }
        QSslCertificate cert(&certf, QSsl::EncodingFormat::Pem);
        if (cert.isNull()) {
            qCritical() << "Failed to read certificate file:" << argv[1];
            return 3;
        }
        QSslKey key(&keyf, QSsl::KeyAlgorithm::Rsa, QSsl::EncodingFormat::Pem);
        if (key.isNull()) {
            qCritical() << "Failed to read key file:" << argv[2];
            return 4;
        }
        qDebug() << "Read cert and key ok";
        QHostAddress iface;
        iface.setAddress(QString(argv[3]));
        if (iface.isNull()) {
            qCritical() << "Bad interface" << argv[3];
            return 5;
        }
        quint16 port = QString(argv[4]).toUShort();
        if (port < 1024) {
            qCritical() << "Bad port" << argv[4];
            return 6;
        }
        QCoreApplication app(argc, argv);
        EchoServer srv(cert, key, &app);
        QTimer::singleShot(10, &srv, [&]{
            const QString hostPortStr = QString::asprintf("%s:%hu", iface.toString().toLatin1().constData(), port);
            if (!srv.listen(iface, port)) {
                qCritical() << "Failed to listen on" << hostPortStr;
                app.exit(1);
                return;
            }
            qDebug() << "Listening for connections on" << hostPortStr;
        });
        QObject::connect(&srv, &QTcpServer::newConnection, &app, [&]{
            QTcpSocket *tsock = srv.nextPendingConnection();
            if (!tsock) return;
            WebSocket::Wrapper *sock = dynamic_cast<WebSocket::Wrapper *>(tsock);
            if (!sock) {
                qWarning() << "Socket not a WebSocket::Wrapper! FIXME!";
                tsock->deleteLater();
                return;
            }
            const QString peerName = QString::asprintf("%s:%hu", sock->peerAddress().toString().toLatin1().constData(), sock->peerPort());
            qDebug() << "Got connection from" << peerName;
            auto closing = std::make_shared<bool>(false);
            auto extantPings = std::make_shared<QSet<QByteArray>>();
            QObject::connect(sock, &WebSocket::Wrapper::messagesReady, sock, [sock, extantPings]() mutable {
                const auto frames = sock->readAllMessages();
                for (const auto & f : frames) {
                    // data, echo back
                    if (f.type == WebSocket::FrameType::Text) {
                        qDebug("Got text frame [%s], echoing back", f.payload.constData());
                        sock->sendText(QByteArray("ECHO ") + f.payload);
                    } else {
                        qDebug("Got data frame [%d bytes], echoing back", f.payload.size());
                        sock->sendBinary(f.payload);
                    }
                }
            });
        });
        return app.exec();
    }
    //else ...
    std::cerr << "Unknowna args\n";
    return 1;
} // end function main
