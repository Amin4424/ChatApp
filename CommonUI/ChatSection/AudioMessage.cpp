#include "AudioMessage.h"
#include "AudioWaveform.h"
#include "TusDownloader.h"

#include <QHBoxLayout>
#include <QUrl>
#include <QToolButton>
#include <QMenu>
#include <QAction>
#include <QGraphicsDropShadowEffect>
#include <QPainter>
#include <QLinearGradient>
#include <QPixmap>
#include <QEvent>
#include <QPropertyAnimation>
#include <QGraphicsOpacityEffect>
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
#include <QEnterEvent>
#endif

AudioMessage::AudioMessage(const QString &fileName,
                           qint64 fileSize,
                           const QString &fileUrl,
                           int durationSeconds,
                           const QString &senderInfo,
                           MessageDirection direction,
                           const QString &timestamp,
                           const QString &serverHost,
                           const QVector<qreal> &waveform,
                           QWidget *parent)
    : AttachmentMessage(fileName, fileSize, fileUrl, senderInfo, direction, serverHost, parent)
    , m_voiceWidget(nullptr)
    , m_player(new QMediaPlayer(this))
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    , m_audioOutput(new QAudioOutput(this))
#endif
    , m_durationSeconds(durationSeconds)
    , m_waveformGenerator(new AudioWaveform(this))
    , m_preloadedWaveform(waveform)
{
    MessageComponent::withTimestamp(timestamp);
    setAttribute(Qt::WA_Hover);
    setMouseTracking(true);

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    m_player->setAudioOutput(m_audioOutput);
#endif

    connect(m_player, &QMediaPlayer::positionChanged, this, &AudioMessage::onPlayerPositionChanged);
    connect(m_player, &QMediaPlayer::durationChanged, this, &AudioMessage::onPlayerDurationChanged);
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    connect(m_player, &QMediaPlayer::playbackStateChanged, this, &AudioMessage::onPlayerStateChanged);
    connect(m_player, &QMediaPlayer::errorOccurred, this, &AudioMessage::onPlayerError);
#else
    connect(m_player, &QMediaPlayer::stateChanged, this, &AudioMessage::onPlayerStateChanged);
    connect(m_player, QOverload<QMediaPlayer::Error>::of(&QMediaPlayer::error), this, &AudioMessage::onPlayerError);
#endif

    connect(m_waveformGenerator, &AudioWaveform::waveformReady, this, &AudioMessage::onWaveformReady);
    connect(m_tusDownloader, &TusDownloader::downloadProgress, this, &AudioMessage::setInProgressState);

    setupUI();
}

AudioMessage::~AudioMessage() = default;

void AudioMessage::setupUI()
{
    delete layout();

    initializeActionButton();

    auto *mainLayout = new QHBoxLayout(this);
    mainLayout->setContentsMargins(5, 5, 5, 5);
    mainLayout->setSpacing(0);

    m_voiceWidget = new VoiceMessageWidget(this);
    m_voiceWidget->setThemeForMessage(isOutgoing(m_direction));
    m_voiceWidget->setDuration(m_durationSeconds);
    m_voiceWidget->setTimestamp(m_timestamp);
    m_voiceWidget->setStatus(isOutgoing(m_direction) ? MessageBubble::Status::Sent : MessageBubble::Status::None);
    
    if (!m_preloadedWaveform.isEmpty()) {
        m_voiceWidget->setWaveform(m_preloadedWaveform);
    }

    connect(m_voiceWidget, &VoiceMessageWidget::playPauseClicked, this, &AudioMessage::onPlayPauseClicked);
    connect(m_voiceWidget, &VoiceMessageWidget::sliderSeeked, this, &AudioMessage::onSliderSeeked);

    if (isOutgoing(m_direction)) {
        // Outgoing (your) voice messages align to LEFT
        mainLayout->addWidget(m_voiceWidget);
        if (m_moreButton) {
            mainLayout->addWidget(m_moreButton, 0, Qt::AlignTop);
        }
        mainLayout->addStretch(1);
    } else {
        // Incoming voice messages align to RIGHT
        mainLayout->addStretch(1);
        if (m_moreButton) {
            mainLayout->addWidget(m_moreButton, 0, Qt::AlignTop);
        }
        mainLayout->addWidget(m_voiceWidget);
    }

    if (m_isDownloaded) {
        setCompletedState();
        if (!m_preloadedWaveform.isEmpty()) {
            m_voiceWidget->setWaveform(m_preloadedWaveform);
        } else {
            generateWaveform();
        }
    } else {
        setIdleState();
        if (!m_preloadedWaveform.isEmpty()) {
            m_voiceWidget->setWaveform(m_preloadedWaveform);
        }
    }

    if (m_moreButton) {
        m_moreButton->hide();
    }
}

void AudioMessage::setIdleState()
{
    if (!m_voiceWidget) {
        return;
    }

    m_voiceWidget->setPlayIcon(true);
    m_voiceWidget->showIdleState();
    if (isOutgoing(m_direction)) {
        m_voiceWidget->setStatus(MessageBubble::Status::Sent);
    } else {
        m_voiceWidget->setStatus(MessageBubble::Status::None);
    }
}

void AudioMessage::setInProgressState(qint64 bytes, qint64 total)
{
    if (!m_voiceWidget) {
        return;
    }

    m_voiceWidget->setSliderEnabled(false);
    m_voiceWidget->setDownloadProgress(bytes, total);
    if (isOutgoing(m_direction)) {
        m_voiceWidget->setStatus(MessageBubble::Status::Sent);
    } else {
        m_voiceWidget->setStatus(MessageBubble::Status::Pending);
    }
}

void AudioMessage::setCompletedState()
{
    if (!m_voiceWidget) {
        return;
    }

    m_voiceWidget->setDownloadProgress(1, 1);
    m_voiceWidget->setSliderEnabled(true);
    m_voiceWidget->setPlayIcon(true);
    if (isOutgoing(m_direction)) {
        m_voiceWidget->setStatus(MessageBubble::Status::Sent);
    } else {
        m_voiceWidget->setStatus(MessageBubble::Status::None);
    }
    generateWaveform();
}

void AudioMessage::markAsSent()
{
    m_isDownloaded = true;
    setCompletedState();
}

void AudioMessage::onClicked()
{
    if (m_isDownloaded) {
        onPlayPauseClicked();
    } else {
        startDownload();
    }
}

void AudioMessage::onPlayPauseClicked()
{
    if (!m_isDownloaded) {
        startDownload();
        return;
    }

    ensureMediaSource();

    if (m_player->playbackState() == QMediaPlayer::PlayingState) {
        m_player->pause();
    } else {
        m_player->play();
    }
}

void AudioMessage::onSliderSeeked(int position)
{
    if (!m_isDownloaded) {
        return;
    }
    m_player->setPosition(static_cast<qint64>(position) * 1000);
}

void AudioMessage::onPlayerPositionChanged(qint64 position)
{
    if (m_voiceWidget) {
        int currentSeconds = static_cast<int>(position / 1000);
        m_voiceWidget->setCurrentTime(currentSeconds);

        if (m_durationSeconds > 0) {
            qreal progress = static_cast<qreal>(position) / (m_durationSeconds * 1000.0);
            m_voiceWidget->setPlaybackProgress(progress);
        }
    }
}

void AudioMessage::onPlayerDurationChanged(qint64 duration)
{
    m_durationSeconds = static_cast<int>(duration / 1000);
    if (m_voiceWidget) {
        m_voiceWidget->setDuration(m_durationSeconds);
    }
}

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
void AudioMessage::onPlayerStateChanged(QMediaPlayer::PlaybackState state)
#else
void AudioMessage::onPlayerStateChanged(QMediaPlayer::State state)
#endif
{
    if (m_voiceWidget) {
        m_voiceWidget->setPlayIcon(state != QMediaPlayer::PlayingState);

        if (state == QMediaPlayer::StoppedState) {
            m_voiceWidget->setPlaybackProgress(0.0);
        }
    }
}

void AudioMessage::onPlayerError()
{
    qWarning() << "🎙️ Voice playback error:" << m_player->errorString();
    if (m_voiceWidget) {
        m_voiceWidget->setPlayIcon(true);
    }
}

void AudioMessage::ensureMediaSource()
{
    if (m_localFilePath.isEmpty()) {
        return;
    }

    const QUrl expectedSource = QUrl::fromLocalFile(m_localFilePath);

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    if (m_player->source() != expectedSource) {
        m_player->setSource(expectedSource);
    }
#else
    if (m_player->media().canonicalUrl() != expectedSource) {
        m_player->setMedia(expectedSource);
    }
#endif
}

void AudioMessage::generateWaveform()
{
    if (m_localFilePath.isEmpty() || !m_waveformGenerator) {
        return;
    }
    m_waveformGenerator->processFile(m_localFilePath);
}

void AudioMessage::onWaveformReady(const QVector<qreal> &waveform)
{
    // If generation failed (empty) but we already have a waveform (from server/preload), keep the old one.
    if (waveform.isEmpty() && !m_preloadedWaveform.isEmpty()) {
        return;
    }

    m_preloadedWaveform = waveform;

    if (m_voiceWidget) {
        m_voiceWidget->setWaveform(waveform);
    }
}

void AudioMessage::initializeActionButton()
{
    if (m_moreButton) {
        return;
    }

    m_moreButton = new QToolButton(this);
    m_moreButton->setObjectName("voiceActionsButton");
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
    m_actionMenu->setObjectName("voiceActionsMenu");
    applyActionMenuStyle();

    m_deleteAction = m_actionMenu->addAction(deleteIcon(true), tr("Delete"));
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

    connect(m_moreButton, &QToolButton::clicked, this, &AudioMessage::showActionMenu);
}

void AudioMessage::applyActionMenuStyle()
{
    if (!m_actionMenu) {
        return;
    }

    m_actionMenu->setStyleSheet(
        "QMenu#voiceActionsMenu {"
        "    background-color: #ffffff;"
        "    border: 1px solid rgba(15, 23, 42, 0.12);"
        "    border-radius: 18px;"
        "    padding: 10px;"
        "    color: #11151c;"
        "}"
        "QMenu#voiceActionsMenu::item {"
        "    padding: 10px 24px;"
        "    border-radius: 14px;"
        "    color: #11151c;"
        "    font-weight: 600;"
        "}"
        "QMenu#voiceActionsMenu::item:selected {"
        "    background-color: rgba(64, 116, 255, 0.15);"
        "}"
        "QMenu#voiceActionsMenu::item:disabled {"
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

void AudioMessage::showActionMenu()
{
    if (!m_actionMenu || !m_voiceWidget || !m_moreButton) {
        return;
    }

    m_actionMenu->adjustSize();
    const QSize menuSize = m_actionMenu->sizeHint();
    const QPoint bubbleTopLeft = m_voiceWidget->mapToGlobal(QPoint(0, 0));
    const int spacing = 12;

    QPoint menuPos;
    if (isOutgoing(m_direction)) {
        const QPoint buttonLeft = m_moreButton->mapToGlobal(QPoint(0, 0));
        menuPos.setX(buttonLeft.x() - menuSize.width() - spacing);
    } else {
        const QPoint buttonRight = m_moreButton->mapToGlobal(QPoint(m_moreButton->width(), 0));
        menuPos.setX(buttonRight.x() + spacing);
    }

    int y = bubbleTopLeft.y() + (m_voiceWidget->height() - menuSize.height()) / 2;
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

QIcon AudioMessage::deleteIcon(bool emphasizeGlobal) const
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
void AudioMessage::enterEvent(QEnterEvent *event)
#else
void AudioMessage::enterEvent(QEvent *event)
#endif
{
    if (m_moreButton) {
        QGraphicsOpacityEffect *eff = qobject_cast<QGraphicsOpacityEffect*>(m_moreButton->graphicsEffect());
        if (!eff) {
            eff = new QGraphicsOpacityEffect(m_moreButton);
            m_moreButton->setGraphicsEffect(eff);
        }
        
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

void AudioMessage::leaveEvent(QEvent *event)
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
