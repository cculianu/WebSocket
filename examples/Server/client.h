#pragma once
#include <QTcpSocket>


namespace WebSocket { class Wrapper; }

/// Represents a client connected to MyServer
class Client : public QObject
{
    Q_OBJECT
public:
    Client(QTcpSocket *socketToWrap, QObject *parent);
    ~Client();

signals:
    void disconnected();

private slots:
    void handshakeSuccess();
    void handshakeFailed();
    void clientReadyRead();

private:
    WebSocket::Wrapper * p_wrapper = nullptr;
};
