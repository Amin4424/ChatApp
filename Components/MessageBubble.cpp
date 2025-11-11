#include "MessageBubble.h"
#include <QVBoxLayout>

MessageBubble::MessageBubble(QWidget *parent)
    : QFrame(parent),
      m_messageType(Received),
      m_bubbleBorderRadius(10),
      m_bubblePadding(10)
{
    setupUI();
    applyDefaultStyles();
    updateStyles();
}

void MessageBubble::setupUI()
{
    // Set object name for styling
    setObjectName("messageBubbleFrame");
    
    // Create internal widgets
    m_senderLabel = new QLabel(this);
    m_senderLabel->setObjectName("senderLabel");
    m_messageLabel = new QLabel(this);
    m_messageLabel->setObjectName("messageLabel");
    
    // Configure sender label
    m_senderLabel->setWordWrap(false);
    m_senderFont = m_senderLabel->font();
    m_senderFont.setBold(true);
    m_senderFont.setPointSize(9);
    m_senderLabel->setFont(m_senderFont);
    
    // Configure message label
    m_messageLabel->setWordWrap(true);
    m_messageLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    m_messageFont = m_messageLabel->font();
    m_messageFont.setPointSize(10);
    m_messageLabel->setFont(m_messageFont);
    
    // Create layout
    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setContentsMargins(m_bubblePadding, 8, m_bubblePadding, 8);
    m_mainLayout->setSpacing(4);
    m_mainLayout->addWidget(m_senderLabel);
    m_mainLayout->addWidget(m_messageLabel);
    
    // Configure the bubble frame
    setFrameShape(QFrame::NoFrame);
    setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Preferred);
    setMaximumWidth(450);
}

MessageBubble::~MessageBubble()
{
    // QObject parent-child relationship handles cleanup
}

MessageBubble* MessageBubble::setSenderText(const QString &sender)
{
    m_senderLabel->setText(sender);
    m_senderLabel->setVisible(!sender.isEmpty());
    return this;
}

MessageBubble* MessageBubble::setMessageText(const QString &message)
{
    m_messageLabel->setText(message);
    
    // Detect text direction and set alignment
    if (isRTL(message)) {
        m_messageLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    } else {
        m_messageLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    }
    
    return this;
}

MessageBubble* MessageBubble::setMessageType(MessageType type)
{
    m_messageType = type;
    applyDefaultStyles();
    updateStyles();
    return this;
}

// Style setters
MessageBubble* MessageBubble::setBubbleBackgroundColor(const QColor &color)
{
    m_bubbleBgColor = color;
    updateStyles();
    return this;
}

MessageBubble* MessageBubble::setBubbleBorderColor(const QColor &color)
{
    m_bubbleBorderColor = color;
    updateStyles();
    return this;
}

MessageBubble* MessageBubble::setBubbleBorderRadius(int radius)
{
    m_bubbleBorderRadius = radius;
    updateStyles();
    return this;
}

MessageBubble* MessageBubble::setBubblePadding(int padding)
{
    m_bubblePadding = padding;
    if (layout()) {
        layout()->setContentsMargins(padding, 8, padding, 8);
    }
    return this;
}

MessageBubble* MessageBubble::setSenderTextColor(const QColor &color)
{
    m_senderTextColor = color;
    updateStyles();
    return this;
}

MessageBubble* MessageBubble::setSenderFont(const QFont &font)
{
    m_senderFont = font;
    m_senderLabel->setFont(font);
    return this;
}

MessageBubble* MessageBubble::setMessageTextColor(const QColor &color)
{
    m_messageTextColor = color;
    updateStyles();
    return this;
}

MessageBubble* MessageBubble::setMessageFont(const QFont &font)
{
    m_messageFont = font;
    m_messageLabel->setFont(font);
    return this;
}

// Direct stylesheet setters
MessageBubble* MessageBubble::setBubbleStyleSheet(const QString &styleSheet)
{
    this->setStyleSheet(styleSheet);
    return this;
}

MessageBubble* MessageBubble::setSenderLabelStyleSheet(const QString &styleSheet)
{
    m_senderLabel->setStyleSheet(styleSheet);
    return this;
}

MessageBubble* MessageBubble::setMessageLabelStyleSheet(const QString &styleSheet)
{
    m_messageLabel->setStyleSheet(styleSheet);
    return this;
}

void MessageBubble::applyDefaultStyles()
{
    // Set default colors based on message type
    switch (m_messageType) {
    case Sent:
        m_bubbleBgColor = QColor("#a0e97e");
        m_bubbleBorderColor = QColor("#8dd06a");
        m_senderTextColor = QColor("#666666");
        m_messageTextColor = QColor("#000000");
        break;
        
    case Received:
        m_bubbleBgColor = QColor("#ffffff");
        m_bubbleBorderColor = QColor("#e0e0e0");
        m_senderTextColor = QColor("#075e54");
        m_messageTextColor = QColor("#000000");
        break;
    }
}

void MessageBubble::updateStyles()
{
    // Build bubble stylesheet with object name selector
    QString bubbleStyle = QString(
        "QFrame#messageBubbleFrame {"
        "    background-color: %1;"
        "    border: 1px solid %2;"
        "    border-radius: %3px;"
        "}"
    ).arg(m_bubbleBgColor.name())
     .arg(m_bubbleBorderColor.name())
     .arg(m_bubbleBorderRadius);
    
    this->setStyleSheet(bubbleStyle);
    
    // Apply sender label style
    QString senderStyle = QString(
        "color: %1; font-family: %2; font-size: %3pt; background-color: transparent;"
    ).arg(m_senderTextColor.name(), m_senderFont.family(), QString::number(m_senderFont.pointSize()));
    if (m_senderFont.bold()) senderStyle += " font-weight: bold;";
    m_senderLabel->setStyleSheet(senderStyle);
    
    // Apply message label style
    QString messageStyle = QString(
        "color: %1; font-family: %2; font-size: %3pt; background-color: transparent;"
    ).arg(m_messageTextColor.name(), m_messageFont.family(), QString::number(m_messageFont.pointSize()));
    if (m_messageFont.bold()) messageStyle += " font-weight: bold;";
    m_messageLabel->setStyleSheet(messageStyle);
}

bool MessageBubble::isRTL(const QString &text) const
{
    if (text.isEmpty()) {
        return false;
    }
    
    // Check first character for RTL script
    // Persian/Arabic range: 0x0600-0x06FF, 0x0750-0x077F, 0xFB50-0xFDFF, 0xFE70-0xFEFF
    for (const QChar &ch : text) {
        ushort unicode = ch.unicode();
        if ((unicode >= 0x0600 && unicode <= 0x06FF) ||  // Arabic
            (unicode >= 0x0750 && unicode <= 0x077F) ||  // Arabic Supplement
            (unicode >= 0xFB50 && unicode <= 0xFDFF) ||  // Arabic Presentation Forms-A
            (unicode >= 0xFE70 && unicode <= 0xFEFF)) {  // Arabic Presentation Forms-B
            return true;
        }
        // If we hit a letter/digit, use it to determine direction
        if (ch.isLetterOrNumber()) {
            break;
        }
    }
    
    return false;
}
