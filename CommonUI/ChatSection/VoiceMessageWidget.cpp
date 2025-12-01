#include "VoiceMessageWidget.h"

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QStyle>
#include <QPainter>
#include <QPaintEvent>
#include <QRandomGenerator>
#include <QSizePolicy>
#include <QtGlobal>
#include <algorithm>
#include <QPolygonF>

namespace {
QString colorToCss(const QColor &color)
{
    if (color.alpha() == 255) {
        return color.name();
    }
    return QString("rgba(%1,%2,%3,%4)")
        .arg(color.red())
        .arg(color.green())
        .arg(color.blue())
        .arg(QString::number(color.alphaF(), 'f', 2));
}
}

WaveformWidget::WaveformWidget(QWidget *parent)
    : QWidget(parent)
    , m_playbackProgress(0.0)
    , m_downloadProgress(0.0)
    , m_backgroundColor(Qt::transparent)
    , m_playedColor("#ffffff")
    , m_unplayedColor(QColor(255, 255, 255, 160))
{
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    setMinimumHeight(36);
    setAttribute(Qt::WA_TranslucentBackground);
}

void WaveformWidget::setPlaybackProgress(qreal progress)
{
    m_playbackProgress = std::clamp(progress, 0.0, 1.0);
    update();
}

void WaveformWidget::setDownloadProgress(qreal progress)
{
    m_downloadProgress = std::clamp(progress, 0.0, 1.0);
    update();
}

void WaveformWidget::setWaveform(const QVector<qreal> &waveform)
{
    m_waveform = waveform;
    update();
}

void WaveformWidget::setColors(const QColor &background, const QColor &played, const QColor &unplayed)
{
    m_backgroundColor = background;
    m_playedColor = played;
    m_unplayedColor = unplayed;
    update();
}

void WaveformWidget::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    if (m_backgroundColor.alpha() > 0) {
        painter.fillRect(event->rect(), m_backgroundColor);
    }

    if (m_waveform.isEmpty()) {
        return;
    }

    const int widthPerSample = qMax(1, event->rect().width() / m_waveform.size());
    const int centerY = event->rect().center().y();


    for (int i = 0; i < m_waveform.size(); ++i) {
        const int x = event->rect().left() + i * widthPerSample;
        const qreal sampleValue = m_waveform.at(i);
        const int barHeight = static_cast<int>(sampleValue * (event->rect().height() / 2.0));
        QRect barRect(x, centerY - barHeight, widthPerSample - 1, barHeight * 2);

        // Color based on playback progress (not download!)
        QColor color;
        const qreal barPosition = static_cast<qreal>(i) / m_waveform.size();
        
        if (m_playbackProgress > barPosition) {
            // Already played - green
            color = m_playedColor;
        } else {
            // Not yet played - gray
            color = m_unplayedColor;
        }
        
        painter.fillRect(barRect, color);
    }
}

VoiceMessageWidget::VoiceMessageWidget(QWidget *parent)
    : QFrame(parent)
    , m_playPauseButton(nullptr)
    , m_durationLabel(nullptr)
    , m_timestampLabel(nullptr)
    , m_statusIconLabel(nullptr)
    , m_waveformWidget(nullptr)
    , m_playbackSlider(nullptr)
    , m_waveformLayout(nullptr)
    , m_sliderPressed(false)
    , m_totalDuration(0)
    , m_downloadMode(false)
    , m_showingPlayIcon(true)
{
    setObjectName("VoiceMessageWidget");
    setFrameShape(QFrame::NoFrame);
    setMinimumSize(280, 88);
    setMaximumWidth(420);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    setupUI();
    setThemeForMessage(true);
}

void VoiceMessageWidget::setupUI()
{
    auto *mainLayout = new QHBoxLayout(this);
    mainLayout->setContentsMargins(12, 8, 12, 8);
    mainLayout->setSpacing(12);

    m_playPauseButton = new QToolButton(this);
    m_playPauseButton->setAutoRaise(true);
    m_playPauseButton->setFixedSize(36, 36);
    connect(m_playPauseButton, &QToolButton::clicked, this, &VoiceMessageWidget::playPauseClicked);
    mainLayout->addWidget(m_playPauseButton);

    auto *centerLayout = new QVBoxLayout();
    centerLayout->setSpacing(6);
    centerLayout->setContentsMargins(0, 0, 0, 0);

    m_waveformWidget = new WaveformWidget(this);

    m_playbackSlider = new QSlider(Qt::Horizontal, this);
    m_playbackSlider->setRange(0, 0);
    m_playbackSlider->setEnabled(false);
    connect(m_playbackSlider, &QSlider::sliderPressed, this, [this]() { m_sliderPressed = true; });
    connect(m_playbackSlider, &QSlider::sliderReleased, this, [this]() {
        m_sliderPressed = false;
        emit sliderSeeked(m_playbackSlider->value());
    });
    connect(m_playbackSlider, &QSlider::sliderMoved, this, [this](int value) {
        if (m_sliderPressed) {
            emit sliderSeeked(value);
        }
    });

    m_waveformLayout = new QStackedLayout();
    m_waveformLayout->setStackingMode(QStackedLayout::StackAll);
    m_waveformLayout->addWidget(m_waveformWidget);
    m_waveformLayout->addWidget(m_playbackSlider);
    centerLayout->addLayout(m_waveformLayout);

    auto *bottomLayout = new QHBoxLayout();
    bottomLayout->setContentsMargins(0, 0, 0, 0);

    m_durationLabel = new QLabel("00:00", this);
    bottomLayout->addWidget(m_durationLabel, 0, Qt::AlignLeft);

    bottomLayout->addStretch(1);

    m_timestampLabel = new QLabel("", this);
    bottomLayout->addWidget(m_timestampLabel, 0, Qt::AlignRight);

    m_statusIconLabel = new QLabel("", this);
    bottomLayout->addWidget(m_statusIconLabel, 0, Qt::AlignRight);
    
    centerLayout->addLayout(bottomLayout);
    mainLayout->addLayout(centerLayout);
}

void VoiceMessageWidget::setStatus(MessageBubble::Status status)
{
    bool incomingTheme = (m_statusColor == QColor("#ffffff"));
    QString sentColor = colorToCss(m_statusColor);
    QString pendingColor = incomingTheme ? "rgba(255,255,255,0.85)" : "#8a94a6";

    switch (status) {
    case MessageBubble::Status::Pending:
        m_statusIconLabel->setText("🕒");
        m_statusIconLabel->setStyleSheet(QString("color: %1; font-weight: bold;").arg(pendingColor));
        m_statusIconLabel->setFixedSize(20, 20); // Set desired width and height
        m_statusIconLabel->setStyleSheet(m_statusIconLabel->styleSheet() + "background-color: transparent;");
        m_statusIconLabel->setAlignment(Qt::AlignCenter); // Optional: center the icon/text
        break;
    case MessageBubble::Status::Sent:
        m_statusIconLabel->setText("✓");
        m_statusIconLabel->setStyleSheet(QString("color: %1; font-weight: bold;").arg(sentColor));
        m_statusIconLabel->setFixedSize(20, 20); // Set desired width and height
        m_statusIconLabel->setStyleSheet(m_statusIconLabel->styleSheet() + "background-color: transparent; font-size:10pt");
        m_statusIconLabel->setAlignment(Qt::AlignCenter);
        break;
    default:
        m_statusIconLabel->clear();
        break;
    }
}

void VoiceMessageWidget::setTimestamp(const QString &time)
{
    m_timestampLabel->setText(time);
    m_timestampLabel->setStyleSheet("background-color: transparent; color: #8a94a6; font-size: 8pt;");
}

void VoiceMessageWidget::setDuration(int totalSeconds)
{
    m_totalDuration = qMax(0, totalSeconds);
    if (!m_downloadMode) {
        m_playbackSlider->setRange(0, m_totalDuration);
    }
    if (m_totalDuration == 0) {
        m_durationLabel->setText("00:00");
    } else {
        // Only show total duration to prevent resizing
        m_durationLabel->setText(formatTime(m_totalDuration));
    }
    // If you want to ensure the text color is set, use:
    m_durationLabel->setStyleSheet("background-color: transparent; color: #000000; font-size:  9pt; ");
}

void VoiceMessageWidget::setCurrentTime(int currentSeconds)
{
    if (m_downloadMode) {
        return;
    }

    if (!m_sliderPressed) {
        m_playbackSlider->setValue(currentSeconds);
    }

    // Do not update text with current time to keep size constant
    // Only update if we don't have a total duration yet
    if (m_totalDuration <= 0) {
        m_durationLabel->setText(formatTime(currentSeconds));
    }
}

void VoiceMessageWidget::setPlayIcon(bool showPlay)
{
    m_showingPlayIcon = showPlay;
    updatePlayPauseIcon(showPlay);
}

void VoiceMessageWidget::setDownloadProgress(qint64 bytes, qint64 total)
{
    m_downloadMode = true;
    m_playbackSlider->setEnabled(false);
    m_playbackSlider->setRange(0, 100);

    int percent = (total > 0) ? static_cast<int>((bytes * 100) / total) : 0;
    m_playbackSlider->setValue(qBound(0, percent, 100));
    m_durationLabel->setText(tr("Downloading %1%" ).arg(percent));

    if (total > 0) {
        m_waveformWidget->setDownloadProgress(static_cast<qreal>(bytes) / static_cast<qreal>(total));
    } else {
        m_waveformWidget->setDownloadProgress(0.0);
    }

    if (bytes >= total && total > 0) {
        m_downloadMode = false;
        m_playbackSlider->setEnabled(true);
        m_playbackSlider->setRange(0, m_totalDuration);
        m_playbackSlider->setValue(0);
        // Only show total duration
        m_durationLabel->setText(formatTime(m_totalDuration));
    }
}

void VoiceMessageWidget::setSliderEnabled(bool enabled)
{
    if (!m_downloadMode) {
        m_playbackSlider->setEnabled(enabled);
    }
}

void VoiceMessageWidget::showIdleState()
{
    m_downloadMode = false;
    m_waveformWidget->setDownloadProgress(0.0);
    m_playbackSlider->setEnabled(false);
    m_playbackSlider->setRange(0, m_totalDuration);
    m_playbackSlider->setValue(0);

    if (m_totalDuration > 0) {
        // Only show total duration
        m_durationLabel->setText(formatTime(m_totalDuration));
    } else {
        m_durationLabel->setText("00:00");
    }
}

void VoiceMessageWidget::setWaveform(const QVector<qreal> &waveform)
{
    m_waveformWidget->setWaveform(waveform);
}

void VoiceMessageWidget::setPlaybackProgress(qreal progress)
{
    m_waveformWidget->setPlaybackProgress(progress);
}

QString VoiceMessageWidget::formatTime(int seconds) const
{
    const int mins = seconds / 60;
    const int secs = seconds % 60;
    return QStringLiteral("%1:%2").arg(mins, 2, 10, QLatin1Char('0')).arg(secs, 2, 10, QLatin1Char('0'));
}

void VoiceMessageWidget::setThemeForMessage(bool isOutgoing)
{
    if (isOutgoing) {
        m_backgroundStyle = "background-color: #ffffff; border: 1px solid #e1e7f5;";
        m_playButtonBgColor = QColor("#edf1ff");
        m_playIconColor = QColor("#2f6dff");
        m_durationTextColor = QColor("#27304b");
        m_timeStampColor = QColor("#ffffffff");
        m_statusColor = QColor("#2f6dff");
        m_waveformBgColor = QColor(0, 0, 0, 0);
        m_waveformPlayedColor = QColor("#2f6dff");
        m_waveformUnplayedColor = QColor("#c3cfe8");
        m_sliderFillColor = QColor(0, 0, 0, 0);
        m_sliderHandleColor = QColor(0, 0, 0, 0);
    } else {
        m_backgroundStyle = "background: qlineargradient(x1:0,y1:0,x2:1,y2:1, stop:0 #7ec7ff, stop:1 #2e6bff);";
        m_playButtonBgColor = QColor("#ffffff");
        m_playIconColor = QColor("#2e6bff");
        m_durationTextColor = QColor("#f4f8ff");
        m_timeStampColor = QColor("#f4f8ff");
        m_statusColor = QColor("#ffffff");
        m_waveformBgColor = QColor(255, 255, 255, 0);
        m_waveformPlayedColor = QColor("#ffffff");
        m_waveformUnplayedColor = QColor(255, 255, 255, 150);
        m_sliderFillColor = QColor(0, 0, 0, 0);
        m_sliderHandleColor = QColor(0, 0, 0, 0);
    }
    applyTheme();
}

void VoiceMessageWidget::applyTheme()
{
    setStyleSheet(QString("QFrame#VoiceMessageWidget { %1 border-radius: 18px; }").arg(m_backgroundStyle));
    m_playPauseButton->setStyleSheet(QString(
        "QToolButton { border: none; border-radius: 18px; background-color: %1; }")
        .arg(colorToCss(m_playButtonBgColor)));
    m_durationLabel->setStyleSheet(QString("background-color: transparent; color: %1; font-weight: 600;").arg(colorToCss(m_durationTextColor)));
    m_timestampLabel->setStyleSheet(QString("background-color: transparent; color: %1;").arg(colorToCss(m_timeStampColor)));
    m_statusIconLabel->setStyleSheet(QString("background-color: transparent; color: %1; font-weight: bold;").arg(colorToCss(m_statusColor)));
    if (m_waveformWidget) {
        m_waveformWidget->setColors(m_waveformBgColor, m_waveformPlayedColor, m_waveformUnplayedColor);
    }
    updateSliderStyle();
    updatePlayPauseIcon(m_showingPlayIcon);
}

void VoiceMessageWidget::updateSliderStyle()
{
    QString sliderStyle = QString(
        "QSlider { background: transparent; }"
        "QSlider::groove:horizontal { background: transparent; height: 12px; }"
        "QSlider::sub-page:horizontal { background: transparent; }"
        "QSlider::add-page:horizontal { background: transparent; }"
        "QSlider::handle:horizontal { background: transparent; width: 12px; margin: -6px 0; border: none; }");
    m_playbackSlider->setStyleSheet(sliderStyle);
}

void VoiceMessageWidget::updatePlayPauseIcon(bool showPlay)
{
    const int size = 22;
    QPixmap pix(size, size);
    pix.fill(Qt::transparent);

    QPainter painter(&pix);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setBrush(m_playIconColor);
    painter.setPen(Qt::NoPen);

    if (showPlay) {
        QPolygonF tri;
        tri << QPointF(size * 0.32, size * 0.25)
            << QPointF(size * 0.32, size * 0.75)
            << QPointF(size * 0.75, size * 0.5);
        painter.drawPolygon(tri);
    } else {
        QRectF leftRect(size * 0.3, size * 0.25, size * 0.16, size * 0.5);
        QRectF rightRect(size * 0.54, size * 0.25, size * 0.16, size * 0.5);
        painter.drawRoundedRect(leftRect, 2, 2);
        painter.drawRoundedRect(rightRect, 2, 2);
    }

    m_playPauseButton->setIcon(QIcon(pix));
    m_playPauseButton->setIconSize(pix.size());
}
