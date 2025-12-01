#ifndef VOICEMESSAGEWIDGET_H
#define VOICEMESSAGEWIDGET_H

#include <QFrame>
#include <QWidget>
#include <QToolButton>
#include <QLabel>
#include <QSlider>
#include <QStackedLayout>
#include <QVector>

#include "MessageBubble.h"

class WaveformWidget : public QWidget
{
public:
    explicit WaveformWidget(QWidget *parent = nullptr);

    void setPlaybackProgress(qreal progress); // For playback coloring
    void setDownloadProgress(qreal progress); // For download indication
    void setWaveform(const QVector<qreal> &waveform);
    void setColors(const QColor &background, const QColor &played, const QColor &unplayed);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QVector<qreal> m_waveform;
    qreal m_playbackProgress;  // 0.0 to 1.0 based on playback position
    qreal m_downloadProgress;  // 0.0 to 1.0 for download
    QColor m_backgroundColor;
    QColor m_playedColor;
    QColor m_unplayedColor;
};

class VoiceMessageWidget : public QFrame
{
    Q_OBJECT
public:
    explicit VoiceMessageWidget(QWidget *parent = nullptr);

signals:
    void playPauseClicked();
    void sliderSeeked(int position);

public slots:
    void setStatus(MessageBubble::Status status);
    void setTimestamp(const QString &time);
    void setDuration(int totalSeconds);
    void setCurrentTime(int currentSeconds);
    void setPlayIcon(bool showPlay);
    void setDownloadProgress(qint64 bytes, qint64 total);
    void setSliderEnabled(bool enabled);
    void showIdleState();
    void setWaveform(const QVector<qreal> &waveform);
    void setPlaybackProgress(qreal progress); // Update waveform color during playback
    void setThemeForMessage(bool isOutgoing);

private:
    void setupUI();
    QString formatTime(int seconds) const;
    void applyTheme();
    void updateSliderStyle();
    void updatePlayPauseIcon(bool isPlayIcon);

    QToolButton *m_playPauseButton;
    QLabel *m_durationLabel;
    QLabel *m_timestampLabel;
    QLabel *m_statusIconLabel;
    WaveformWidget *m_waveformWidget;
    QSlider *m_playbackSlider;
    QStackedLayout *m_waveformLayout;
    bool m_sliderPressed;
    int m_totalDuration;
    bool m_downloadMode;
    bool m_showingPlayIcon;

    // Theme colors
    QString m_backgroundStyle;
    QColor m_playButtonBgColor;
    QColor m_playIconColor;
    QColor m_durationTextColor;
    QColor m_timeStampColor;
    QColor m_statusColor;
    QColor m_waveformBgColor;
    QColor m_waveformPlayedColor;
    QColor m_waveformUnplayedColor;
    QColor m_sliderFillColor;
    QColor m_sliderHandleColor;
};

#endif // VOICEMESSAGEWIDGET_H
