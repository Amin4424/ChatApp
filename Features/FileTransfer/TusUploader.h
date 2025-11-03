#ifndef TUSUPLOADER_H
#define TUSUPLOADER_H

#include <QObject>
#include <QString>
#include <QUrl>
#include <QFile>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QFileInfo>

class TusUploader : public QObject
{
    Q_OBJECT

public:
    explicit TusUploader(QObject *parent = nullptr);
    ~TusUploader();

    // Call this to start the upload
    void startUpload(const QString &filePath, const QUrl &tusEndpoint);

signals:
    void finished(const QString &uploadUrl, qint64 fileSize);
    void error(const QString &errorMessage);
    void uploadProgress(qint64 bytesSent, qint64 bytesTotal);

private slots:
    // Slots for the state machine
    void onUploadCreated();
    void onChunkUploaded();
    void onUploadError(QNetworkReply::NetworkError code);

private:
    void createUpload();
    void uploadChunk();

    QNetworkAccessManager *m_manager;
    QFile *m_file;
    qint64 m_fileSize;
    qint64 m_bytesUploaded;
    QUrl m_uploadUrl; // The unique URL for this file, given by the tus server
    QNetworkReply *m_reply;

    // You can adjust this, e.g., 5MB chunks
    static const int CHUNK_SIZE = 5 * 1024 * 1024;
};

#endif // TUSUPLOADER_H
