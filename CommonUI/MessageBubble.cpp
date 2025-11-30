#include "MessageBubble.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSizePolicy>
#include <QPalette>
#include <QDebug>

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
    m_messageLabel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    m_timestampLabel = new QLabel(this);
    m_timestampLabel->setObjectName("timestampLabel");
    m_editedLabel = new QLabel(this);
    m_editedLabel->setObjectName("editedLabel");
    m_statusIconLabel = new QLabel(this);  // NEW: Create status icon label
    m_statusIconLabel->setObjectName("statusIconLabel");
    
    // Configure sender label
    m_senderLabel->setWordWrap(false);
    m_senderFont = m_senderLabel->font();
    m_senderFont.setBold(true);
    m_senderFont.setPointSize(9);
    m_senderLabel->setFont(m_senderFont);
    m_senderLabel->setTextFormat(Qt::PlainText);
    
    // Configure message label
    m_messageLabel->setWordWrap(true);
    m_messageLabel->setTextFormat(Qt::PlainText);  // Always render as plain text so markup doesn't disappear
    m_messageLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    m_messageFont = m_messageLabel->font();
    m_messageFont.setPointSize(10);
    m_messageLabel->setFont(m_messageFont);
    m_messageLabel->setMinimumHeight(m_messageLabel->fontMetrics().height() + 4);
    m_messageLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred); // Allow expansion
    // Ensure label wraps correctly by constraining its width
    m_messageLabel->setMaximumWidth(450 - (m_bubblePadding * 2));

    
    // Configure timestamp label
    m_timestampLabel->setWordWrap(false);
    QFont timestampFont = m_timestampLabel->font();
    timestampFont.setPointSize(7);
    timestampFont.setItalic(true);
    m_timestampLabel->setFont(timestampFont);
    m_timestampLabel->setStyleSheet("QLabel { color: #666666; font-size: 10px; padding: 0px; margin: 0px; }");
    m_timestampLabel->setVisible(false);  // Hidden by default until timestamp is set
    m_timestampLabel->setContentsMargins(0, 0, 0, 0);
    m_timestampLabel->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    
    // Configure edited label
    m_editedLabel->setWordWrap(false);
    QFont editedFont = timestampFont;
    editedFont.setItalic(true);
    m_editedLabel->setFont(editedFont);
    m_editedLabel->setStyleSheet("QLabel { background-color: transparent; color: #999999; font-size: 10px; font-style: italic; padding: 0px; margin: 0px; }");
    // m_editedLabel->setVisible(false); // <-- REMOVED: Don't hide initially, let setEdited control it
    m_editedLabel->setContentsMargins(0, 0, 0, 0);
    m_editedLabel->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    
    // Configure status icon label (NEW)
    m_statusIconLabel->setWordWrap(false);
    m_statusIconLabel->setStyleSheet("QLabel { color: #666; font-size: 12px; padding: 0px; margin: 0px; }");
    m_statusIconLabel->setVisible(false);  // Hidden by default until status is set
    m_statusIconLabel->setContentsMargins(0, 0, 0, 0);
    m_statusIconLabel->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    
    // Create main layout
    m_mainLayout = new QVBoxLayout(this);
    // Reduced top margin (was 6), Adjusted bottom margin (was 16) - relying on label padding
    m_mainLayout->setContentsMargins(m_bubblePadding, 4, m_bubblePadding, 8);
    m_mainLayout->setSpacing(4);
    m_mainLayout->addWidget(m_senderLabel);
    m_mainLayout->addWidget(m_messageLabel);
    
    // Create bottom layout for timestamp + status icon (NEW)
    QHBoxLayout *bottomLayout = new QHBoxLayout();
    bottomLayout->setContentsMargins(0, 0, 0, 0);
    bottomLayout->setSpacing(2);
    bottomLayout->addStretch(1);  // Spacer to push to the right
    bottomLayout->addWidget(m_editedLabel);
    bottomLayout->addWidget(m_timestampLabel);
    bottomLayout->addWidget(m_statusIconLabel);
    
    m_mainLayout->addLayout(bottomLayout);  // Add bottom layout to main layout
    
    // Configure the bubble frame
    setFrameShape(QFrame::NoFrame);
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    setMaximumWidth(450);
    // Minimum height based on content (message line + footer + padding)
    const int minH = m_messageLabel->fontMetrics().height()
                   + m_timestampLabel->fontMetrics().height()
                   + 16; // padding allowance
    setMinimumHeight(qMax(56, minH));
}

MessageBubble::~MessageBubble()
{
    // QObject parent-child relationship handles cleanup
}

MessageBubble* MessageBubble::setSenderText(const QString &sender)
{
    m_senderLabel->setText(sender);
    m_senderLabel->setVisible(!sender.isEmpty());
    if (!sender.isEmpty()) {
        m_senderLabel->adjustSize();
    }
    return this;
}

MessageBubble* MessageBubble::setMessageText(const QString &message)
{
    m_messageLabel->setText(message);
    m_messageLabel->setVisible(!message.isEmpty());
    m_messageLabel->adjustSize();
    
    // Detect text direction and set alignment
    if (isRTL(message)) {
        m_messageLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    } else {
        m_messageLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    }
    
    // Ensure the layout grows to fit the text
    if (layout()) {
        layout()->invalidate();
        layout()->activate();
    }
    adjustSize();
    updateGeometry();

    return this;
}

MessageBubble* MessageBubble::setMessageType(MessageType type)
{
    m_messageType = type;
    applyDefaultStyles();
    updateStyles();
    return this;
}

MessageBubble* MessageBubble::setTimestamp(const QString &timestamp)
{
    m_timestampLabel->setText(timestamp);
    m_timestampLabel->setVisible(!timestamp.isEmpty());
    return this;
}

MessageBubble* MessageBubble::setEdited(bool edited)
{
    m_isEdited = edited;
    if (m_editedLabel) {
        if (edited) {
            m_editedLabel->setText(tr("(Edited)"));
            m_editedLabel->show(); // Explicitly show
        } else {
            m_editedLabel->hide(); // Explicitly hide
        }
        
        // Force layout update when visibility changes
        if (layout()) {
            layout()->invalidate();
            layout()->activate();
        }
        
        // Ensure message label is also updated
        if (m_messageLabel) {
            m_messageLabel->adjustSize();
            m_messageLabel->updateGeometry();
        }

        adjustSize();
        updateGeometry();
        

    }
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
        // Reduced top margin (was 6), Adjusted bottom margin (was 16)
        layout()->setContentsMargins(padding, 4, padding, 8);
    }
    if (m_messageLabel) {
        m_messageLabel->setMaximumWidth(450 - (padding * 2));
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
    
    // Update message label minimum height for the new font
    m_messageLabel->setMinimumHeight(QFontMetrics(font).height() + 4);
    
    // Recalculate minimum height based on new font
    if (m_messageLabel && m_timestampLabel) {
        const int minH = m_messageLabel->fontMetrics().height()
                       + m_timestampLabel->fontMetrics().height()
                       + 16; // padding allowance
        setMinimumHeight(qMax(56, minH));
    }
    
    if (layout()) {
        layout()->invalidate();
        layout()->activate();
    }
    adjustSize();
    updateGeometry();
    
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

MessageBubble* MessageBubble::setEditedStyle(const QString &styleSheet)
{
    if (m_editedLabel) {
        m_editedLabel->setStyleSheet(styleSheet);
    }
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
    QPalette senderPalette = m_senderLabel->palette();
    senderPalette.setColor(QPalette::WindowText, m_senderTextColor);
    senderPalette.setColor(QPalette::Text, m_senderTextColor);
    m_senderLabel->setPalette(senderPalette);
    
    // Apply message label style
    // Added padding-bottom to prevent text clipping
    QString messageStyle = QString(
        "color: %1; font-family: %2; font-size: %3pt; background-color: transparent; padding-bottom: 8px;"
    ).arg(m_messageTextColor.name(), m_messageFont.family(), QString::number(m_messageFont.pointSize()));
    if (m_messageFont.bold()) messageStyle += " font-weight: bold;";
    m_messageLabel->setStyleSheet(messageStyle);
    QPalette messagePalette = m_messageLabel->palette();
    messagePalette.setColor(QPalette::WindowText, m_messageTextColor);
    messagePalette.setColor(QPalette::Text, m_messageTextColor);
    m_messageLabel->setPalette(messagePalette);

    QString timestampColor = (m_messageType == Sent) ? "#7f8ca6" : "#ffffff";
    QString fontWeight = (m_messageType == Sent) ? "normal" : "bold";
    m_timestampLabel->setStyleSheet(QString("QLabel { background-color: transparent; color: %1; font-size: 11px; padding: 0px; margin: 0px; font-weight: %2; }").arg(timestampColor, fontWeight));
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

MessageBubble* MessageBubble::setMessageStatus(Status status)
{
    QColor tickColor = (m_messageType == Sent) ? QColor("#1f6bff") : QColor("#ffffff");
    QColor pendingColor = (m_messageType == Sent) ? QColor("#7f8ca6") : QColor("#ffffff");
    switch (status) {
        case Status::Pending:
            m_statusIconLabel->setText("🕒");
            m_statusIconLabel->setStyleSheet(QString("QLabel { background-color: transparent; color: %1; font-size: 11px; padding: 0px; margin: 0px; }").arg(pendingColor.name()));
            m_statusIconLabel->setVisible(true);
            break;
        case Status::Sent:
            m_statusIconLabel->setText("✓");
            m_statusIconLabel->setStyleSheet(QString("QLabel { background-color: transparent; color: %1; font-weight: bold; font-size: 11px; padding: 0px; margin: 0px; }")
                                             .arg(tickColor.name()));
            m_statusIconLabel->setVisible(true);
            break;
        case Status::None:
        default:
            m_statusIconLabel->setVisible(false);
            break;
    }
    return this;
}
