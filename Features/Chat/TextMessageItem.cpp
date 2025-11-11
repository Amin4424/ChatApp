#include "TextMessageItem.h"
#include <QHBoxLayout>

TextMessageItem::TextMessageItem(const QString &messageText, const QString &senderInfo, MessageType type, QWidget *parent)
    : BaseMessageItem(senderInfo, parent)
    , m_bubble(nullptr)
    , m_messageText(messageText)
    , m_messageType(type)
{
    setupUI();
    applyStyles();
}

TextMessageItem::~TextMessageItem()
{
    // QObject parent-child relationship handles cleanup
}

void TextMessageItem::setupUI()
{
    // 1. Remove previous layout if exists
    delete layout();

    // 2. Create main layout for alignment
    QHBoxLayout *mainLayout = new QHBoxLayout(this);
    mainLayout->setContentsMargins(5, 5, 5, 5);
    mainLayout->setSpacing(0);

    // 3. Create and configure MessageBubble component
    m_bubble = new MessageBubble(this);
    m_bubble->setSenderText(m_senderInfo);
    m_bubble->setMessageText(m_messageText);
    m_bubble->setMessageType(static_cast<::MessageType>(m_messageType)); // Cast to global MessageType

    // 4. Apply alignment (right/left)
    if (m_messageType == MessageType::Sent) {
        // Sent message: bubble on the right (in RTL layout)
        mainLayout->addWidget(m_bubble);
        mainLayout->addStretch(1); // Add space on the left
    } else {
        // Received message: bubble on the left (in RTL layout)
        mainLayout->addStretch(1); // Add space on the right
        mainLayout->addWidget(m_bubble);
    }
}

void TextMessageItem::applyStyles()
{
    if (!m_bubble) {
        return;
    }

    // Apply styles based on message type
    if (m_messageType == MessageType::Sent) {
        // Sent message styling (green bubble, WhatsApp style)
        m_bubble->setBubbleBackgroundColor(QColor("#dcf8c6"))
               ->setBubbleBorderColor(QColor("#b5e48c"))
               ->setBubbleBorderRadius(12)
               ->setBubblePadding(10)
               ->setSenderTextColor(QColor("#666666"))
               ->setMessageTextColor(QColor("#000000"));
    } else {
        // Received message styling (white bubble)
        m_bubble->setBubbleBackgroundColor(QColor("#ffffff"))
               ->setBubbleBorderColor(QColor("#e0e0e0"))
               ->setBubbleBorderRadius(12)
               ->setBubblePadding(10)
               ->setSenderTextColor(QColor("#075e54"))
               ->setMessageTextColor(QColor("#000000"));
    }

    // Optional: Set custom fonts
    QFont senderFont = m_bubble->senderLabel()->font();
    senderFont.setBold(true);
    senderFont.setPointSize(9);
    m_bubble->setSenderFont(senderFont);

    QFont messageFont = m_bubble->messageLabel()->font();
    messageFont.setPointSize(10);
    m_bubble->setMessageFont(messageFont);
}
