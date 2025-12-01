#include "UserListManager.h"
#include "UserCard.h"
#include <QPainter>
#include <QPixmap>

UserListManager::UserListManager(QListWidget *listWidget, QLabel *countLabel, QStackedWidget *stackedWidget, QObject *parent)
    : QObject(parent)
    , m_listWidget(listWidget)
    , m_countLabel(countLabel)
    , m_stackedWidget(stackedWidget)
{
    if (m_listWidget) {
        connect(m_listWidget, &QListWidget::itemClicked, this, &UserListManager::onItemClicked);
    }
}

void UserListManager::updateUsers(const QStringList &users)
{
    if (!m_listWidget) return;

    m_listWidget->clear();

    // Update count label
    if (m_countLabel) {
        m_countLabel->setText(QString("%1 User").arg(users.size()));
    }

    // Handle Empty State
    if (users.isEmpty()) {
        if (m_stackedWidget) {
            m_stackedWidget->setCurrentIndex(1); // Show Empty State
        }
        return;
    } else {
        if (m_stackedWidget) {
            m_stackedWidget->setCurrentIndex(0); // Show List
        }
    }

    for (const QString &user : users) {
        QListWidgetItem *item = new QListWidgetItem(m_listWidget);
        UserCard *card = new UserCard(m_listWidget);
        card->setName(user);
        card->setMessage("Click to open chat"); // Placeholder
        card->setTime(""); // Placeholder
        card->setLayoutDirection(Qt::LeftToRight);
        
        // Generate avatar from name initials
        QPixmap userAvatar(48, 48);
        // Generate a random-ish color based on name hash
        int hash = qHash(user);
        QColor bg = QColor::fromHsl(qAbs(hash) % 360, 200, 150); 
        userAvatar.fill(bg);
        QPainter p2(&userAvatar);
        p2.setPen(Qt::white);
        
        QFont f = p2.font();
        f.setPixelSize(24);
        p2.setFont(f);
        QString initials = user.left(1).toUpper();
        if (user.contains(" ")) {
            initials += user.split(" ").last().left(1).toUpper();
        }
        p2.drawText(userAvatar.rect(), Qt::AlignCenter, initials);
        card->setAvatar(userAvatar);
        
        // Online status (assume online if in list)
        card->setOnlineStatus(true);

        // Set size hint explicitly with some padding to prevent overlap
        item->setSizeHint(QSize(244, 112)); 
        m_listWidget->setItemWidget(item, card);
        item->setData(Qt::UserRole, user);

        connect(card, &UserCard::clicked, this, [this, item]() {
            m_listWidget->setCurrentItem(item);
            onItemClicked(item);
        });
    }
}

void UserListManager::updateSelection(const QString &currentUser, bool isPrivateChat)
{
    if (!m_listWidget) return;

    for (int i = 0; i < m_listWidget->count(); ++i) {
        QListWidgetItem *item = m_listWidget->item(i);
        QWidget *widget = m_listWidget->itemWidget(item);
        if (UserCard *card = qobject_cast<UserCard*>(widget)) {
            QString userId = item->data(Qt::UserRole).toString();
            bool isSelected = false;
            
            if (isPrivateChat) {
                isSelected = (userId == currentUser);
            } else {
                isSelected = (userId == "All Users");
            }
            
            card->setSelected(isSelected);
        }
    }
}

void UserListManager::updateUserCount(int count)
{
    if (m_countLabel) {
        m_countLabel->setText(QString("%1 User").arg(count));
    }
}

void UserListManager::onItemClicked(QListWidgetItem *item)
{
    if (item) {
        // Try to get ID from UserRole first (set by UserCard logic)
        QString userId = item->data(Qt::UserRole).toString();
        
        // Fallback to text if UserRole is empty (legacy support)
        if (userId.isEmpty()) {
            userId = item->text();
        }

        // Check if it's the separator
        if (userId.contains("──────")) {
            // Do nothing for separator
            return;
        }
        
        // It's a specific user
        // Remove the icon prefix if present
        QString cleanUserId = userId;
        if (cleanUserId.startsWith("👤 ")) {
            cleanUserId = cleanUserId.mid(2); // Remove "👤 "
        }
        
        emit userSelected(cleanUserId);
        
        // Clear unread count for the selected user
        QWidget *widget = m_listWidget->itemWidget(item);
        if (UserCard *card = qobject_cast<UserCard*>(widget)) {
            card->setUnreadCount(0);
        }
    }
}
