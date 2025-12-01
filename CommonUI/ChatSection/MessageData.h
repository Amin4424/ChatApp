#ifndef MESSAGEDATA_H
#define MESSAGEDATA_H

#include <QString>
#include <QDateTime>

// Simple struct for file information
struct FileInfo {
    QString fileName;
    qint64 fileSize = 0;
    QString fileUrl;
    
    FileInfo() = default;
    FileInfo(const QString &name, qint64 size, const QString &url)
        : fileName(name), fileSize(size), fileUrl(url) {}
};

struct VoiceInfo {
    QString url;
    int duration = 0;
    QVector<qreal> waveform; // Real waveform data
    
    VoiceInfo() = default;
    VoiceInfo(const QString &voiceUrl, int durationSeconds, const QVector<qreal> &waveformData = QVector<qreal>())
        : url(voiceUrl), duration(durationSeconds), waveform(waveformData) {}
};

// Main message data class
class MessageData {
public:
    enum SenderType {
        User_Me,      // "You"
        User_Other    // "Server" or "User #1"
    };

    QString text;
    QString senderName;
    QString timestamp; // Just "hh:mm"
    SenderType senderType = User_Other;
    bool isFileMessage = false;
    bool isVoiceMessage = false;
    FileInfo fileInfo;
    VoiceInfo voiceInfo;
    int databaseId = -1;
    bool isEdited = false;
    
    MessageData() = default;
    
    // Constructor for text messages
    MessageData(const QString &messageText, const QString &sender, 
                SenderType type, const QString &time)
        : text(messageText), senderName(sender), timestamp(time), 
          senderType(type), isFileMessage(false) {}
    
    // Constructor for file messages
    MessageData(const FileInfo &file, const QString &sender, 
                SenderType type, const QString &time)
        : senderName(sender), timestamp(time), senderType(type), 
          isFileMessage(true), fileInfo(file) {}
};

#endif // MESSAGEDATA_H
