#include "CryptoManager.h"
#include <openssl/err.h>
#include <QDebug>

CryptoManager::CryptoManager(QObject *parent)
    : QObject(parent)
{
    // Initialize OpenSSL
    OpenSSL_add_all_algorithms();
    ERR_load_crypto_strings();
}

CryptoManager::~CryptoManager()
{
    // Cleanup OpenSSL
    EVP_cleanup();
    ERR_free_strings();
}

// Simplified static methods for easy use
QString CryptoManager::encryptMessage(const QString &message)
{
    // Convert QString to UTF-8 QByteArray
    QByteArray plaintext = message.toUtf8();

    // Generate key and IV
    QByteArray key = generateAES256Key();
    QByteArray iv = generateGCMIV();

    if (key.isEmpty() || iv.isEmpty()) {
        qWarning() << "Failed to generate key or IV";
        return QString();
    }

    QByteArray ciphertext, tag;

    // Encrypt the message
if (!encryptAESGCM256(plaintext, key, ciphertext, iv, tag)) {
        qWarning() << "Encryption failed";
    return QString();
        }

     QByteArray combinedData;
    combinedData.append(iv);
    combinedData.append(tag);
    combinedData.append(ciphertext);


                 // Convert to base64 string for easy string handling
return QString(combinedData.toBase64());
    }

QString CryptoManager::decryptMessage(const QString &encryptedData)
{
    // Convert base64 string back to QByteArray
    QByteArray combinedData = QByteArray::fromBase64(encryptedData.toUtf8());


if (combinedData.size() < 28) {
     qWarning() << "Encrypted data too small. Minimum 28 bytes required.";
     return QString();
}
    // Get the pre-shared secret key
     QByteArray key = generateAES256Key();

     // Extract components
     QByteArray iv = combinedData.mid(0, 12);
     QByteArray tag = combinedData.mid(12, 16);
     QByteArray ciphertext = combinedData.mid(28);
     QByteArray plaintext;
     // Decrypt the message
     if (!decryptAESGCM256(ciphertext, key, iv, tag, plaintext)) {
    qWarning() << "Decryption failed";
        return QString();
    }

    // Convert decrypted bytes back to QString
    return QString::fromUtf8(plaintext);
}

QFile CryptoManager::encryptFile(const QFile &file)
{
    // TODO: Implement full file encryption if needed
    return QFile();
}

QFile CryptoManager::decryptFile(const QFile &file)
{
    // TODO: Implement full file decryption if needed
    return QFile();
}

bool CryptoManager::encryptChunk(const QByteArray &plainChunk,
                                const QByteArray &key,
                                QByteArray &encryptedChunk,
                                ChunkMetadata &metadata)
{
    if (plainChunk.isEmpty()) {
        qWarning() << "Cannot encrypt empty chunk";
        return false;
    }

    // Generate a unique IV for this chunk
    metadata.iv = generateGCMIV();
    if (metadata.iv.isEmpty()) {
        qWarning() << "Failed to generate IV for chunk";
        return false;
    }

    metadata.originalSize = plainChunk.size();

    // Encrypt the chunk using AES-GCM-256
    QByteArray tag;
    if (!encryptAESGCM256(plainChunk, key, encryptedChunk, metadata.iv, tag)) {
        qWarning() << "Failed to encrypt chunk";
        return false;
    }

    metadata.tag = tag;
    metadata.encryptedSize = encryptedChunk.size();

    qDebug() << "Encrypted chunk" << metadata.chunkIndex 
             << "- Original size:" << metadata.originalSize 
             << "Encrypted size:" << metadata.encryptedSize;

    return true;
}

bool CryptoManager::decryptChunk(const QByteArray &encryptedChunk,
                                const QByteArray &key,
                                const ChunkMetadata &metadata,
                                QByteArray &plainChunk)
{
    if (encryptedChunk.isEmpty()) {
        qWarning() << "Cannot decrypt empty chunk";
        return false;
    }

    if (metadata.iv.size() != 12) {
        qWarning() << "Invalid IV size for chunk" << metadata.chunkIndex;
        return false;
    }

    if (metadata.tag.size() != 16) {
        qWarning() << "Invalid tag size for chunk" << metadata.chunkIndex;
        return false;
    }

    // Decrypt the chunk using AES-GCM-256
    if (!decryptAESGCM256(encryptedChunk, key, metadata.iv, metadata.tag, plainChunk)) {
        qWarning() << "Failed to decrypt chunk" << metadata.chunkIndex;
        return false;
    }

    qDebug() << "Decrypted chunk" << metadata.chunkIndex 
             << "- Encrypted size:" << encryptedChunk.size() 
             << "Decrypted size:" << plainChunk.size();

    return true;
}

bool CryptoManager::encryptAESGCM256(const QByteArray &plaintext,
                                   const QByteArray &key,
                                   QByteArray &ciphertext,
                                   QByteArray &iv,
                                   QByteArray &tag)
{
    if (key.size() != 32) {
        qWarning() << "AES-GCM-256 requires a 32-byte key";
        return false;
    }

    if (iv.size() != 12) {
        qWarning() << "AES-GCM requires a 12-byte IV";
        return false;
    }

    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    if (!ctx) {
        qWarning() << "Failed to create cipher context:" << getOpenSSLErrorString();
        return false;
    }

    // Initialize encryption context
    if (EVP_EncryptInit_ex(ctx, EVP_aes_256_gcm(), nullptr, nullptr, nullptr) != 1) {
        qWarning() << "Failed to initialize AES-GCM encryption:" << getOpenSSLErrorString();
        EVP_CIPHER_CTX_free(ctx);
        return false;
    }

    // Set IV length for GCM
    if (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN, iv.size(), nullptr) != 1) {
        qWarning() << "Failed to set IV length:" << getOpenSSLErrorString();
        EVP_CIPHER_CTX_free(ctx);
        return false;
    }

    // Initialize key and IV
    if (EVP_EncryptInit_ex(ctx, nullptr, nullptr,
                          reinterpret_cast<const unsigned char*>(key.data()),
                          reinterpret_cast<const unsigned char*>(iv.data())) != 1) {
        qWarning() << "Failed to set key and IV:" << getOpenSSLErrorString();
        EVP_CIPHER_CTX_free(ctx);
        return false;
    }

    // Prepare output buffer
    ciphertext.resize(plaintext.size());
    int outlen = 0;

    // Encrypt the plaintext
    if (EVP_EncryptUpdate(ctx,
                         reinterpret_cast<unsigned char*>(ciphertext.data()),
                         &outlen,
                         reinterpret_cast<const unsigned char*>(plaintext.data()),
                         plaintext.size()) != 1) {
        qWarning() << "Failed to encrypt data:" << getOpenSSLErrorString();
        EVP_CIPHER_CTX_free(ctx);
        return false;
    }

    int total_len = outlen;

    // Finalize encryption
    if (EVP_EncryptFinal_ex(ctx, reinterpret_cast<unsigned char*>(ciphertext.data()) + total_len, &outlen) != 1) {
        qWarning() << "Failed to finalize encryption:" << getOpenSSLErrorString();
        EVP_CIPHER_CTX_free(ctx);
        return false;
    }
    total_len += outlen;

    // Get the authentication tag
    tag.resize(16); // AES-GCM tag is always 16 bytes
    if (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_GET_TAG, 16,
                           reinterpret_cast<unsigned char*>(tag.data())) != 1) {
        qWarning() << "Failed to get authentication tag:" << getOpenSSLErrorString();
        EVP_CIPHER_CTX_free(ctx);
        return false;
    }

    // Resize ciphertext to actual size
    ciphertext.resize(total_len);

    EVP_CIPHER_CTX_free(ctx);
    return true;
}

bool CryptoManager::decryptAESGCM256(const QByteArray &ciphertext,
                                   const QByteArray &key,
                                   const QByteArray &iv,
                                   const QByteArray &tag,
                                   QByteArray &plaintext)
{
    if (key.size() != 32) {
        qWarning() << "AES-GCM-256 requires a 32-byte key";
        return false;
    }

    if (iv.size() != 12) {
        qWarning() << "AES-GCM requires a 12-byte IV";
        return false;
    }

    if (tag.size() != 16) {
        qWarning() << "AES-GCM requires a 16-byte authentication tag";
        return false;
    }

    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    if (!ctx) {
        qWarning() << "Failed to create cipher context:" << getOpenSSLErrorString();
        return false;
    }

    // Initialize decryption context
    if (EVP_DecryptInit_ex(ctx, EVP_aes_256_gcm(), nullptr, nullptr, nullptr) != 1) {
        qWarning() << "Failed to initialize AES-GCM decryption:" << getOpenSSLErrorString();
        EVP_CIPHER_CTX_free(ctx);
        return false;
    }

    // Set IV length for GCM
    if (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN, iv.size(), nullptr) != 1) {
        qWarning() << "Failed to set IV length:" << getOpenSSLErrorString();
        EVP_CIPHER_CTX_free(ctx);
        return false;
    }

    // Initialize key and IV
    if (EVP_DecryptInit_ex(ctx, nullptr, nullptr,
                          reinterpret_cast<const unsigned char*>(key.data()),
                          reinterpret_cast<const unsigned char*>(iv.data())) != 1) {
        qWarning() << "Failed to set key and IV:" << getOpenSSLErrorString();
        EVP_CIPHER_CTX_free(ctx);
        return false;
    }

    // Prepare output buffer
    plaintext.resize(ciphertext.size());
    int outlen = 0;

    // Decrypt the ciphertext
    if (EVP_DecryptUpdate(ctx,
                         reinterpret_cast<unsigned char*>(plaintext.data()),
                         &outlen,
                         reinterpret_cast<const unsigned char*>(ciphertext.data()),
                         ciphertext.size()) != 1) {
        qWarning() << "Failed to decrypt data:" << getOpenSSLErrorString();
        EVP_CIPHER_CTX_free(ctx);
        return false;
    }

    int total_len = outlen;

    // Set the expected authentication tag
    if (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_TAG, tag.size(),
                           reinterpret_cast<unsigned char*>(const_cast<QByteArray&>(tag).data())) != 1) {
        qWarning() << "Failed to set authentication tag:" << getOpenSSLErrorString();
        EVP_CIPHER_CTX_free(ctx);
        return false;
    }

    // Finalize decryption and verify authentication tag
    int ret = EVP_DecryptFinal_ex(ctx, reinterpret_cast<unsigned char*>(plaintext.data()) + total_len, &outlen);

    if (ret > 0) {
        total_len += outlen;
        plaintext.resize(total_len);
        EVP_CIPHER_CTX_free(ctx);
        return true;
    } else {
        // Authentication failed
        qWarning() << "Authentication failed - data may be corrupted or tampered with";
        EVP_CIPHER_CTX_free(ctx);
        return false;
    }
}

QByteArray CryptoManager::generateAES256Key()
{
    // QByteArray key(32, 0); // 256 bits = 32 bytes

    // if (RAND_bytes(reinterpret_cast<unsigned char*>(key.data()), key.size()) != 1) {
    //     qWarning() << "Failed to generate random key:" << getOpenSSLErrorString();
    //     return QByteArray();
    // }

    const QByteArray b64_key = "llZem1M53EBHauupjh4z/Biv1FR0vu7cXv8sqKjMpTw=";

    QByteArray key = QByteArray::fromBase64(b64_key);
    qDebug() << "\n\n\n\n key :     "
             << key.toBase64() << "\n\n\n\n";
    return key;
}

QByteArray CryptoManager::generateGCMIV()
{
    QByteArray iv(12, 0); // GCM recommended IV size is 96 bits = 12 bytes

    if (RAND_bytes(reinterpret_cast<unsigned char*>(iv.data()), iv.size()) != 1) {
        qWarning() << "Failed to generate random IV:" << getOpenSSLErrorString();
        return QByteArray();
    }

    return iv;
}

QString CryptoManager::bytesToHex(const QByteArray &bytes)
{
    return bytes.toHex().toUpper();
}

QByteArray CryptoManager::hexToBytes(const QString &hex)
{
    return QByteArray::fromHex(hex.toUtf8());
}

QString CryptoManager::getOpenSSLErrorString()
{
    char buffer[256];
    ERR_error_string_n(ERR_get_error(), buffer, sizeof(buffer));
    return QString(buffer);
}

QByteArray CryptoManager::serializeMetadata(const QVector<ChunkMetadata> &metadataList)
{
    QByteArray result;
    QDataStream stream(&result, QIODevice::WriteOnly);
    stream.setVersion(QDataStream::Qt_5_15);

    // Write number of chunks
    stream << static_cast<qint32>(metadataList.size());

    // Write each chunk's metadata
    for (const ChunkMetadata &meta : metadataList) {
        stream << meta.iv;
        stream << meta.tag;
        stream << meta.chunkIndex;
        stream << meta.originalSize;
        stream << meta.encryptedSize;
    }

    qDebug() << "Serialized metadata for" << metadataList.size() << "chunks, total size:" << result.size() << "bytes";
    return result;
}

QVector<ChunkMetadata> CryptoManager::deserializeMetadata(const QByteArray &data)
{
    QVector<ChunkMetadata> result;
    QDataStream stream(data);
    stream.setVersion(QDataStream::Qt_5_15);

    qint32 count = 0;
    stream >> count;

    if (count < 0 || count > 100000) { // Sanity check
        qWarning() << "Invalid chunk count in metadata:" << count;
        return result;
    }

    result.reserve(count);

    for (qint32 i = 0; i < count; i++) {
        ChunkMetadata meta;
        stream >> meta.iv;
        stream >> meta.tag;
        stream >> meta.chunkIndex;
        stream >> meta.originalSize;
        stream >> meta.encryptedSize;

        if (stream.status() != QDataStream::Ok) {
            qWarning() << "Error deserializing metadata at chunk" << i;
            return QVector<ChunkMetadata>();
        }

        result.append(meta);
    }

    qDebug() << "Deserialized metadata for" << result.size() << "chunks";
    return result;
}
