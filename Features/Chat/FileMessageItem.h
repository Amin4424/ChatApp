#ifndef FILEMESSAGEITEM_H
#define FILEMESSAGEITEM_H

#include "BaseMessageItem.h"
#include <QString>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QProgressBar>
#include <QPushButton>

class FileMessageItem : public BaseMessageItem
{
    Q_OBJECT

public:
    explicit FileMessageItem(const QString &fileName, qint64 fileSize, const QString &fileUrl, const QString &senderInfo = "", QWidget *parent = nullptr);
    ~FileMessageItem();

protected:
    void setupUI() override;

private slots:
    void downloadFile();
    void onDownloadProgress(qint64 bytesReceived, qint64 bytesTotal);
    void onDownloadFinished();
    void onDownloadError(QNetworkReply::NetworkError code);
    void openFile();

private:
    void setupFileUI(const QString &fileName, qint64 fileSize);
    QPixmap getFileIconPixmap(const QString &extension);
    QString formatSize(qint64 bytes);

    QString m_fileUrl;
    QString m_fileName;
    qint64 m_fileSize;
    QNetworkAccessManager *m_networkManager;
    QNetworkReply *m_reply;
    QPushButton *m_downloadButton;
    QProgressBar *m_progressBar;
};

#endif // FILEMESSAGEITEM_H