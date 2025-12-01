#include "TusUploader.h"

TusUploader::TusUploader(QObject *parent)
    : QObject(parent),
    m_file(nullptr),
    m_fileSize(0),
    m_bytesUploaded(0),
    m_reply(nullptr),
    m_encryptionEnabled(false),
    m_currentChunkIndex(0)
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

void TusUploader::setEncryptionKey(const QByteArray &key)
{
    if (key.size() != 32) {
        return;
    }
    m_encryptionKey = key;
}

void TusUploader::startUpload(const QString &filePath, const QUrl &tusEndpoint, bool encrypt)
{
    m_encryptionEnabled = encrypt;
    m_currentChunkIndex = 0;
    m_chunkMetadata.clear();

    if (m_encryptionEnabled && m_encryptionKey.size() != 32) {
        // Generate a key if encryption is enabled but no key is set
        m_encryptionKey = CryptoManager::generateAES256Key();
    }

    m_file = new QFile(filePath);
    if (!m_file->open(QIODevice::ReadOnly)) {
        emit error("Failed to open file: " + m_file->errorString());
        return;
    }

    m_fileSize = m_file->size();
    m_uploadUrl = tusEndpoint; // This is the main endpoint, e.g., http://192.168.1.10:1080/files/


    // Step 1: Create the upload resource on the server
    createUpload();
}

void TusUploader::cancelUpload()
{
    
    // Disconnect all signals to prevent segfault
    if (m_reply) {
        m_reply->disconnect();
        m_reply->abort();
        m_reply->deleteLater();
        m_reply = nullptr;
    }
    
    // Close file
    if (m_file) {
        m_file->close();
        delete m_file;
        m_file = nullptr;
    }
    
    // Disconnect from manager
    if (m_manager) {
        m_manager->disconnect();
    }
    
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

    // --- FIX: Store the relative path, not the full resolved URL ---
    // Get the new unique URL for this upload
    // m_uploadUrl = m_reply->url().resolved(QUrl(QString::fromUtf8(m_reply->rawHeader("Location"))));
    m_uploadUrl = QUrl(QString::fromUtf8(m_reply->rawHeader("Location")));
    // --- END FIX ---
    
    m_reply->deleteLater();


    // Step 2: Start uploading chunks
    m_bytesUploaded = 0;
    uploadChunk();
}

void TusUploader::uploadChunk()
{
    QByteArray chunk = m_file->read(CHUNK_SIZE);
    if (chunk.isEmpty() && m_bytesUploaded != m_fileSize) {
        // This can happen if read() fails but not at end
        return;
    }

    QByteArray dataToUpload = chunk;

    // Encrypt chunk if encryption is enabled
    if (m_encryptionEnabled && !chunk.isEmpty()) {
        ChunkMetadata metadata;
        metadata.chunkIndex = m_currentChunkIndex++;
        QByteArray encryptedChunk;
        if (!CryptoManager::encryptChunk(chunk, m_encryptionKey, encryptedChunk, metadata)) {
            emit error("Failed to encrypt chunk");
            return;
        }
        m_chunkMetadata.append(metadata);
        dataToUpload = encryptedChunk;
    }

    QNetworkRequest request(m_uploadUrl);
    request.setRawHeader("Tus-Resumable", "1.0.0");
    request.setRawHeader("Upload-Offset", QByteArray::number(m_bytesUploaded));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/offset+octet-stream");


    m_reply = m_manager->sendCustomRequest(request, "PATCH", dataToUpload); // Send chunk with PATCH

    connect(m_reply, &QNetworkReply::finished, this, &TusUploader::onChunkUploaded);
    connect(m_reply, &QNetworkReply::uploadProgress, this, [=](qint64 sent, qint64 total){
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

    if (!m_reply->hasRawHeader("Upload-Offset")) {
        emit error("Tus: Chunk upload failed. Server did not return Upload-Offset.");
        m_reply->deleteLater();
        return;
    }

    m_bytesUploaded = m_reply->rawHeader("Upload-Offset").toLongLong();
    m_reply->deleteLater();


    if (m_bytesUploaded == m_fileSize) {
        // We are done!
        m_file->close();
        emit uploadProgress(m_fileSize, m_fileSize);
        // Serialize encryption metadata if encryption was used
        QByteArray metadata;
        if (m_encryptionEnabled) {
            metadata = CryptoManager::serializeMetadata(m_chunkMetadata);
        }
        // --- FIX: Emit the *path* only, not the full URL ---
        emit finished(m_uploadUrl.path(), m_fileSize, metadata);
        // --- END FIX ---
    } else {
        // Send the next chunk
        uploadChunk();
    }
}

void TusUploader::onUploadError(QNetworkReply::NetworkError code)
{
    QString errorMsg = m_reply->errorString();
    emit error(errorMsg);

    if (m_reply) {
        m_reply->deleteLater();
    }
    if (m_file && m_file->isOpen()) {
        m_file->close();
    }
}