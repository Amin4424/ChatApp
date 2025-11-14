#ifndef TUSDOWNLOADER_H
#define TUSDOWNLOADER_H

#include <QObject>
#include <QString>
#include <QUrl>
#include <QFile>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QVector>
#include "CryptoManager.h"

class TusDownloader : public QObject
{
    Q_OBJECT

public:
    explicit TusDownloader(QObject *parent = nullptr);
    ~TusDownloader();

    // Call this to start the download (with optional decryption)
    void startDownload(const QUrl &fileUrl, const QString &savePath, 
                      bool decrypt = false, const QByteArray &encryptionMetadata = QByteArray());

    // Set decryption key (must be 32 bytes for AES-256)
    void setDecryptionKey(const QByteArray &key);

signals:
    void finished(const QString &filePath);
    void error(const QString &errorMessage);
    void downloadProgress(qint64 bytesReceived, qint64 bytesTotal);

private slots:
    void onDataReady(); // --- ADD: Handle incoming data incrementally
    void onDownloadProgress(qint64 bytesReceived, qint64 bytesTotal);
    void onDownloadFinished();
    void onDownloadError(QNetworkReply::NetworkError code);

private:
    QNetworkAccessManager *m_manager;
    QFile *m_file;
    QNetworkReply *m_reply;
    QString m_savePath;

    // Decryption support
    bool m_decryptionEnabled;
    QByteArray m_decryptionKey;
    QVector<ChunkMetadata> m_chunkMetadata;
    qint64 m_currentChunkIndex;
    QByteArray m_chunkBuffer; // Buffer for assembling chunks
    qint64 m_bytesDownloaded;

    // Chunk size must match uploader (5MB)
    static const int CHUNK_SIZE = 5 * 1024 * 1024;

    void processDownloadedChunks();
};

#endif // TUSDOWNLOADER_H