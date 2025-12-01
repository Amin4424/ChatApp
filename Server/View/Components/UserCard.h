#ifndef USERCARD_H
#define USERCARD_H

#include <QFrame>
#include <QLabel>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QPixmap>
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
#include <QEnterEvent>
#endif

class UserCard : public QFrame {
    Q_OBJECT

public:
    explicit UserCard(QWidget *parent = nullptr);

    // API Methods
    void setName(const QString &name);
    void setMessage(const QString &message);
    void setTime(const QString &time);
    void setAvatar(const QPixmap &avatar);
    void setUnreadCount(int count);
    void setOnlineStatus(bool isOnline);
    void setSelected(bool isSelected);
    
    // Extra method to handle "is typing..." state mentioned in visual analysis
    void setIsTyping(bool isTyping);
    
    // Extra method to show checkmark (read receipt) when unread count is 0
    // Pass true to show checkmark, false to hide
    void setReadReceipt(bool isRead);

signals:
    void clicked();

protected:
    void mousePressEvent(QMouseEvent *event) override;
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    void enterEvent(QEnterEvent *event) override;
#else
    void enterEvent(QEvent *event) override;
#endif
    void leaveEvent(QEvent *event) override;

private:
    void setupUI();
    void updateStyle();
    QPixmap getRoundedPixmap(const QPixmap &src, int radius);

    // UI Components
    QLabel *m_avatarLabel;
    QLabel *m_nameLabel;
    QLabel *m_timeLabel;
    QLabel *m_messageLabel;
    QLabel *m_unreadBadge;
    QLabel *m_readReceiptIcon; // For the checkmark
    QWidget *m_onlineIndicator;

    // State
    bool m_isSelected;
    bool m_isTyping;
    bool m_isHovered;
    int m_unreadCount;
    
    // Colors
    const QString COLOR_SELECTED_BG = "#3B82F6";     // Bright Blue
    const QString COLOR_UNSELECTED_BG = "#FFFFFF";   // White
    const QString COLOR_HOVER_BG = "#F3F4F6";        // Light Gray
    
    const QString COLOR_TEXT_SELECTED = "#FFFFFF";
    const QString COLOR_TEXT_NAME = "#000000";
    const QString COLOR_TEXT_PREVIEW = "#666666";
    const QString COLOR_TEXT_TIME = "#999999";
    const QString COLOR_TEXT_TYPING = "#3B82F6";
};

#endif // USERCARD_H
