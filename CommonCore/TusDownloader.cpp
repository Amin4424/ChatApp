#include "TusDownloader.h"
#include <QDir>
#include <QStandardPaths>

TusDownloader::TusDownloader(QObject *parent)
    : QObject(parent),
    m_file(nullptr),
    m_reply(nullptr),
    m_decryptionEnabled(false),
    m_currentChunkIndex(0),
    m_bytesDownloaded(0)
{
    m_manager = new QNetworkAccessManager(this);
}

TusDownloader::~TusDownloader()
{
    if (m_file) {
        m_file->close();
        delete m_file;
    }
    if (m_reply) {
        m_reply->deleteLater();
    }
}

void TusDownloader::setDecryptionKey(const QByteArray &key)
{
    if (key.size() != 32) {
        return;
    }
    m_decryptionKey = key;
}

void TusDownloader::startDownload(const QUrl &fileUrl, const QString &savePath, 
                                  bool decrypt, const QByteArray &encryptionMetadata)
{
    m_savePath = savePath;
    m_decryptionEnabled = decrypt;
    m_currentChunkIndex = 0;
    m_bytesDownloaded = 0;
    m_chunkBuffer.clear();

    // Load encryption metadata if decryption is enabled
    if (m_decryptionEnabled) {
        if (encryptionMetadata.isEmpty()) {
            emit error("Decryption requested but no encryption metadata provided");
            return;
        }
        m_chunkMetadata = CryptoManager::deserializeMetadata(encryptionMetadata);
        if (m_chunkMetadata.isEmpty()) {
            emit error("Failed to deserialize encryption metadata");
            return;
        }
        if (m_decryptionKey.size() != 32) {
            emit error("Decryption key not set or invalid");
            return;
        }
    } else {
    }

    // Create directory if it doesn't exist
    QDir dir = QFileInfo(savePath).absoluteDir();
    if (!dir.exists()) {
        dir.mkpath(".");
    }

    m_file = new QFile(savePath);
    if (!m_file->open(QIODevice::WriteOnly)) {
        emit error("Failed to create file: " + m_file->errorString());
        return;
    }

    QNetworkRequest request(fileUrl);
    // Set tus headers for resumable downloads (if supported)
    request.setRawHeader("Tus-Resumable", "1.0.0");
    

    m_reply = m_manager->get(request);

    // --- FIX: Add readyRead to write incrementally ---
    connect(m_reply, &QNetworkReply::readyRead, this, &TusDownloader::onDataReady);
    connect(m_reply, &QNetworkReply::downloadProgress, this, &TusDownloader::onDownloadProgress);
    connect(m_reply, &QNetworkReply::finished, this, &TusDownloader::onDownloadFinished);
    connect(m_reply, QOverload<QNetworkReply::NetworkError>::of(&QNetworkReply::errorOccurred),
            this, &TusDownloader::onDownloadError);
}

void TusDownloader::onDownloadProgress(qint64 bytesReceived, qint64 bytesTotal)
{
    emit downloadProgress(bytesReceived, bytesTotal);
}

void TusDownloader::onDataReady()
{
    // --- FIX: Write data incrementally as it arrives ---
    if (m_file && m_file->isOpen() && m_reply) {
        QByteArray data = m_reply->readAll();
        if (!data.isEmpty()) {
            if (m_decryptionEnabled) {
                // Add to buffer for chunk-based decryption
                m_chunkBuffer.append(data);
                processDownloadedChunks();
            } else {
                // No encryption - write directly
                m_file->write(data);
                m_file->flush(); // Ensure data is written to disk
            }
        }
    }
}

void TusDownloader::processDownloadedChunks()
{
    // Process complete chunks from buffer
    while (m_currentChunkIndex < m_chunkMetadata.size()) {
        const ChunkMetadata &metadata = m_chunkMetadata[m_currentChunkIndex];
        
        // Check if we have enough data for this chunk
        if (m_chunkBuffer.size() < metadata.encryptedSize) {
            // Wait for more data
            break;
        }

        // Extract this chunk's encrypted data
        QByteArray encryptedChunk = m_chunkBuffer.left(metadata.encryptedSize);
        m_chunkBuffer.remove(0, metadata.encryptedSize);

        // Decrypt the chunk
        QByteArray decryptedChunk;
        if (!CryptoManager::decryptChunk(encryptedChunk, m_decryptionKey, metadata, decryptedChunk)) {
            emit error(QString("Failed to decrypt chunk %1").arg(m_currentChunkIndex));
            if (m_reply) {
                m_reply->abort();
            }
            return;
        }

        // Write decrypted data to file
        if (m_file->write(decryptedChunk) == -1) {
            emit error("Failed to write decrypted data to file");
            if (m_reply) {
                m_reply->abort();
            }
            return;
        }
        m_file->flush();


        m_currentChunkIndex++;
    }
}

void TusDownloader::onDownloadFinished()
{
    if (m_reply->error() != QNetworkReply::NoError) {
        return; // Error handled in onDownloadError
    }

    // --- FIX: Write any remaining data (though onDataReady should have handled most) ---
    if (m_file && m_file->isOpen()) {
        QByteArray remaining = m_reply->readAll();
        if (!remaining.isEmpty()) {
            if (m_decryptionEnabled) {
                m_chunkBuffer.append(remaining);
                processDownloadedChunks();
            } else {
                m_file->write(remaining);
            }
        }

        // Verify all chunks were processed if decryption was enabled
        if (m_decryptionEnabled) {
            if (m_currentChunkIndex != m_chunkMetadata.size()) {
                // Removed qWarning
            }
            if (!m_chunkBuffer.isEmpty()) {
                // Removed qWarning
            }
        }

        m_file->flush();
        m_file->close();
    }

    QString completionMsg = m_decryptionEnabled 
        ? QString("Download and decryption completed successfully (%1 chunks)").arg(m_currentChunkIndex)
        : "Download completed successfully";
    
    emit finished(m_savePath);

    m_reply->deleteLater();
    m_reply = nullptr;
}

void TusDownloader::onDownloadError(QNetworkReply::NetworkError code)
{
    QString errorMsg = "Download failed: " + m_reply->errorString();

    if (m_file && m_file->isOpen()) {
        m_file->close();
        // Remove incomplete file
        m_file->remove();
    }

    emit error(errorMsg);

    if (m_reply) {
        m_reply->deleteLater();
        m_reply = nullptr;
    }
}