#include "Network/WebSocketClient.h"
#include <QDebug>
#include <QHostAddress>
#include "Core/CryptoManager.h"
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
    qDebug() << "Connected to" << m_webSocket.requestUrl();
    emit connected();
}

void WebSocketClient::connectToServer(const QUrl &url)
{
    if (!url.isValid()) {
        qWarning() << "Invalid URL passed to connectToServer():" << url;
        // Emit our string-based signal
        emit connectionFailed("Invalid URL: " + url.toString());
        return;
    }

    const QString host = url.host();
    if (!host.isEmpty() && host != QLatin1String("localhost")) {
        QHostAddress addr(host);
        if (addr.isNull()) {
            qWarning() << "Invalid host/address:" << host;
            emit connectionFailed("Invalid host/address: " + host);
            return;
        }
    }

    qDebug() << "Trying to connect to" << url;
    m_url = url;
    m_webSocket.open(url);
}

void WebSocketClient::onMessageReceived(QString message)
{
    QString decryptedMessage = CryptoManager::decryptMessage(message);
    qDebug() << "\n=== CLIENT RECEIVED MESSAGE ===";
    qDebug() << "Message from server:" << message;
    qDebug() << "Emitting messageReceived signal...";
    emit messageReceived(decryptedMessage);
    qDebug() << "=== END CLIENT RECEIVE ===\n";
}

void WebSocketClient::onDisconnected()
{
    qDebug() << "\n[DISCONNECTED]";
    qDebug() << "Connection closed.\n";
}

void WebSocketClient::onError(QAbstractSocket::SocketError error)
{
    QString errorString = m_webSocket.errorString();
    qDebug() << "\n[ERROR]";
    qDebug() << "WebSocket Error:" << errorString;
    qDebug() << "Error code:" << error << "\n";

    emit connectionFailed(errorString);
}

void WebSocketClient::sendMessage(const QString &message)
{
    QString encryptedMessage = CryptoManager::encryptMessage(message);
    if (m_webSocket.state() == QAbstractSocket::ConnectedState) {
        m_webSocket.sendTextMessage(encryptedMessage);
        qDebug() << "\n[SENT]:" << encryptedMessage;
    } else {
        qDebug() << "\n[ERROR] Cannot send message - not connected!";
    }
}
