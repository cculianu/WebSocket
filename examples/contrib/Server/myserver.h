#pragma once
#include <QTcpServer>

class MyServer : public QTcpServer
{
public:
    explicit MyServer(QObject *parent = nullptr);
    ~MyServer();

private:
    void on_newConnection();
};
