#include "TusUploader.h"
#include <QDebug>

TusUploader::TusUploader(QObject *parent)
    : QObject(parent),
    m_file(nullptr),
    m_fileSize(0),
    m_bytesUploaded(0),
    m_reply(nullptr)
{
    m_manager = new QNetworkAccessManager(this);
}

TusUploader::~TusUploader()
{
    if (m_file) {
        m_file->close();
        delete m_file;
    }
}

void TusUploader::startUpload(const QString &filePath, const QUrl &tusEndpoint)
{
    m_file = new QFile(filePath);
    if (!m_file->open(QIODevice::ReadOnly)) {
        emit error("Failed to open file: " + m_file->errorString());
        return;
    }

    m_fileSize = m_file->size();
    m_uploadUrl = tusEndpoint; // This is the main endpoint, e.g., http://localhost:1080/files/

    // Step 1: Create the upload resource on the server
    createUpload();
}

void TusUploader::createUpload()
{
    QNetworkRequest request(m_uploadUrl);

    // Set tus headers for creation
    request.setRawHeader("Tus-Resumable", "1.0.0");
    request.setRawHeader("Upload-Length", QByteArray::number(m_fileSize));

    QFileInfo fileInfo(*m_file);
    QString metadata = "filename " + QByteArray(fileInfo.fileName().toUtf8()).toBase64();
    request.setRawHeader("Upload-Metadata", metadata.toLocal8Bit());

    qDebug() << "Tus: Creating upload for" << fileInfo.fileName();

    m_reply = m_manager->post(request, QByteArray()); // Send empty POST

    connect(m_reply, &QNetworkReply::finished, this, &TusUploader::onUploadCreated);
    connect(m_reply, &QNetworkReply::errorOccurred, this, &TusUploader::onUploadError);
}

void TusUploader::onUploadCreated()
{
    if (m_reply->error() != QNetworkReply::NoError) {
        // Error is handled by onUploadError
        return;
    }

    // Server MUST return 201 Created and a Location header
    int statusCode = m_reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    if (statusCode != 201) {
        emit error(QString("Tus: Create failed. Server responded with %1").arg(statusCode));
        m_reply->deleteLater();
        return;
    }

    if (!m_reply->hasRawHeader("Location")) {
        emit error("Tus: Create failed. Server did not provide a Location header.");
        m_reply->deleteLater();
        return;
    }

    // Get the new unique URL for this upload
    m_uploadUrl = m_reply->url().resolved(QUrl(QString::fromUtf8(m_reply->rawHeader("Location"))));
    m_reply->deleteLater();

    qDebug() << "Tus: Upload created at" << m_uploadUrl.toString();

    // Step 2: Start uploading chunks
    m_bytesUploaded = 0;
    uploadChunk();
}

void TusUploader::uploadChunk()
{
    QByteArray chunk = m_file->read(CHUNK_SIZE);
    if (chunk.isEmpty() && m_bytesUploaded != m_fileSize) {
        // This can happen if read() fails but not at end
        qDebug() << "Tus: Read failed, but not at end.";
        // Don't emit finished() here, let error handler take it
        return;
    }

    QNetworkRequest request(m_uploadUrl);
    request.setRawHeader("Tus-Resumable", "1.0.0");
    request.setRawHeader("Upload-Offset", QByteArray::number(m_bytesUploaded));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/offset+octet-stream");

    qDebug() << "Tus: Sending chunk, offset" << m_bytesUploaded << "size" << chunk.size();

    m_reply = m_manager->sendCustomRequest(request, "PATCH", chunk); // Send chunk with PATCH

    connect(m_reply, &QNetworkReply::finished, this, &TusUploader::onChunkUploaded);
    connect(m_reply, &QNetworkReply::uploadProgress, this, [=](qint64 sent, qint64 total){
        // We emit progress based on total file size, not chunk size
        if (total > 0) {
            emit uploadProgress(m_bytesUploaded + sent, m_fileSize);
        }
    });
    connect(m_reply, &QNetworkReply::errorOccurred, this, &TusUploader::onUploadError);
}

void TusUploader::onChunkUploaded()
{
    if (m_reply->error() != QNetworkReply::NoError) {
        // Error is handled by onUploadError (it could be resumed here)
        return;
    }

    // Server MUST return 204 No Content
    int statusCode = m_reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    if (statusCode != 204) {
        emit error(QString("Tus: Chunk upload failed. Server responded with %1").arg(statusCode));
        m_reply->deleteLater();
        return;
    }

    // Check the offset returned by the server
    if (!m_reply->hasRawHeader("Upload-Offset")) {
        emit error("Tus: Chunk upload failed. Server did not return Upload-Offset.");
        m_reply->deleteLater();
        return;
    }

    m_bytesUploaded = m_reply->rawHeader("Upload-Offset").toLongLong();
    m_reply->deleteLater();

    qDebug() << "Tus: Chunk uploaded. New offset is" << m_bytesUploaded;

    if (m_bytesUploaded == m_fileSize) {
        // We are done!
        qDebug() << "Tus: Upload finished.";
        m_file->close();
        emit uploadProgress(m_fileSize, m_fileSize);
        emit finished(m_uploadUrl.toString(), m_fileSize);
    } else {
        // Send the next chunk
        uploadChunk();
    }
}

void TusUploader::onUploadError(QNetworkReply::NetworkError code)
{
    // A real implementation would check for resumable errors (like 409 Conflict)
    // and try to resume using a HEAD request.
    // For now, we will just error out.
    QString errorMsg = m_reply->errorString();
    qDebug() << "Tus: Network Error" << code << errorMsg;
    emit error(errorMsg);

    if (m_reply) {
        m_reply->deleteLater();
    }
    if (m_file && m_file->isOpen()) {
        m_file->close();
    }
}
