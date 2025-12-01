#ifndef TEXTMESSAGE_H
#define TEXTMESSAGE_H

#include "MessageComponent.h"
#include "MessageBubble.h"
#include <QString>
#include <QIcon>
#include <QStyle>

class QToolButton;
class QMenu;
class QAction;
class QEnterEvent;

class TextMessage : public MessageComponent
{
    Q_OBJECT

public:
    explicit TextMessage(const QString &messageText,
                         const QString &senderInfo = QString(),
                         MessageDirection direction = MessageDirection::Incoming,
                         const QString &timestamp = QString(),
                         QWidget *parent = nullptr);
    ~TextMessage();

    TextMessage &withMessage(const QString &messageText);
    TextMessage &withSender(const QString &senderInfo);
    TextMessage &withTimestamp(const QString &timestamp);
    TextMessage &withDirection(MessageDirection direction);
    
    // Public methods to control status
    void markAsSent();  // Change from pending (🕒) to sent (✓)
    QString messageText() const { return m_messageText; }
    void updateMessageText(const QString &newText);
    void markAsEdited(bool edited = true);
    bool isEdited() const { return m_isEdited; }
    void setDatabaseId(int id) override;
    int databaseId() const { return MessageComponent::databaseId(); }

signals:
    void copyRequested(const QString &text);
    void editRequested(TextMessage *item);
    void deleteRequested(TextMessage *item);

protected:
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    void enterEvent(QEnterEvent *event) override;
#else
    void enterEvent(QEvent *event) override;
#endif
    void leaveEvent(QEvent *event) override;

protected:
    void setupUI() override;
    void applyStyles();  // New method to apply styling to the MessageBubble

private:
    enum class MenuActionIcon {
        Copy,
        Edit,
        Delete
    };

    MessageBubble *m_bubble;
    QToolButton *m_moreButton;
    QMenu *m_actionMenu;
    QAction *m_copyAction;
    QAction *m_editAction;
    QAction *m_deleteAction;
    QString m_messageText;
    bool m_menuVisible = false;
    bool m_isEdited = false;

    void initializeActionButton();
    void updateActionStates();
    void showActionMenu();
    void applyActionMenuStyle();
    QIcon actionIcon(MenuActionIcon type) const;
};

#endif // TEXTMESSAGE_H
