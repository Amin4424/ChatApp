#include "UserCard.h"
#include <QPainter>
#include <QPainterPath>
#include <QMouseEvent>
#include <QGraphicsDropShadowEffect>
#include <QStyle>

UserCard::UserCard(QWidget *parent)
    : QFrame(parent)
    , m_isSelected(false)
    , m_isTyping(false)
    , m_isHovered(false)
    , m_unreadCount(0)
{
    setupUI();
    updateStyle();
}

void UserCard::setupUI()
{
    // Frame setup
    setFrameShape(QFrame::NoFrame);
    setFixedSize(224, 112);
    setCursor(Qt::PointingHandCursor);

    // Main Layout
    QHBoxLayout *mainLayout = new QHBoxLayout(this);
    mainLayout->setContentsMargins(0, 16, 16, 16); // Padding: 10px left, 16px top/right/bottom
    mainLayout->setSpacing(16); // Gap: 16px

    // --- Avatar Section ---
    QWidget *avatarContainer = new QWidget(this);
    avatarContainer->setObjectName("avatarContainer");
    avatarContainer->setFixedSize(40, 40);
    
    m_avatarLabel = new QLabel(avatarContainer);
    m_avatarLabel->setObjectName("avatarLabel");
    m_avatarLabel->setFixedSize(40, 40);
    m_avatarLabel->setScaledContents(true);
    
    // Online Indicator
    m_onlineIndicator = new QWidget(avatarContainer);
    m_onlineIndicator->setObjectName("onlineIndicator");
    m_onlineIndicator->setFixedSize(12, 12); // Size: 12x12
    // Position relative to avatar (bottom-right)
    // Avatar is 40x40. Indicator is 12x12.
    // Let's place it at x=28, y=28 to overlap slightly or x=30, y=30.
    // Figma says "top: 438px; left: 68px" vs Avatar "top: ...". 
    // Assuming standard bottom-right alignment.
    m_onlineIndicator->move(28, 28); 
    m_onlineIndicator->hide();

    mainLayout->addWidget(avatarContainer, 0, Qt::AlignTop);

    // --- Text Section ---
    QVBoxLayout *textLayout = new QVBoxLayout();
    textLayout->setContentsMargins(0, 0, 0, 0);
    textLayout->setSpacing(4); // Adjust spacing between rows

    // Top Row: Name + Time
    QHBoxLayout *topRow = new QHBoxLayout();
    topRow->setSpacing(8);
    
    m_nameLabel = new QLabel("User Name", this);
    m_nameLabel->setObjectName("nameLabel");
    m_nameLabel->setFixedHeight(19); // Height: 19
    QFont nameFont("Inter");
    nameFont.setPixelSize(16); // Size: 16px
    nameFont.setWeight(QFont::Normal); // Weight: 400
    m_nameLabel->setFont(nameFont);
    
    m_timeLabel = new QLabel("00:00", this);
    m_timeLabel->setObjectName("timeLabel");
    m_timeLabel->setFixedSize(37, 19); // Size: 37x19
    QFont timeFont("Vazirmatn");
    timeFont.setPixelSize(12); // Size: 12px
    timeFont.setWeight(QFont::Normal); // Weight: 400
    m_timeLabel->setFont(timeFont);
    m_timeLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

    topRow->addWidget(m_nameLabel, 1);
    topRow->addWidget(m_timeLabel);

    // Bottom Row: Message + Badge/Status
    QHBoxLayout *bottomRow = new QHBoxLayout();
    bottomRow->setSpacing(8);

    m_messageLabel = new QLabel("Message preview...", this);
    m_messageLabel->setObjectName("messageLabel");
    m_messageLabel->setFixedHeight(15); // Height: 15
    QFont msgFont("Inter");
    msgFont.setPixelSize(12); // Size: 12px
    msgFont.setWeight(QFont::Normal); // Weight: 400
    m_messageLabel->setFont(msgFont);
    m_messageLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    
    // Unread Badge
    m_unreadBadge = new QLabel(this);
    m_unreadBadge->setObjectName("unreadBadge");
    m_unreadBadge->setFixedSize(16, 16); // Size: 16x16
    m_unreadBadge->setAlignment(Qt::AlignCenter);
    QFont badgeFont = m_unreadBadge->font();
    badgeFont.setBold(true);
    badgeFont.setPixelSize(10);
    m_unreadBadge->setFont(badgeFont);
    m_unreadBadge->hide();

    // Read Receipt Icon
    m_readReceiptIcon = new QLabel(this);
    m_readReceiptIcon->setObjectName("readReceiptIcon");
    m_readReceiptIcon->setFixedSize(16, 16);
    m_readReceiptIcon->setAlignment(Qt::AlignCenter);
    m_readReceiptIcon->setText("✓✓"); 
    QFont iconFont = m_readReceiptIcon->font();
    iconFont.setPixelSize(10);
    iconFont.setBold(true);
    m_readReceiptIcon->setFont(iconFont);
    m_readReceiptIcon->hide();
    // textLayout->addSpacing(16);
    // bottomRow->addWidget(m_messageLabel, 1);
    // bottomRow->addWidget(m_unreadBadge);
    bottomRow->addWidget(m_readReceiptIcon);

    textLayout->addLayout(topRow);
    textLayout->addLayout(bottomRow);
    textLayout->addStretch(); // Push content up
    textLayout->addSpacing(16);
    QHBoxLayout *messageLayout = new QHBoxLayout();
    messageLayout->setContentsMargins(0, 0, 0, 0);
    messageLayout->setSpacing(0);
    messageLayout->addWidget(m_messageLabel, 1 , Qt::AlignLeft);
    messageLayout->addWidget(m_unreadBadge , 0 , Qt::AlignRight);
    textLayout->addLayout(messageLayout);

    mainLayout->addLayout(textLayout);

    // --- SET STYLESHEET ---
    this->setStyleSheet(
        "UserCard { "
            "background-color: #FFFFFF; "
            "border-radius: 12px; "
            "border: 1px solid #E0E0E0; "
        "}"
        "UserCard[selected='true'] { "
            "background: qlineargradient(x1:1, y1:0, x2:0, y2:0, stop:0 #79B6F7, stop:1 #318FF2); " /* 270deg gradient */
            "border: none;"
        "}"
        "UserCard[hovered='true'][selected='false'] { "
            "background-color: #F3F4F6; "
        "}"
        
        "QWidget#avatarContainer { background-color: transparent; border: none; }"
        "QLabel { background-color: transparent; border: none; }"
        
        "QLabel#avatarLabel { border: none; border-radius: 20px; }" /* Circular radius */
        
        "QWidget#onlineIndicator { "
            "background-color: #14B8A6; "
            "border: 2px solid #FFFFFF; "
            "border-radius: 6px; " /* Half of 12px */
        "}"

        "QLabel#nameLabel { color: #000000; }"
        "UserCard[selected='true'] QLabel#nameLabel { color: #FFFFFF; }"
        
        "QLabel#timeLabel { color: #C4C4C4; }" /* Using the color specified */
        "UserCard[selected='true'] QLabel#timeLabel { color: #FFFFFF; }"
        
        "QLabel#messageLabel { color: #929292; }" /* Default color not specified, assuming black/dark gray */
        "UserCard[selected='true'] QLabel#messageLabel { color: #FFFFFF; }"
        "UserCard[typing='true'] QLabel#messageLabel { color: #3B82F6; font-style: italic; }"
        
        "QLabel#unreadBadge { "
            "background-color: #0059FF; "
            "color: white; "
            "border-radius: 8px; " /* Half of 16px */
        "}"
        "UserCard[selected='true'] QLabel#unreadBadge { "
            "background-color: #FFFFFF; "
            "color: #318FF2; "
        "}"

        "UserCard[selected='true'][typing='true'] QLabel#messageLabel { color: #FFFFFF; }"

        "QLabel#readReceiptIcon { color: #3B82F6; }"
        "UserCard[selected='true'] QLabel#readReceiptIcon { color: white; }"
    );
}

void UserCard::setName(const QString &name)
{
    QString displayName = name;
    if (displayName.length() > 15) {
        displayName = displayName.left(15) + "...";
    }
    m_nameLabel->setText(displayName);
}

void UserCard::setMessage(const QString &message)
{
    QFontMetrics metrics(m_messageLabel->font());
    // Available width approx: 300 (card) - 24 (margins) - 40 (avatar) - 20 (spacing) - 30 (badge/time reserve) = ~180
    QString elidedText = metrics.elidedText(message, Qt::ElideLeft, 180); 
    m_messageLabel->setText(elidedText);
}

void UserCard::setTime(const QString &time)
{
    m_timeLabel->setText(time);
    m_timeLabel->setVisible(!time.isEmpty());
}

void UserCard::setAvatar(const QPixmap &avatar)
{
    if (avatar.isNull()) return;
    m_avatarLabel->setPixmap(getRoundedPixmap(avatar, 32)); // Radius = 32 for 64px size
}

void UserCard::setUnreadCount(int count)
{
    m_unreadCount = count;
    if (count > 0) {
        m_unreadBadge->setText(count > 99 ? "99+" : QString::number(count));
        m_unreadBadge->show();
        m_readReceiptIcon->hide();
    } else {
        m_unreadBadge->hide();
    }
    updateStyle();
}

void UserCard::setReadReceipt(bool isRead)
{
    if (m_unreadCount == 0) {
        m_readReceiptIcon->setVisible(isRead);
    }
}

void UserCard::setOnlineStatus(bool isOnline)
{
    m_onlineIndicator->setVisible(isOnline);
}

void UserCard::setSelected(bool isSelected)
{
    m_isSelected = isSelected;
    setProperty("selected", isSelected);
    style()->unpolish(this);
    style()->polish(this);
    
    // Force update children to ensure they pick up the new style from parent
    m_nameLabel->style()->unpolish(m_nameLabel);
    m_nameLabel->style()->polish(m_nameLabel);
    
    m_timeLabel->style()->unpolish(m_timeLabel);
    m_timeLabel->style()->polish(m_timeLabel);
    
    m_messageLabel->style()->unpolish(m_messageLabel);
    m_messageLabel->style()->polish(m_messageLabel);
    
    m_avatarLabel->style()->unpolish(m_avatarLabel);
    m_avatarLabel->style()->polish(m_avatarLabel);
}

void UserCard::setIsTyping(bool isTyping)
{
    m_isTyping = isTyping;
    setProperty("typing", isTyping);
    if (isTyping) {
        m_messageLabel->setText(tr("is typing ..."));
    }
    style()->unpolish(this);
    style()->polish(this);
}

void UserCard::updateStyle()
{
    // Deprecated: Style is now handled by dynamic properties and stylesheet in setupUI
}

QPixmap UserCard::getRoundedPixmap(const QPixmap &src, int radius)
{
    if (src.isNull()) return QPixmap();
    
    QSize size(radius * 2, radius * 2);
    QPixmap dest(size);
    dest.fill(Qt::transparent);
    
    QPainter painter(&dest);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);
    
    QPainterPath path;
    path.addEllipse(0, 0, size.width(), size.height());
    painter.setClipPath(path);
    
    painter.drawPixmap(0, 0, size.width(), size.height(), src.scaled(size, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation));
    
    return dest;
}

void UserCard::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        emit clicked();
    }
    QFrame::mousePressEvent(event);
}

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
void UserCard::enterEvent(QEnterEvent *event)
#else
void UserCard::enterEvent(QEvent *event)
#endif
{
    m_isHovered = true;
    setProperty("hovered", true);
    style()->unpolish(this);
    style()->polish(this);
    QFrame::enterEvent(event);
}

void UserCard::leaveEvent(QEvent *event)
{
    m_isHovered = false;
    setProperty("hovered", false);
    style()->unpolish(this);
    style()->polish(this);
    QFrame::leaveEvent(event);
}
