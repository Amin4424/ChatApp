#include "MessageItem.h"
#include <QLabel>
#include <QHBoxLayout>

MessageItem::MessageItem(const QString &message, int messageId, QWidget *parent)
    : QWidget(parent)
    , m_fullMessage(message)
    , m_messageId(messageId)
{
    setupUI();
}

void MessageItem::setupUI()
{
    // Main layout
    QHBoxLayout *mainLayout = new QHBoxLayout(this);
    mainLayout->setContentsMargins(10, 5, 10, 5);
    
    // Message label
    m_messageLabel = new QLabel(m_fullMessage, this);
    m_messageLabel->setWordWrap(true);
    m_messageLabel->setStyleSheet(
        "QLabel {"
        "   background-color: white;"
        "   border-radius: 8px;"
        "   padding: 10px;"
        "   color: #333;"
        "}"
    );
    
    mainLayout->addWidget(m_messageLabel);
    
    setStyleSheet(
        "MessageItem {"
        "   background-color: transparent;"
        "}"
    );
}
