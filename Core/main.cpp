#include <QApplication>
#include "ConnectPage.h"
#include "ConnectPageController.h"
#include "ChatWindow.h"
#include "ServerChatWindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    
    // Create UI components
    ConnectPage *connectPage = new ConnectPage();
    ChatWindow *chatWindow = new ChatWindow();  // For clients
    ServerChatWindow *serverChatWindow = new ServerChatWindow();  // For server
    
    // Create controller that manages connection logic
    ConnectPageController *controller = new ConnectPageController(connectPage, chatWindow, serverChatWindow);
    
    // Show connect page initially
    connectPage->show();
    
    int result = app.exec();
    
    // Cleanup
    delete controller;
    delete connectPage;
    delete chatWindow;
    delete serverChatWindow;
    
    return result;
}
