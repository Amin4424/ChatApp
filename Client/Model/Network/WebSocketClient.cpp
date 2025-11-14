#include "Model/Network/WebSocketClient.h"
#include <QDebug>
#include <QHostAddress>
#include "CryptoManager.h"
WebSocketClient::WebSocketClient(QObject *parent)
    : QObject(parent)
{

    connect(&m_webSocket, &QWebSocket::connected,
            this, &WebSocketClient::onConnected);

    connect(&m_webSocket, &QWebSocket::disconnected,
            this, &WebSocketClient::onDisconnected, Qt::UniqueConnection);


    connect(&m_webSocket, QOverload<QAbstractSocket::SocketError>::of(&QWebSocket::error),
            this, &WebSocketClient::onError, Qt::UniqueConnection);

    connect(&m_webSocket, &QWebSocket::textMessageReceived,
            this, &WebSocketClient::onMessageReceived);
}

WebSocketClient::~WebSocketClient()
{
    m_webSocket.close();
}

void WebSocketClient::onConnected()
{
    emit connected();
}

void WebSocketClient::connectToServer(const QUrl &url)
{
    if (!url.isValid()) {
        // Emit our string-based signal
        emit connectionFailed("Invalid URL: " + url.toString());
        return;
    }

    const QString host = url.host();
    if (!host.isEmpty() && host != QLatin1String("localhost")) {
        QHostAddress addr(host);
        if (addr.isNull()) {
            emit connectionFailed("Invalid host/address: " + host);
            return;
        }
    }

    m_url = url;
    m_webSocket.open(url);
}

void WebSocketClient::onMessageReceived(QString message)
{
    QString decryptedMessage = CryptoManager::decryptMessage(message);
    emit messageReceived(decryptedMessage);
}

void WebSocketClient::onDisconnected()
{
}

void WebSocketClient::onError(QAbstractSocket::SocketError error)
{
    QString errorString = m_webSocket.errorString();
    emit connectionFailed(errorString);
}

void WebSocketClient::sendMessage(const QString &message)
{
    QString encryptedMessage = CryptoManager::encryptMessage(message);
    if (m_webSocket.state() == QAbstractSocket::ConnectedState) {
        m_webSocket.sendTextMessage(encryptedMessage);
    } else {
    }
}
