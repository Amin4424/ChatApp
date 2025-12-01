#include "TextMessage.h"

#include <QAction>
#include <QCursor>
#include <QHBoxLayout>
#include <QMenu>
#include <QSize>
#include <QSizePolicy>
#include <QToolButton>
#include <QGraphicsDropShadowEffect>
#include <QPainter>
#include <QLinearGradient>
#include <QDateTime>
#include <QPropertyAnimation>
#include <QGraphicsOpacityEffect>

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
#include <QEnterEvent>
#endif

TextMessage::TextMessage(const QString &messageText,
                         const QString &senderInfo,
                         MessageDirection direction,
                         const QString &timestamp,
                         QWidget *parent)
    : MessageComponent(senderInfo, direction, parent)
    , m_bubble(nullptr)
    , m_moreButton(nullptr)
    , m_actionMenu(nullptr)
    , m_copyAction(nullptr)
    , m_editAction(nullptr)
    , m_deleteAction(nullptr)
    , m_messageText(messageText)
    , m_isEdited(false) // Initialize m_isEdited
{
    MessageComponent::withTimestamp(timestamp);

    setAttribute(Qt::WA_Hover);
    setMouseTracking(true);
    setupUI();
    applyStyles();
}

TextMessage::~TextMessage() = default;

TextMessage &TextMessage::withMessage(const QString &messageText)
{
    m_messageText = messageText;
    if (m_bubble) {
        m_bubble->setMessageText(m_messageText);
    }
    return *this;
}

TextMessage &TextMessage::withSender(const QString &senderInfo)
{
    MessageComponent::withSender(senderInfo);
    if (m_bubble) {
        m_bubble->setSenderText(QString());
    }
    return *this;
}

TextMessage &TextMessage::withTimestamp(const QString &timestamp)
{
    MessageComponent::withTimestamp(timestamp);
    if (m_bubble) {
        m_bubble->setTimestamp(m_timestamp);
    }
    return *this;
}

TextMessage &TextMessage::withDirection(MessageDirection direction)
{
    MessageComponent::withDirection(direction);
    setupUI();
    applyStyles();
    return *this;
}

void TextMessage::setupUI()
{
    delete layout();

    initializeActionButton();

    auto *mainLayout = new QHBoxLayout(this);
    mainLayout->setContentsMargins(5, 5, 5, 5);
    mainLayout->setSpacing(0);

    // Placeholder to force proper sizing when no content is provided
    const QString fallbackMessage = QStringLiteral("Salam, this is a test message to fix the layout.");
    if (m_messageText.trimmed().isEmpty()) {
        m_messageText = fallbackMessage;
    }

    m_bubble = new MessageBubble(this);
    m_bubble->setSenderText(QString());
    if (m_timestamp.isEmpty()) {
        m_timestamp = QDateTime::currentDateTime().toString("hh:mm");
    }
    m_bubble->setMessageText(m_messageText);
    m_bubble->setMessageType(isOutgoing(m_direction) ? ::MessageType::Sent : ::MessageType::Received);

    m_bubble->setTimestamp(m_timestamp);

    m_bubble->setMessageStatus(MessageBubble::Status::Sent);
    
    // Apply edited state if needed
    if (m_isEdited) {
        m_bubble->setEdited(true);
    }

    // Subtle shadow to lift the bubble from the canvas
    auto *shadow = new QGraphicsDropShadowEffect(this);
    shadow->setBlurRadius(18);
    shadow->setOffset(0, 2);
    shadow->setColor(QColor(0, 0, 0, 28));
    m_bubble->setGraphicsEffect(shadow);

    if (isOutgoing(m_direction)) {
        // Outgoing (your) messages align to LEFT
        mainLayout->addWidget(m_bubble);
        if (m_moreButton) {
            mainLayout->addWidget(m_moreButton, 0, Qt::AlignTop);
        }
        mainLayout->addStretch(1);
    } else {
        // Incoming messages align to RIGHT
        mainLayout->addStretch(1);
        if (m_moreButton) {
            mainLayout->addWidget(m_moreButton, 0, Qt::AlignTop);
        }
        mainLayout->addWidget(m_bubble);
    }

    if (m_moreButton) {
        m_moreButton->hide();
    }
}

void TextMessage::applyStyles()
{
    if (!m_bubble) {
        return;
    }

    if (isOutgoing(m_direction)) {
        // Outgoing (your) messages: clean white bubble with strong contrast text
        const QColor bubbleBg("#ffffff");
        const QColor bubbleBorder("#dfe3eb");
        const QColor senderColor("#555555");
        const QColor messageColor("#111111");

        m_bubble->setBubbleBackgroundColor(bubbleBg)
               ->setBubbleBorderColor(bubbleBorder)
               ->setBubbleBorderRadius(18)
               ->setBubblePadding(14)
               ->setSenderTextColor(senderColor)
               ->setMessageTextColor(messageColor);
        m_bubble->setBubbleStyleSheet(QString(
            "QFrame#messageBubbleFrame {"
            "    background-color: %1;"
            "    border: 1px solid %2;"
            "    border-radius: 18px;"
            "}"
        ).arg(bubbleBg.name(), bubbleBorder.name()));
    } else {
        m_bubble->setBubbleBorderRadius(18)
               ->setBubblePadding(14)
               ->setSenderTextColor(QColor("#e9f3ff"))
               ->setMessageTextColor(QColor("#ffffff"));
        m_bubble->setEditedStyle("QLabel { background-color: transparent; color: #ffffff; font-weight: bold; }");
        m_bubble->setBubbleStyleSheet(
            "QFrame#messageBubbleFrame {"
            "    background: qlineargradient(x1:0,y1:0,x2:1,y2:1, stop:0 #7ec7ff, stop:1 #2e6bff);"
            "    border: none;"
            "    border-radius: 18px;"
            "}"
        );
    }

    QFont senderFont = m_bubble->senderLabel()->font();
    senderFont.setBold(true);
    senderFont.setPointSize(9);
    m_bubble->setSenderFont(senderFont);

    QFont messageFont = m_bubble->messageLabel()->font();
    messageFont.setPointSize(14);  // Larger font for better visibility
    messageFont.setWeight(QFont::Normal);
    m_bubble->setMessageFont(messageFont);

    // Force geometry update after style application
    m_bubble->adjustSize();
    m_bubble->updateGeometry();
    adjustSize();
    updateGeometry();
}

void TextMessage::markAsSent()
{
    if (m_bubble) {
        m_bubble->setMessageStatus(MessageBubble::Status::Sent);
    }
}

void TextMessage::updateMessageText(const QString &newText)
{
    if (newText == m_messageText) {
        return;
    }

    m_messageText = newText;
    if (m_bubble) {
        
        m_bubble->setMessageText(m_messageText);
        
        if (m_bubble->messageLabel()) {
            m_bubble->messageLabel()->adjustSize();
        }
        m_bubble->adjustSize();
        m_bubble->updateGeometry();
    }
    adjustSize();
    updateGeometry();
}

void TextMessage::markAsEdited(bool edited)
{
    m_isEdited = edited;
    if (m_bubble) {
        m_bubble->setEdited(edited);
        // Force layout update to ensure size hint is correct
        adjustSize();
        updateGeometry();
    }
}

void TextMessage::initializeActionButton()
{
    if (m_moreButton) {
        return;
    }

    m_moreButton = new QToolButton(this);
    m_moreButton->setObjectName("messageActionsButton");
    m_moreButton->setCursor(Qt::PointingHandCursor);
    m_moreButton->setAutoRaise(true);
    m_moreButton->setFocusPolicy(Qt::NoFocus);
    m_moreButton->setIconSize(QSize(18, 18));
    m_moreButton->setText(QStringLiteral("⋮"));
    m_moreButton->setToolTip(tr("Message actions"));
    m_moreButton->setStyleSheet(
        "QToolButton {"
        "    border: 1px solid rgba(15, 23, 42, 0.08);"
        "    background-color: #ffffff;"
        "    padding: 6px;"
        "    border-radius: 16px;"
        "    color: #3c445c;"
        "}"
        "QToolButton:hover {"
        "    background-color: #f5f7ff;"
        "}"
        "QToolButton::menu-indicator { image: none; }"
    );

    m_actionMenu = new QMenu(m_moreButton);
    m_actionMenu->setObjectName("messageActionsMenu");
    applyActionMenuStyle();

    m_copyAction = m_actionMenu->addAction(actionIcon(MenuActionIcon::Copy), tr("Copy"));
    m_editAction = m_actionMenu->addAction(actionIcon(MenuActionIcon::Edit), tr("Edit"));
    m_deleteAction = m_actionMenu->addAction(actionIcon(MenuActionIcon::Delete), tr("Delete"));
    QFont deleteFont = m_actionMenu->font();
    deleteFont.setBold(true);
    m_deleteAction->setFont(deleteFont);

    connect(m_moreButton, &QToolButton::clicked, this, &TextMessage::showActionMenu);
    connect(m_copyAction, &QAction::triggered, this, [this]() {
        emit copyRequested(m_messageText);
    });
    connect(m_editAction, &QAction::triggered, this, [this]() {
        emit editRequested(this);
    });
    connect(m_deleteAction, &QAction::triggered, this, [this]() {
        emit deleteRequested(this);
    });

    connect(m_actionMenu, &QMenu::aboutToShow, this, [this]() {
        m_menuVisible = true;
        if (m_moreButton) {
            m_moreButton->show();
        }
    });
    connect(m_actionMenu, &QMenu::aboutToHide, this, [this]() {
        m_menuVisible = false;
        if (!underMouse() && m_moreButton) {
            m_moreButton->hide();
        }
    });
    updateActionStates();
}

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
void TextMessage::enterEvent(QEnterEvent *event)
#else
void TextMessage::enterEvent(QEvent *event)
#endif
{
    if (m_moreButton) {
        QGraphicsOpacityEffect *eff = qobject_cast<QGraphicsOpacityEffect*>(m_moreButton->graphicsEffect());
        if (!eff) {
            eff = new QGraphicsOpacityEffect(m_moreButton);
            m_moreButton->setGraphicsEffect(eff);
        }
        
        // If hidden, start from 0 opacity
        if (!m_moreButton->isVisible()) {
            eff->setOpacity(0.0);
            m_moreButton->show();
        }

        QPropertyAnimation *a = new QPropertyAnimation(eff, "opacity");
        a->setDuration(150);
        a->setStartValue(eff->opacity());
        a->setEndValue(1.0);
        a->setEasingCurve(QEasingCurve::OutQuad);
        a->start(QAbstractAnimation::DeleteWhenStopped);
    }
    QWidget::enterEvent(event);
}

void TextMessage::leaveEvent(QEvent *event)
{
    if (m_moreButton && !m_menuVisible) {
        QGraphicsOpacityEffect *eff = qobject_cast<QGraphicsOpacityEffect*>(m_moreButton->graphicsEffect());
        if (!eff) {
            eff = new QGraphicsOpacityEffect(m_moreButton);
            m_moreButton->setGraphicsEffect(eff);
        }

        QPropertyAnimation *a = new QPropertyAnimation(eff, "opacity");
        a->setDuration(150);
        a->setStartValue(eff->opacity());
        a->setEndValue(0.0);
        a->setEasingCurve(QEasingCurve::OutQuad);
        connect(a, &QPropertyAnimation::finished, m_moreButton, &QToolButton::hide);
        a->start(QAbstractAnimation::DeleteWhenStopped);
    }
    QWidget::leaveEvent(event);
}

void TextMessage::setDatabaseId(int id)
{
    MessageComponent::setDatabaseId(id);
    updateActionStates();
}

void TextMessage::updateActionStates()
{
    const bool canModify = isOutgoing(m_direction) && (databaseId() >= 0);
    if (m_editAction) {
        m_editAction->setVisible(canModify);
    }
    if (m_deleteAction) {
        m_deleteAction->setVisible(canModify);
    }
}

void TextMessage::showActionMenu()
{
    if (!m_actionMenu || !m_bubble || !m_moreButton) {
        return;
    }

    m_actionMenu->adjustSize();
    const QSize menuSize = m_actionMenu->sizeHint();
    const QPoint bubbleTopLeft = m_bubble->mapToGlobal(QPoint(0, 0));
    const int spacing = 12;

    QPoint menuPos;
    if (isOutgoing(m_direction)) {
        const QPoint buttonLeft = m_moreButton->mapToGlobal(QPoint(0, 0));
        menuPos.setX(buttonLeft.x() - menuSize.width() - spacing);
    } else {
        const QPoint buttonRight = m_moreButton->mapToGlobal(QPoint(m_moreButton->width(), 0));
        menuPos.setX(buttonRight.x() + spacing);
    }

    int y = bubbleTopLeft.y() + (m_bubble->height() - menuSize.height()) / 2;
    menuPos.setY(y);
    
    // Fade in animation for menu
    m_actionMenu->setWindowOpacity(0.0);
    m_actionMenu->popup(menuPos);
    
    QPropertyAnimation *a = new QPropertyAnimation(m_actionMenu, "windowOpacity");
    a->setDuration(150);
    a->setStartValue(0.0);
    a->setEndValue(1.0);
    a->setEasingCurve(QEasingCurve::OutQuad);
    a->start(QAbstractAnimation::DeleteWhenStopped);
}

void TextMessage::applyActionMenuStyle()
{
    if (!m_actionMenu) {
        return;
    }

    m_actionMenu->setStyleSheet(
        "QMenu#messageActionsMenu {"
        "    background-color: #ffffff;"
        "    border: 1px solid rgba(15, 23, 42, 0.12);"
        "    border-radius: 18px;"
        "    padding: 10px;"
        "    color: #11151c;"
    "}"
        "QMenu#messageActionsMenu::item {"
        "    padding: 10px 24px;"
        "    border-radius: 14px;"
        "    color: #11151c;"
        "    font-weight: 600;"
    "}"
        "QMenu#messageActionsMenu::item:selected {"
        "    background-color: rgba(64, 116, 255, 0.15);"
    "}"
        "QMenu#messageActionsMenu::item:disabled {"
        "    color: rgba(17, 21, 28, 0.45);"
    "}"
    );

#if QT_CONFIG(graphicseffect)
    auto *shadow = new QGraphicsDropShadowEffect(m_actionMenu);
    shadow->setBlurRadius(28);
    shadow->setOffset(0, 12);
    shadow->setColor(QColor(15, 23, 42, 50));
    m_actionMenu->setGraphicsEffect(shadow);
#endif
}

QIcon TextMessage::actionIcon(MenuActionIcon type) const
{
    const int size = 44;
    QPixmap pix(size, size);
    pix.fill(Qt::transparent);
    QPainter painter(&pix);
    painter.setRenderHint(QPainter::Antialiasing, true);

    QRectF bubbleRect(2.0, 2.0, size - 4.0, size - 4.0);
    QColor startColor;
    QColor endColor;
    QColor strokeColor;

    switch (type) {
    case MenuActionIcon::Copy:
        startColor = QColor("#f1f5ff");
        endColor = QColor("#dfe8ff");
        strokeColor = QColor("#4d67ff");
        break;
    case MenuActionIcon::Edit:
        startColor = QColor("#fff6e9");
        endColor = QColor("#ffd8b3");
        strokeColor = QColor("#f1772f");
        break;
    case MenuActionIcon::Delete:
        startColor = QColor("#fff0f2");
        endColor = QColor("#ffd4db");
        strokeColor = QColor("#ef3e59");
        break;
    }

    QLinearGradient gradient(bubbleRect.topLeft(), bubbleRect.bottomRight());
    gradient.setColorAt(0, startColor);
    gradient.setColorAt(1, endColor);
    painter.setBrush(gradient);
    painter.setPen(Qt::NoPen);
    painter.drawRoundedRect(bubbleRect, 14, 14);

    painter.setPen(QPen(strokeColor, 2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    painter.setBrush(Qt::NoBrush);

    switch (type) {
    case MenuActionIcon::Copy: {
        QRectF frontRect(bubbleRect.left() + 10, bubbleRect.top() + 12,
                         bubbleRect.width() - 22, bubbleRect.height() - 22);
        QRectF backRect = frontRect.translated(5, -5);

        QPen backPen(strokeColor.lighter(140));
        backPen.setWidthF(1.6);
        backPen.setCapStyle(Qt::RoundCap);
        backPen.setJoinStyle(Qt::RoundJoin);

        painter.setPen(backPen);
        painter.drawRoundedRect(backRect, 6, 6);

        painter.setPen(QPen(strokeColor, 2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        painter.drawRoundedRect(frontRect, 6, 6);
        painter.drawLine(QPointF(frontRect.left() + 5, frontRect.top() + 9),
                         QPointF(frontRect.right() - 5, frontRect.top() + 9));
        painter.drawLine(QPointF(frontRect.left() + 5, frontRect.top() + 15),
                         QPointF(frontRect.right() - 5, frontRect.top() + 15));
        break;
    }
    case MenuActionIcon::Edit: {
        QPointF startPoint(bubbleRect.left() + 12, bubbleRect.bottom() - 14);
        QPointF endPoint(bubbleRect.right() - 14, bubbleRect.top() + 14);

        QPen highlight(QColor("#ffe4c6"), 8, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
        painter.setPen(highlight);
        painter.drawLine(startPoint, endPoint);

        QPen bodyPen(strokeColor, 5, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
        painter.setPen(bodyPen);
        painter.drawLine(startPoint, endPoint);

        QPolygonF tip;
        tip << QPointF(endPoint.x() + 6, endPoint.y() - 6)
            << QPointF(endPoint.x() - 1, endPoint.y() + 1)
            << QPointF(endPoint.x() + 4, endPoint.y() + 8);
        painter.setBrush(Qt::white);
        painter.setPen(QPen(QColor("#b3541c"), 1.8, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        painter.drawPolygon(tip);
        break;
    }
    case MenuActionIcon::Delete: {
        QRectF binRect(bubbleRect.left() + 12, bubbleRect.top() + 14,
                       bubbleRect.width() - 24, bubbleRect.height() - 28);
        painter.setPen(QPen(strokeColor, 2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        painter.drawRoundedRect(binRect, 6, 6);

        const qreal centerX = binRect.center().x();
        painter.drawLine(QPointF(centerX, binRect.top() + 2),
                         QPointF(centerX, binRect.bottom() - 2));
        painter.drawLine(QPointF(centerX - 6, binRect.top() + 2),
                         QPointF(centerX - 6, binRect.bottom() - 2));
        painter.drawLine(QPointF(centerX + 6, binRect.top() + 2),
                         QPointF(centerX + 6, binRect.bottom() - 2));

        QRectF lidRect(bubbleRect.left() + 10, bubbleRect.top() + 8,
                       bubbleRect.width() - 20, 6);
        painter.drawLine(QPointF(lidRect.left(), lidRect.bottom()),
                         QPointF(lidRect.right(), lidRect.bottom()));
        painter.drawLine(QPointF(lidRect.left() + 4, lidRect.top()),
                         QPointF(lidRect.right() - 4, lidRect.top()));
        painter.drawLine(QPointF(bubbleRect.center().x(), lidRect.top()),
                         QPointF(bubbleRect.center().x(), lidRect.top() - 4));
        break;
    }
    }

    painter.end();
    return QIcon(pix);
}
