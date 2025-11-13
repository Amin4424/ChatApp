#ifndef WEBSOCKETCLIENT_H
#define WEBSOCKETCLIENT_H

#include <QObject>
#include <QtWebSockets/QWebSocket>
#include <QUrl>
#include <QTimer>
class WebSocketClient : public QObject
{
    Q_OBJECT

public:
    explicit WebSocketClient(QObject *parent = nullptr);
    ~WebSocketClient();

    void sendMessage(const QString &message);

signals:
    void connected();
    void connectionFailed(const QString &errorMessage);
    void messageReceived(const QString &message);

private slots:

    void onConnected();
    void onMessageReceived(QString message);
    void onDisconnected();
    void onError(QAbstractSocket::SocketError error);

public slots:
    void connectToServer(const QUrl &url);

private:
    QWebSocket m_webSocket;
    QUrl m_url;
    QTimer *m_reconnectTimer;
};

#endif // WEBSOCKETCLIENT_H
