#include "ConnectPageController.h"
#include "UI/ConnectPage.h"
#include "UI/ClientChatWindow.h"
#include "ClientController.h"
#include "Network/WebSocketClient.h"
#include "Core/DatabaseManager.h"

#include <QDebug>
#include <QMessageBox>

ConnectPageController::ConnectPageController(ConnectPage *connectPage, QObject *parent)
    : QObject(parent)
    , m_connectPage(connectPage)
    , m_chatWindow(nullptr)
    , m_clientController(nullptr)
    , m_client(nullptr)
    , m_db(nullptr)
{
    if (m_connectPage) {
        connect(m_connectPage, &ConnectPage::connectClient, this, &ConnectPageController::onConnectClient);
    }
}

ConnectPageController::~ConnectPageController()
{
    delete m_clientController;
    delete m_db;
    delete m_client;
    delete m_chatWindow;
}

void ConnectPageController::onConnectClient(const QString &ipAddress)
{
    qDebug() << "ConnectPageController: Connecting to server at" << ipAddress;
    
    m_serverUrl = ipAddress;
    
    // Parse input - could be IP, IP:Port, or just Port
    QString host;
    quint16 port = 8080; // Default port
    
    if (ipAddress.contains(':')) {
        // Format: IP:Port
        QStringList parts = ipAddress.split(':');
        host = parts[0];
        if (parts.size() > 1) {
            port = parts[1].toUShort();
        }
    } else {
        // Check if it's just a number (port only)
        bool isNumber;
        quint16 possiblePort = ipAddress.toUShort(&isNumber);
        if (isNumber && possiblePort > 0) {
            // It's a port, use localhost
            host = "localhost";
            port = possiblePort;
        } else {
            // It's an IP/hostname
            host = ipAddress;
        }
    }
    
    QString wsUrl = QString("ws://%1:%2").arg(host).arg(port);
    qDebug() << "ConnectPageController: Connecting to" << wsUrl;
    
    // Create client components
    m_chatWindow = new ClientChatWindow();
    m_client = new WebSocketClient();
    m_db = new DatabaseManager("client_chat.db");
    
    // Initialize database
    if (!m_db->initDatabase()) {
        qDebug() << "Failed to initialize client database";
    }
    
    // Create controller
    m_clientController = new ClientController(m_chatWindow, m_client, m_db);
    
    // Connect signals
    connect(m_client, &WebSocketClient::connected, this, &ConnectPageController::onConnectionSuccess);
    connect(m_client, &WebSocketClient::connectionFailed, this, &ConnectPageController::onConnectionFailed);
    
    // Attempt connection
    m_client->connectToServer(QUrl(wsUrl));
}

void ConnectPageController::onConnectionSuccess()
{
    qDebug() << "ConnectPageController: Connection successful";
    
    if (m_connectPage) {
        m_connectPage->hide();
    }
    
    if (m_chatWindow) {
        m_chatWindow->show();
        m_chatWindow->updateConnectionInfo(m_serverUrl, "Connected");
    }
}

void ConnectPageController::onConnectionFailed(const QString &error)
{
    qDebug() << "ConnectPageController: Connection failed:" << error;
    
    QMessageBox::critical(m_connectPage, "Connection Error", 
        QString("Failed to connect to server: %1").arg(error));
    
    // Cleanup
    delete m_clientController;
    m_clientController = nullptr;
    delete m_chatWindow;
    m_chatWindow = nullptr;
    delete m_client;
    m_client = nullptr;
    delete m_db;
    m_db = nullptr;
}
