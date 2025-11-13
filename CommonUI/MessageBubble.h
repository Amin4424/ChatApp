#ifndef MESSAGEBUBBLE_H
#define MESSAGEBUBBLE_H

#include <QFrame>
#include <QLabel>
#include <QVBoxLayout>
#include <QString>
#include <QColor>
#include <QFont>

// Forward declaration of MessageType from BaseChatWindow
enum MessageType {
    Received,
    Sent
};

class MessageBubble : public QFrame
{
    Q_OBJECT

public:
    explicit MessageBubble(QWidget *parent = nullptr);
    ~MessageBubble();

    // Fluent API setters for content
    MessageBubble* setSenderText(const QString &sender);
    MessageBubble* setMessageText(const QString &message);
    MessageBubble* setMessageType(MessageType type);
    MessageBubble* setTimestamp(const QString &timestamp);  // NEW: Add timestamp

    // Fluent API setters for styling
    MessageBubble* setBubbleBackgroundColor(const QColor &color);
    MessageBubble* setBubbleBorderColor(const QColor &color);
    MessageBubble* setBubbleBorderRadius(int radius);
    MessageBubble* setBubblePadding(int padding);
    MessageBubble* setSenderTextColor(const QColor &color);
    MessageBubble* setSenderFont(const QFont &font);
    MessageBubble* setMessageTextColor(const QColor &color);
    MessageBubble* setMessageFont(const QFont &font);

    // Direct stylesheet setters (like InfoCard pattern)
    MessageBubble* setBubbleStyleSheet(const QString &styleSheet);
    MessageBubble* setSenderLabelStyleSheet(const QString &styleSheet);
    MessageBubble* setMessageLabelStyleSheet(const QString &styleSheet);

    // Widget accessors for advanced customization
    QLabel* senderLabel() const { return m_senderLabel; }
    QLabel* messageLabel() const { return m_messageLabel; }
    QLabel* timestampLabel() const { return m_timestampLabel; }  // NEW: Add timestamp accessor
    QVBoxLayout* mainLayout() const { return m_mainLayout; }

private:
    // Setup UI
    void setupUI();
    
    // Internal widgets
    QLabel *m_senderLabel;
    QLabel *m_messageLabel;
    QLabel *m_timestampLabel;  // NEW: Add timestamp label
    QVBoxLayout *m_mainLayout;
    
    // Current message type for default styling
    MessageType m_messageType;

    // Style member variables
    QColor m_bubbleBgColor;
    QColor m_bubbleBorderColor;
    int m_bubbleBorderRadius;
    int m_bubblePadding;
    QColor m_senderTextColor;
    QFont m_senderFont;
    QColor m_messageTextColor;
    QFont m_messageFont;

    // Apply styles based on current style settings
    void updateStyles();
    
    // Set default styles based on message type
    void applyDefaultStyles();
    
    // Detect text direction (RTL/LTR)
    bool isRTL(const QString &text) const;
};

#endif // MESSAGEBUBBLE_H
