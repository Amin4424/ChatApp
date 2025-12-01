#include "AudioWaveform.h"

#include <QAudioDecoder>
#include <QAudioBuffer>
#include <QFile>
#include <QUrl>
#include <QtGlobal>
#include <cmath>
#include <QDebug>
AudioWaveform::AudioWaveform(QObject *parent)
    : QObject(parent)
    , m_decoder(nullptr)
{
}

void AudioWaveform::processFile(const QString &filePath)
{
    if (filePath.isEmpty() || !QFile::exists(filePath)) {
        qWarning() << "Audio file does not exist:" << filePath;
        emit waveformReady({});
        return;
    }

    m_samples.clear();
    
    // Create decoder
    if (m_decoder) {
        m_decoder->deleteLater();
    }
    
    m_decoder = new QAudioDecoder(this);
    
    connect(m_decoder, &QAudioDecoder::bufferReady, this, &AudioWaveform::handleBufferReady);
    connect(m_decoder, &QAudioDecoder::finished, this, &AudioWaveform::handleFinished);
    connect(m_decoder, QOverload<QAudioDecoder::Error>::of(&QAudioDecoder::error), this, &AudioWaveform::handleError);

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    m_decoder->setSource(QUrl::fromLocalFile(filePath));
#else
    m_decoder->setSourceFilename(filePath);
#endif
    m_decoder->start();
    
}

void AudioWaveform::handleBufferReady()
{
    if (!m_decoder) {
        return;
    }
    
    QAudioBuffer buffer = m_decoder->read();
    if (!buffer.isValid()) {
        return;
    }

    const int frameCount = buffer.frameCount();
    const int channelCount = buffer.format().channelCount();
    
    // Read audio samples based on format
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    if (buffer.format().sampleFormat() == QAudioFormat::Int16) {
        const qint16 *data = buffer.constData<qint16>();
#else
    if (buffer.format().sampleType() == QAudioFormat::SignedInt && buffer.format().sampleSize() == 16) {
        const qint16 *data = buffer.constData<qint16>();
#endif
        for (int i = 0; i < frameCount; ++i) {
            // Calculate RMS (Root Mean Square) for all channels
            qreal sum = 0.0;
            for (int ch = 0; ch < channelCount; ++ch) {
                qreal sample = static_cast<qreal>(data[i * channelCount + ch]) / 32768.0;
                sum += sample * sample;
            }
            qreal rms = std::sqrt(sum / channelCount);
            m_samples.append(rms);
        }
    }
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    else if (buffer.format().sampleFormat() == QAudioFormat::Float) {
        const float *data = buffer.constData<float>();
#else
    else if (buffer.format().sampleType() == QAudioFormat::Float) {
        const float *data = buffer.constData<float>();
#endif
        for (int i = 0; i < frameCount; ++i) {
            qreal sum = 0.0;
            for (int ch = 0; ch < channelCount; ++ch) {
                qreal sample = static_cast<qreal>(data[i * channelCount + ch]);
                sum += sample * sample;
            }
            qreal rms = std::sqrt(sum / channelCount);
            m_samples.append(rms);
        }
    }
}

void AudioWaveform::handleFinished()
{
    
    if (m_samples.isEmpty()) {
        qWarning() << "No audio samples decoded!";
        emit waveformReady({});
        return;
    }
    
    // Downsample to reasonable size for display (50 bars)
    QVector<qreal> waveform = downsample(m_samples, 50);
    
    // Normalize to 0.0-1.0 range
    qreal maxVal = 0.0;
    for (qreal val : waveform) {
        if (val > maxVal) maxVal = val;
    }
    
    if (maxVal > 0.0) {
        for (qreal &val : waveform) {
            val = val / maxVal;
        }
    }
    
    emit waveformReady(waveform);
}

void AudioWaveform::handleError(QAudioDecoder::Error error)
{
    qWarning() << "🎵 Audio decode error:" << error << (m_decoder ? m_decoder->errorString() : "");
    emit waveformReady({});
}

QVector<qreal> AudioWaveform::downsample(const QVector<qreal> &input, int targetSize)
{
    if (input.size() <= targetSize) {
        return input;
    }

    QVector<qreal> output;
    output.reserve(targetSize);

    const qsizetype blockSize = input.size() / targetSize;

    for (int i = 0; i < targetSize; ++i) {
        const qsizetype start = i * blockSize;
        const qsizetype end = start + blockSize;

        qreal maxVal = 0.0;
        for (qsizetype j = start; j < end; ++j) {
            if (input[j] > maxVal) {
                maxVal = input[j];
            }
        }
        output.append(maxVal);
    }

    return output;
}
