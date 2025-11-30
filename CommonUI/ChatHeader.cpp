#include "ChatHeader.h"
#include <QPainter>
#include <QPainterPath>
#include <QGraphicsDropShadowEffect>
#include <QDir>
#include <QCoreApplication>
#include <QStyleOption>
#include <QPaintEvent>

ChatHeader::ChatHeader(QWidget *parent)
    : QWidget(parent)
{
    setupUI();
}

void ChatHeader::setupUI()
{
    // Height estimation: 
    // Top Margin (16) + Avatar (48) + Bottom Margin (24) = 88px
    // The separator is the bottom border.
    setFixedHeight(88); 
    setStyleSheet("background-color: #FFFFFF; border-bottom: 1px solid #E0E0E0;");

    QHBoxLayout *mainLayout = new QHBoxLayout(this);
    mainLayout->setContentsMargins(48, 16, 48, 24); // Left/Right 48px, Top 16px, Bottom 24px
    mainLayout->setSpacing(16);

    // --- Left Side: Avatar + Info ---
    
    // Avatar
    m_avatarLabel = new QLabel(this);
    m_avatarLabel->setFixedSize(48, 48);
    m_avatarLabel->setStyleSheet("border-radius: 24px; background-color: #CCCCCC;"); // Placeholder
    
    // Info (Name + IP)
    QVBoxLayout *infoLayout = new QVBoxLayout();
    infoLayout->setSpacing(0); // Close spacing
    infoLayout->setContentsMargins(0, 0, 0, 0);
    infoLayout->setAlignment(Qt::AlignVCenter);

    m_nameLabel = new QLabel(this);
    // Font settings as fallback/sizing
    QFont nameFont("Inter");
    nameFont.setPixelSize(20);
    nameFont.setWeight(QFont::Black); // 900
    m_nameLabel->setFont(nameFont);
    m_nameLabel->setText(QString("<span style='font-family:Inter; font-size:20px; font-weight:900; color:#000000; line-height:24px;'>Minerva Barnett</span>"));

    m_ipLabel = new QLabel(this);
    QFont ipFont("Inter");
    ipFont.setPixelSize(14);
    ipFont.setWeight(QFont::Normal); // 400
    m_ipLabel->setFont(ipFont);
    m_ipLabel->setText(QString("<span style='font-family:Inter; font-size:14px; font-weight:400; color:#919191; line-height:20px;'>IP 1245.324.21.01</span>"));

    infoLayout->addWidget(m_nameLabel);
    infoLayout->addWidget(m_ipLabel);

    mainLayout->addWidget(m_avatarLabel);
    mainLayout->addLayout(infoLayout);
    
    mainLayout->addStretch(); // Spacer to push buttons to right

    // --- Right Side: Buttons ---
    
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->setSpacing(24); // Space between buttons

    auto createButton = [this](const QString &iconName, const QString &objName) -> QPushButton* {
        QPushButton *btn = new QPushButton(this);
        btn->setObjectName(objName);
        btn->setFixedSize(24, 24); // Slightly larger touch area, icon is 20
        btn->setCursor(Qt::PointingHandCursor);
        btn->setStyleSheet("QPushButton { border: none; background: transparent; } QPushButton:hover { background-color: #F3F4F6; border-radius: 4px; }");
        
        // Try to load from assets folder relative to executable or current dir
        QString iconPath = "assets/" + iconName;
        if (!QFile::exists(iconPath)) {
             // Fallback to check if we are in build dir and assets are one level up or copied
             if (QFile::exists("../assets/" + iconName)) iconPath = "../assets/" + iconName;
        }

        QIcon icon(iconPath);
        btn->setIcon(icon);
        btn->setIconSize(QSize(20, 20));
        return btn;
    };

    // Search
    m_searchBtn = createButton("magnifier.png", "searchBtn");
    connect(m_searchBtn, &QPushButton::clicked, this, &ChatHeader::searchClicked);

    // Delete
    m_deleteBtn = createButton("trash.png", "deleteBtn");
    connect(m_deleteBtn, &QPushButton::clicked, this, &ChatHeader::deleteClicked);

    // More
    m_moreBtn = createButton("more-vertical.png", "moreBtn");
    connect(m_moreBtn, &QPushButton::clicked, this, &ChatHeader::moreClicked);

    buttonLayout->addWidget(m_searchBtn);
    buttonLayout->addWidget(m_deleteBtn);
    buttonLayout->addWidget(m_moreBtn);

    mainLayout->addLayout(buttonLayout);
}

void ChatHeader::setUserName(const QString &name)
{
    m_nameLabel->setText(QString("<span style='font-family:Inter; font-size:20px; font-weight:900; color:#000000; line-height:24px;'>%1</span>").arg(name));
}

void ChatHeader::setUserIP(const QString &ip)
{
    m_ipLabel->setText(QString("<span style='font-family:Inter; font-size:14px; font-weight:400; color:#919191; line-height:20px;'>%1</span>").arg(ip));
}

void ChatHeader::setAvatar(const QPixmap &avatar)
{
    if (avatar.isNull()) return;
    m_avatarLabel->setPixmap(getRoundedPixmap(avatar, 24)); // Radius 24 for 48px
}

QPixmap ChatHeader::getRoundedPixmap(const QPixmap &src, int radius)
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

void ChatHeader::paintEvent(QPaintEvent *)
{
    QStyleOption opt;
    opt.initFrom(this);
    QPainter p(this);
    style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);
}
