#pragma once

#include <WebSocket.h>

#include <QTcpSocket>
#include <QDebug>


class Client : public QObject
{
    Q_OBJECT
public:
    explicit Client(QObject *parent = nullptr);
    ~Client();

    void init();

signals:
    void failed();

private slots:

    void handshakeSuccess();
    void handshakeFailed();
    void clientReadyRead();

private:
    WebSocket::Wrapper *p_wrapper = nullptr;
};
