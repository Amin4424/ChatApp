#include <QApplication>
#include "View/ConnectPage.h"
#include "ConnectPageController.h"
#include "View/ClientChatWindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    
    // Client-only setup
    ConnectPage *connectPage = new ConnectPage();
    
    // Create controller for client
    ConnectPageController *controller = new ConnectPageController(connectPage);
    
    // Show connect page
    connectPage->show();
    
    int result = app.exec();
    
    // Cleanup
    delete controller;
    delete connectPage;
    
    return result;
}
