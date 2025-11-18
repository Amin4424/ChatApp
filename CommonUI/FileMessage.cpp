#include "FileMessage.h"
#include "TusDownloader.h"

#include <QDateTime>
#include <QDebug>
#include <QHBoxLayout>
#include <QToolButton>
#include <QMenu>
#include <QAction>
#include <QGraphicsDropShadowEffect>
#include <QPainter>
#include <QLinearGradient>
#include <QPixmap>
#include <QEvent>
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
#include <QEnterEvent>
#endif

FileMessage::FileMessage(const QString &fileName,
                         qint64 fileSize,
                         const QString &fileUrl,
                         const QString &senderInfo,
                         MessageDirection direction,
                         const QString &serverHost,
                         const QString &timestamp,
                         QWidget *parent)
    : AttachmentMessage(fileName, fileSize, fileUrl, senderInfo, direction, serverHost, parent)
    , m_infoCard(nullptr)
{
    m_timestamp = timestamp;
    setAttribute(Qt::WA_Hover);
    setMouseTracking(true);
    connect(m_tusDownloader, &TusDownloader::downloadProgress, this, &FileMessage::setInProgressState);
    setupUI();
}

FileMessage::~FileMessage() = default;

void FileMessage::setupUI()
{
    delete layout();

    initializeActionButton();

    auto *mainLayout = new QHBoxLayout(this);
    mainLayout->setContentsMargins(5, 5, 5, 5);
    mainLayout->setSpacing(0);

    m_infoCard = new InfoCard(this);
    m_infoCard->setTitle(m_senderInfo)
              ->setFileName(m_fileName)
              ->setFileSize(formatSize(m_fileSize))
              ->setIcon(getFileIconPixmap(m_fileName));

    if (m_timestamp.isEmpty()) {
        m_timestamp = QDateTime::currentDateTime().toString("hh:mm");
    }
    m_infoCard->setTimestamp(m_timestamp);
    m_infoCard->setCornerRadius(20);

    connect(m_infoCard, &InfoCard::cardClicked, this, &FileMessage::onClicked);

    if (isOutgoing(m_direction)) {
        m_infoCard->setMessageDirection(true)
                  ->setCardBackgroundColor(QColor("#ffffff"))
                  ->setTitleColor(QColor("#6b7387"))
                  ->setFileNameColor(QColor("#11151c"))
                  ->setFileSizeColor(QColor("#7e879a"))
                  ->setTimestampColor(QColor("#8a94a6"));

        m_isSent = true;
        m_isDownloaded = true;
        setCompletedState();

        mainLayout->addStretch(1);
        if (m_moreButton) {
            mainLayout->addWidget(m_moreButton, 0, Qt::AlignTop);
        }
        mainLayout->addWidget(m_infoCard);
    } else {
        m_infoCard->setMessageDirection(false)
                  ->setCardBackgroundGradient("background: qlineargradient(x1:0,y1:0,x2:1,y2:1, stop:0 #7ec7ff, stop:1 #2e6bff); border: none;")
                  ->setTitleColor(QColor("#e6f0ff"))
                  ->setFileNameColor(QColor("#ffffff"))
                  ->setFileSizeColor(QColor("#dceaff"))
                  ->setTimestampColor(QColor("#e7f1ff"));

        m_isDownloaded = false;
        setIdleState();

        mainLayout->addWidget(m_infoCard);
        if (m_moreButton) {
            mainLayout->addWidget(m_moreButton, 0, Qt::AlignTop);
        }
        mainLayout->addStretch(1);
    }

    qDebug() << "[FileMessage] Created | Direction:"
             << (isOutgoing(m_direction) ? "Outgoing" : "Incoming")
             << "| InfoCard size:" << m_infoCard->size();

    if (m_moreButton) {
        m_moreButton->hide();
    }
}

void FileMessage::setIdleState()
{
    if (!m_infoCard) {
        return;
    }

    if (isOutgoing(m_direction)) {
        m_infoCard->setState(InfoCard::State::Completed_Sent);
    } else {
        m_infoCard->setState(InfoCard::State::Idle_Downloadable);
    }
}

void FileMessage::setInProgressState(qint64 bytes, qint64 total)
{
    if (!m_infoCard) {
        return;
    }

    m_infoCard->setState(InfoCard::State::In_Progress_Download);
    m_infoCard->updateProgress(bytes, total);
}

void FileMessage::setCompletedState()
{
    if (!m_infoCard) {
        return;
    }

    if (isOutgoing(m_direction) || m_isSent) {
        m_infoCard->setState(InfoCard::State::Completed_Sent);
    } else {
        m_infoCard->setState(InfoCard::State::Completed_Downloaded);
    }
}

void FileMessage::onClicked()
{
    if (m_isDownloaded) {
        openFile();
    } else {
        startDownload();
    }
}

void FileMessage::markAsSent()
{
    qDebug() << "📤 [FileMessage] markAsSent() called for:" << m_fileName;
    m_isSent = true;
    setCompletedState();
}

void FileMessage::setTransferProgress(qint64 bytes, qint64 total)
{
    if (m_infoCard) {
        m_infoCard->updateProgress(bytes, total);
    }
}

void FileMessage::setTimestamp(const QString &timestamp)
{
    m_timestamp = timestamp;
    if (m_infoCard) {
        m_infoCard->setTimestamp(m_timestamp);
    }
}

void FileMessage::initializeActionButton()
{
    if (m_moreButton) {
        return;
    }

    m_moreButton = new QToolButton(this);
    m_moreButton->setObjectName("fileActionsButton");
    m_moreButton->setCursor(Qt::PointingHandCursor);
    m_moreButton->setAutoRaise(true);
    m_moreButton->setFocusPolicy(Qt::NoFocus);
    m_moreButton->setIconSize(QSize(18, 18));
    m_moreButton->setText(QStringLiteral("⋮"));
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
    m_actionMenu->setObjectName("fileActionsMenu");
    applyActionMenuStyle();

    m_deleteAction = m_actionMenu->addAction(deleteIcon(true), tr("Delete"));
    connect(m_deleteAction, &QAction::triggered, this, [this]() {
        qDebug() << "🔥🔥🔥 [FileMessage] Delete action triggered!";
        qDebug() << "    this:" << this;
        qDebug() << "    databaseId:" << databaseId();
        emit deleteRequested(this);
        qDebug() << "    deleteRequested signal emitted!";
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

    connect(m_moreButton, &QToolButton::clicked, this, &FileMessage::showActionMenu);
}

void FileMessage::applyActionMenuStyle()
{
    if (!m_actionMenu) {
        return;
    }

    m_actionMenu->setStyleSheet(
        "QMenu#fileActionsMenu {"
        "    background-color: #ffffff;"
        "    border: 1px solid rgba(15, 23, 42, 0.12);"
        "    border-radius: 18px;"
        "    padding: 10px;"
        "    color: #11151c;"
        "}"
        "QMenu#fileActionsMenu::item {"
        "    padding: 10px 24px;"
        "    border-radius: 14px;"
        "    color: #11151c;"
        "    font-weight: 600;"
        "}"
        "QMenu#fileActionsMenu::item:selected {"
        "    background-color: rgba(64, 116, 255, 0.15);"
        "}"
        "QMenu#fileActionsMenu::item:disabled {"
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

void FileMessage::showActionMenu()
{
    if (!m_actionMenu || !m_infoCard || !m_moreButton) {
        return;
    }

    m_actionMenu->adjustSize();
    const QSize menuSize = m_actionMenu->sizeHint();
    const QPoint cardTopLeft = m_infoCard->mapToGlobal(QPoint(0, 0));
    const int spacing = 12;

    QPoint menuPos;
    if (isOutgoing(m_direction)) {
        const QPoint buttonLeft = m_moreButton->mapToGlobal(QPoint(0, 0));
        menuPos.setX(buttonLeft.x() - menuSize.width() - spacing);
    } else {
        const QPoint buttonRight = m_moreButton->mapToGlobal(QPoint(m_moreButton->width(), 0));
        menuPos.setX(buttonRight.x() + spacing);
    }

    const int y = cardTopLeft.y() + (m_infoCard->height() - menuSize.height()) / 2;
    menuPos.setY(y);
    m_actionMenu->popup(menuPos);
}

QIcon FileMessage::deleteIcon(bool emphasizeGlobal) const
{
    const int size = 44;
    QPixmap pix(size, size);
    pix.fill(Qt::transparent);

    QPainter painter(&pix);
    painter.setRenderHint(QPainter::Antialiasing, true);

    QRectF bubbleRect(2.0, 2.0, size - 4.0, size - 4.0);
    QColor startColor = emphasizeGlobal ? QColor("#fff0f2") : QColor("#f1f5ff");
    QColor endColor = emphasizeGlobal ? QColor("#ffd4db") : QColor("#dfe8ff");
    QColor strokeColor = emphasizeGlobal ? QColor("#ef3e59") : QColor("#4d67ff");

    QLinearGradient gradient(bubbleRect.topLeft(), bubbleRect.bottomRight());
    gradient.setColorAt(0, startColor);
    gradient.setColorAt(1, endColor);
    painter.setBrush(gradient);
    painter.setPen(Qt::NoPen);
    painter.drawRoundedRect(bubbleRect, 14, 14);

    painter.setPen(QPen(strokeColor, 2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    painter.setBrush(Qt::NoBrush);

    QRectF binRect(bubbleRect.left() + 12, bubbleRect.top() + 14,
                   bubbleRect.width() - 24, bubbleRect.height() - 28);
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

    painter.end();
    return QIcon(pix);
}

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
void FileMessage::enterEvent(QEnterEvent *event)
#else
void FileMessage::enterEvent(QEvent *event)
#endif
{
    if (m_moreButton) {
        m_moreButton->show();
    }
    QWidget::enterEvent(event);
}

void FileMessage::leaveEvent(QEvent *event)
{
    if (m_moreButton && !m_menuVisible) {
        m_moreButton->hide();
    }
    QWidget::leaveEvent(event);
}
