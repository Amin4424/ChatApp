#ifndef CRYPTOMANAGER_H
#define CRYPTOMANAGER_H

#include <openssl/aes.h>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <QObject>
#include <QByteArray>
#include <QString>
#include <QFile>
#include <QVector>

// Structure to hold chunk encryption metadata
struct ChunkMetadata {
    QByteArray iv;    // 12 bytes for GCM
    QByteArray tag;   // 16 bytes for GCM authentication tag
    qint64 chunkIndex; // Sequential chunk number
    qint64 originalSize; // Size before encryption
    qint64 encryptedSize; // Size after encryption
};

class CryptoManager : public QObject
{
    Q_OBJECT

public:
    explicit CryptoManager(QObject *parent = nullptr);
    ~CryptoManager();

    // Simplified static methods for easy use
    static QString encryptMessage(const QString &message);
    static QString decryptMessage(const QString &encryptedData);

    static QFile encryptFile(const QFile &file);
    static QFile decryptFile(const QFile &file);

    // Chunk-based encryption for streaming (TUS uploads)
    static bool encryptChunk(const QByteArray &plainChunk,
                           const QByteArray &key,
                           QByteArray &encryptedChunk,
                           ChunkMetadata &metadata);

    static bool decryptChunk(const QByteArray &encryptedChunk,
                           const QByteArray &key,
                           const ChunkMetadata &metadata,
                           QByteArray &plainChunk);

    // AES-GCM-256 methods (for advanced usage)
    static bool encryptAESGCM256(const QByteArray &plaintext,
                                const QByteArray &key,
                                QByteArray &ciphertext,
                                QByteArray &iv,
                                QByteArray &tag);

    static bool decryptAESGCM256(const QByteArray &ciphertext,
                                const QByteArray &key,
                                const QByteArray &iv,
                                const QByteArray &tag,
                                QByteArray &plaintext);

    // Key generation
    static QByteArray generateAES256Key();
    static QByteArray generateGCMIV();

    // Utility methods
    static QString bytesToHex(const QByteArray &bytes);
    static QByteArray hexToBytes(const QString &hex);

    // Metadata serialization for storage/transmission
    static QByteArray serializeMetadata(const QVector<ChunkMetadata> &metadataList);
    static QVector<ChunkMetadata> deserializeMetadata(const QByteArray &data);

private:
    // Helper method for OpenSSL error handling
    static QString getOpenSSLErrorString();
};

#endif // CRYPTOMANAGER_H
