#ifndef AUDIOWAVEFORM_H
#define AUDIOWAVEFORM_H

#include <QObject>
#include <QString>
#include <QVector>
#include <QAudioDecoder>

class AudioWaveform : public QObject
{
    Q_OBJECT
public:
    explicit AudioWaveform(QObject *parent = nullptr);

    Q_INVOKABLE void processFile(const QString &filePath);

signals:
    void waveformReady(const QVector<qreal> &waveform);

private slots:
    void handleBufferReady();
    void handleFinished();
    void handleError(QAudioDecoder::Error error);

private:
    QVector<qreal> downsample(const QVector<qreal> &input, int targetSize);
    
    QAudioDecoder *m_decoder;
    QVector<qreal> m_samples;
};

#endif // AUDIOWAVEFORM_H
