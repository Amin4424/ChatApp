#include <QApplication>
#include "UI/ServerChatWindow.h"
#include "Network/WebSocketServer.h"
#include "Network/TusServer.h"
#include "Core/DatabaseManager.h"
#include "ServerController.h"
#include <QMessageBox>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    
    // Server-only setup
    ServerChatWindow *serverWindow = new ServerChatWindow();
    
    // Initialize server components
    WebSocketServer *wsServer = new WebSocketServer(8080);  // Pass port to constructor
    TusServer *tusServer = new TusServer();
    DatabaseManager *db = new DatabaseManager("server_chat.db");  // Pass database path
    
    // Initialize database
    if (!db->initDatabase()) {
        QMessageBox::critical(nullptr, "Database Error",
            "Failed to initialize database. Check write permissions.");
        return 1;
    }
    
    // Create server controller
    ServerController *controller = new ServerController(
        serverWindow,
        wsServer,
        db
    );
    
    // Start TUS server (WebSocketServer starts in constructor)
    if (!tusServer->start(1080)) {  // Use start() method
        QMessageBox::critical(nullptr, "Server Error",
            "Failed to start TUS server on port 1080");
        return 1;
    }
    
    // Show server window
    serverWindow->show();
    serverWindow->updateServerInfo("0.0.0.0", 8080, "Running");
    
    int result = app.exec();
    
    // Cleanup
    tusServer->stop();  // Use stop() method
    delete controller;
    delete db;
    delete tusServer;
    delete wsServer;
    delete serverWindow;
    
    return result;
}
