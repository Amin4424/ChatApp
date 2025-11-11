/**
 * MessageBubble Component - Usage Examples
 * 
 * This file demonstrates various ways to use the MessageBubble component
 * with different styling approaches and configurations.
 */

#include "MessageBubble.h"
#include <QApplication>
#include <QWidget>
#include <QVBoxLayout>
#include <QScrollArea>
#include <QFont>
#include <QColor>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    // Create main window
    QWidget *mainWindow = new QWidget();
    mainWindow->setWindowTitle("MessageBubble Component Examples");
    mainWindow->setMinimumSize(600, 800);
    
    // Create scroll area for messages
    QScrollArea *scrollArea = new QScrollArea();
    scrollArea->setWidgetResizable(true);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    
    // Create widget to hold all messages
    QWidget *messagesWidget = new QWidget();
    QVBoxLayout *messagesLayout = new QVBoxLayout(messagesWidget);
    messagesLayout->setSpacing(15);
    messagesLayout->setContentsMargins(20, 20, 20, 20);
    messagesLayout->setAlignment(Qt::AlignTop);
    
    // ========================================
    // Example 1: Basic Received Message
    // ========================================
    {
        MessageBubble *bubble = new MessageBubble();
        bubble->setSenderText("John Doe")
              ->setMessageText("Hello! How are you doing today?")
              ->setMessageType(Received);
        
        QHBoxLayout *msgLayout = new QHBoxLayout();
        msgLayout->addWidget(bubble);
        msgLayout->addStretch();
        messagesLayout->addLayout(msgLayout);
    }
    
    // ========================================
    // Example 2: Basic Sent Message
    // ========================================
    {
        MessageBubble *bubble = new MessageBubble();
        bubble->setSenderText("Me")
              ->setMessageText("I'm doing great! Thanks for asking.")
              ->setMessageType(Sent);
        
        QHBoxLayout *msgLayout = new QHBoxLayout();
        msgLayout->addStretch();
        msgLayout->addWidget(bubble);
        messagesLayout->addLayout(msgLayout);
    }
    
    // ========================================
    // Example 3: Custom Colors (Programmatic)
    // ========================================
    {
        MessageBubble *bubble = new MessageBubble();
        bubble->setSenderText("Alice")
              ->setMessageText("This message has custom colors!")
              ->setMessageType(Received)
              ->setBubbleBackgroundColor(QColor("#e3f2fd"))
              ->setBubbleBorderColor(QColor("#2196f3"))
              ->setBubbleBorderRadius(15)
              ->setSenderTextColor(QColor("#1565c0"))
              ->setMessageTextColor(QColor("#0d47a1"));
        
        QHBoxLayout *msgLayout = new QHBoxLayout();
        msgLayout->addWidget(bubble);
        msgLayout->addStretch();
        messagesLayout->addLayout(msgLayout);
    }
    
    // ========================================
    // Example 4: Custom Fonts
    // ========================================
    {
        QFont senderFont("Arial", 11, QFont::Bold);
        QFont messageFont("Georgia", 10);
        
        MessageBubble *bubble = new MessageBubble();
        bubble->setSenderText("Bob")
              ->setMessageText("This message uses custom fonts!")
              ->setMessageType(Sent)
              ->setSenderFont(senderFont)
              ->setMessageFont(messageFont);
        
        QHBoxLayout *msgLayout = new QHBoxLayout();
        msgLayout->addStretch();
        msgLayout->addWidget(bubble);
        messagesLayout->addLayout(msgLayout);
    }
    
    // ========================================
    // Example 5: Gradient Background (Stylesheet)
    // ========================================
    {
        MessageBubble *bubble = new MessageBubble();
        bubble->setSenderText("Designer")
              ->setMessageText("Check out this beautiful gradient background!");
        
        bubble->setBubbleStyleSheet(
            "QFrame#messageBubbleFrame {"
            "    background: qlineargradient(x1:0, y1:0, x2:1, y2:1, "
            "                stop:0 #667eea, stop:1 #764ba2);"
            "    border: none;"
            "    border-radius: 18px;"
            "}"
        );
        
        bubble->setSenderLabelStyleSheet(
            "color: #ffffff; "
            "font-weight: bold; "
            "font-size: 10pt;"
        );
        
        bubble->setMessageLabelStyleSheet(
            "color: #ffffff; "
            "font-size: 11pt;"
        );
        
        QHBoxLayout *msgLayout = new QHBoxLayout();
        msgLayout->addWidget(bubble);
        msgLayout->addStretch();
        messagesLayout->addLayout(msgLayout);
    }
    
    // ========================================
    // Example 6: RTL Text (Persian/Arabic)
    // ========================================
    {
        MessageBubble *bubble = new MessageBubble();
        bubble->setSenderText("محمد")
              ->setMessageText("سلام! این یک پیام فارسی است که به صورت خودکار راست‌چین می‌شود.")
              ->setMessageType(Received);
        
        QHBoxLayout *msgLayout = new QHBoxLayout();
        msgLayout->addWidget(bubble);
        msgLayout->addStretch();
        messagesLayout->addLayout(msgLayout);
    }
    
    // ========================================
    // Example 7: Dark Theme
    // ========================================
    {
        MessageBubble *bubble = new MessageBubble();
        bubble->setSenderText("Night Owl")
              ->setMessageText("This is a dark themed message!")
              ->setMessageType(Sent)
              ->setBubbleBackgroundColor(QColor("#2c2c2e"))
              ->setBubbleBorderColor(QColor("#3a3a3c"))
              ->setSenderTextColor(QColor("#8e8e93"))
              ->setMessageTextColor(QColor("#ffffff"));
        
        QHBoxLayout *msgLayout = new QHBoxLayout();
        msgLayout->addStretch();
        msgLayout->addWidget(bubble);
        messagesLayout->addLayout(msgLayout);
    }
    
    // ========================================
    // Example 8: Long Message with Word Wrap
    // ========================================
    {
        MessageBubble *bubble = new MessageBubble();
        bubble->setSenderText("Author")
              ->setMessageText(
                  "This is a very long message that demonstrates the word wrapping "
                  "capability of the MessageBubble component. The text will automatically "
                  "wrap to multiple lines when it exceeds the maximum width. This ensures "
                  "that the message remains readable and properly formatted, regardless of "
                  "how long the content is."
              )
              ->setMessageType(Received)
              ->setMaximumWidth(400);
        
        QHBoxLayout *msgLayout = new QHBoxLayout();
        msgLayout->addWidget(bubble);
        msgLayout->addStretch();
        messagesLayout->addLayout(msgLayout);
    }
    
    // ========================================
    // Example 9: Direct Widget Access
    // ========================================
    {
        MessageBubble *bubble = new MessageBubble();
        bubble->setSenderText("Advanced User")
              ->setMessageText("Using direct widget access for custom styling!")
              ->setMessageType(Sent);
        
        // Access widgets directly for advanced customization
        bubble->senderLabel()->setStyleSheet("color: #ff5722; font-weight: bold; text-decoration: underline;");
        bubble->messageLabel()->setStyleSheet("color: #333333; font-style: italic;");
        bubble->mainLayout()->setSpacing(8);
        
        QHBoxLayout *msgLayout = new QHBoxLayout();
        msgLayout->addStretch();
        msgLayout->addWidget(bubble);
        messagesLayout->addLayout(msgLayout);
    }
    
    // ========================================
    // Example 10: System/Info Message
    // ========================================
    {
        MessageBubble *bubble = new MessageBubble();
        bubble->setSenderText("")  // No sender for system messages
              ->setMessageText("🔒 Messages are end-to-end encrypted. No one outside this chat can read them.");
        
        bubble->setBubbleStyleSheet(
            "QFrame#messageBubbleFrame {"
            "    background-color: #fff4e6;"
            "    border: 1px dashed #ff9800;"
            "    border-radius: 8px;"
            "}"
        );
        
        bubble->setMessageLabelStyleSheet(
            "color: #e65100; "
            "font-size: 9pt; "
            "font-style: italic;"
        );
        
        bubble->messageLabel()->setAlignment(Qt::AlignCenter);
        
        QHBoxLayout *msgLayout = new QHBoxLayout();
        msgLayout->addStretch();
        msgLayout->addWidget(bubble);
        msgLayout->addStretch();
        messagesLayout->addLayout(msgLayout);
    }
    
    // ========================================
    // Example 11: WhatsApp Style Messages
    // ========================================
    {
        // Received message
        MessageBubble *received = new MessageBubble();
        received->setSenderText("Contact")
                ->setMessageText("Are you free this evening?")
                ->setMessageType(Received)
                ->setBubbleBackgroundColor(QColor("#ffffff"))
                ->setBubbleBorderColor(QColor("#e0e0e0"))
                ->setSenderTextColor(QColor("#128c7e"))
                ->setMessageTextColor(QColor("#000000"));
        
        QHBoxLayout *receivedLayout = new QHBoxLayout();
        receivedLayout->addWidget(received);
        receivedLayout->addStretch();
        messagesLayout->addLayout(receivedLayout);
        
        // Sent message
        MessageBubble *sent = new MessageBubble();
        sent->setSenderText("")  // No sender name for sent messages
            ->setMessageText("Yes! What do you have in mind?")
            ->setMessageType(Sent)
            ->setBubbleBackgroundColor(QColor("#dcf8c6"))
            ->setBubbleBorderColor(QColor("#b5e48c"))
            ->setMessageTextColor(QColor("#000000"));
        
        QHBoxLayout *sentLayout = new QHBoxLayout();
        sentLayout->addStretch();
        sentLayout->addWidget(sent);
        messagesLayout->addLayout(sentLayout);
    }
    
    // ========================================
    // Example 12: Multi-line with Bullet Points
    // ========================================
    {
        MessageBubble *bubble = new MessageBubble();
        bubble->setSenderText("Team Lead")
              ->setMessageText(
                  "Meeting agenda:\n"
                  "• Review Q4 results\n"
                  "• Discuss new project\n"
                  "• Team building activities\n"
                  "• Q&A session"
              )
              ->setMessageType(Received)
              ->setBubblePadding(15);
        
        QHBoxLayout *msgLayout = new QHBoxLayout();
        msgLayout->addWidget(bubble);
        msgLayout->addStretch();
        messagesLayout->addLayout(msgLayout);
    }
    
    // Set up scroll area
    scrollArea->setWidget(messagesWidget);
    
    // Main layout
    QVBoxLayout *mainLayout = new QVBoxLayout(mainWindow);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->addWidget(scrollArea);
    
    mainWindow->show();
    
    return app.exec();
}
