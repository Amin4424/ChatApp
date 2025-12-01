#ifndef AUDIOMESSAGE_H
#define AUDIOMESSAGE_H

#include "AttachmentMessage.h"
#include "VoiceMessageWidget.h"
#include "AudioWaveform.h"

#include <QtMultimedia/QMediaPlayer>
#include <QIcon>

class QEnterEvent;

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
#include <QAudioOutput>
#else
#include <QMediaContent>
#endif

class AudioWaveform;
class QToolButton;
class QMenu;
class QAction;

class AudioMessage : public AttachmentMessage
{
    Q_OBJECT
public:
    explicit AudioMessage(const QString &fileName,
                          qint64 fileSize,
                          const QString &fileUrl,
                          int durationSeconds,
                          const QString &senderInfo,
                          MessageDirection direction,
                          const QString &timestamp,
                          const QString &serverHost = "localhost",
                          const QVector<qreal> &waveform = QVector<qreal>(),
                          QWidget *parent = nullptr);
    ~AudioMessage() override;

    // Make these public so View can call them during upload
    void setInProgressState(qint64 bytes, qint64 total) override;
    void setCompletedState() override;
    void markAsSent();
    void setTransferProgress(qint64 bytes, qint64 total) { setInProgressState(bytes, total); }

signals:
    void deleteRequested(AudioMessage *item);

protected:
    void setupUI() override;
    void setIdleState() override;
    void onClicked() override;
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    void enterEvent(QEnterEvent *event) override;
#else
    void enterEvent(QEvent *event) override;
#endif
    void leaveEvent(QEvent *event) override;

private slots:
    void onPlayPauseClicked();
    void onSliderSeeked(int position);
    void onPlayerPositionChanged(qint64 position);
    void onPlayerDurationChanged(qint64 duration);
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    void onPlayerStateChanged(QMediaPlayer::PlaybackState state);
#else
    void onPlayerStateChanged(QMediaPlayer::State state);
#endif
    void onPlayerError();
    void onWaveformReady(const QVector<qreal> &waveform);

private:
    void ensureMediaSource();
    void generateWaveform();
    void initializeActionButton();
    void applyActionMenuStyle();
    void showActionMenu();
    QIcon deleteIcon(bool emphasizeGlobal) const;

    VoiceMessageWidget *m_voiceWidget;
    QMediaPlayer *m_player;
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    QAudioOutput *m_audioOutput;
#endif
    int m_durationSeconds;
    AudioWaveform *m_waveformGenerator;
    QVector<qreal> m_preloadedWaveform; // Store preloaded waveform data
    QToolButton *m_moreButton = nullptr;
    QMenu *m_actionMenu = nullptr;
    QAction *m_deleteAction = nullptr;
    bool m_menuVisible = false;
};

#endif // AUDIOMESSAGE_H
