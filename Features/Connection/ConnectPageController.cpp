#include "ConnectPageController.h"
#include "ConnectPage.h"
#include "../Chat/ChatWindow.h"
#include "../Chat/ServerChatWindow.h"
#include "../Chat/ChatController.h"
#include "../../Core/WebSocketClient.h"
#include "../../Core/WebSocketServer.h"
#include "../../Core/DatabaseManager.h"

#include <QDebug>
#include <QMessageBox>
#include <QUrl>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QDir>

ConnectPageController::ConnectPageController(ConnectPage *connectPage, ChatWindow *chatWindow, ServerChatWindow *serverChatWindow, QObject *parent)
    : QObject(parent)
    , m_connectPage(connectPage)
    , m_chatWindow(chatWindow)
    , m_serverChatWindow(serverChatWindow)
    , m_chatController(new ChatController())
    , m_client(nullptr)
    , m_server(nullptr)
    , m_clientDb(nullptr)
    , m_serverDb(nullptr)
{
    // These connections are correct
    connect(m_connectPage, &ConnectPage::connectClient,
            this, &ConnectPageController::onConnectClient);
    connect(m_connectPage, &ConnectPage::createServer,
            this, &ConnectPageController::onCreateServer);
}

ConnectPageController::~ConnectPageController()
{
    delete m_chatController;
    delete m_client;
    delete m_server;
    delete m_clientDb;
    delete m_serverDb;
}

void ConnectPageController::onConnectClient(const QString &ipAddress)
{
    qDebug() << "Controller: Client connection requested to" << ipAddress;
    
    // Validate input
    QString input = ipAddress.trimmed();
    if (input.isEmpty()) {
        qDebug() << "No host/port provided";
        QMessageBox::warning(m_connectPage, "Invalid Input", "Please enter a port number (e.g., 8000) or IP address.");
        return;
    }
    
    QString urlString;
    
    // Check if input is just a port number (digits only)
    QRegularExpression digitsOnly("^\\d+$");
    if (digitsOnly.match(input).hasMatch()) {
        // User entered just a port number, connect to localhost
        qDebug() << "Port number detected:" << input << "- connecting to localhost:" << input;
        urlString = QString("ws://localhost:%1").arg(input);
    } else {
        // User entered an IP or hostname, use port 8000
        qDebug() << "IP/hostname detected:" << input << "- connecting on port 8000";
        urlString = QString("ws://%1:8000").arg(input);
    }
    
    m_client = new WebSocketClient(this);
    m_serverUrl = urlString; // Store for later use

    // These connections are correct
    connect(m_client, &WebSocketClient::connected,
            this, &ConnectPageController::onConnectionSuccess);
    connect(m_client, &WebSocketClient::connectionFailed,
            this, &ConnectPageController::onConnectionFailed);

    // Connect to the server
    m_client->connectToServer(QUrl(urlString));
}

void ConnectPageController::onCreateServer()
{
    qDebug() << "Controller: Server creation requested";
    m_server = new WebSocketServer(8000, this);
    
    // Create server database
    QString dbPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/server_chat.db";
    QDir().mkpath(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation));
    
    m_serverDb = new DatabaseManager(dbPath, this);
    if (!m_serverDb->initDatabase()) {
        qWarning() << "Failed to initialize server database!";
    } else {
        qDebug() << "Server database initialized at:" << dbPath;
    }
    
    // Initialize chat controller with server (uses ServerChatWindow)
    m_chatController->initServer(m_serverChatWindow, m_server, m_serverDb);
    
    // Update server info panel
    m_serverChatWindow->updateServerInfo("127.0.0.1", 8000, "متصل");
    
    m_connectPage->hide();
    m_serverChatWindow->show();
}

void ConnectPageController::onConnectionSuccess()
{
    qDebug() << "Controller: Connection successful.";
    
    // Create client database
    QString dbPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/client_chat.db";
    QDir().mkpath(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation));
    
    m_clientDb = new DatabaseManager(dbPath, this);
    if (!m_clientDb->initDatabase()) {
        qWarning() << "Failed to initialize client database!";
    } else {
        qDebug() << "Client database initialized at:" << dbPath;
    }
    
    // Initialize chat controller with client (uses regular ChatWindow)
    m_chatController->initClient(m_chatWindow, m_client, m_clientDb);
    
    // Update client connection info
    m_chatWindow->updateConnectionInfo(m_serverUrl, "متصل");
    
    m_connectPage->hide();
    m_chatWindow->show();
}

void ConnectPageController::onConnectionFailed(const QString &error)
{
    qDebug() << "Controller: Connection failed:" << error;

    QMessageBox::critical(m_connectPage, "Connection Error", error);

    delete m_client;
    m_client = nullptr;
    delete m_server;
    m_server = nullptr;
}
