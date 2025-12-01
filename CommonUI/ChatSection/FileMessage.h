#ifndef FILEMESSAGE_H
#define FILEMESSAGE_H

#include "AttachmentMessage.h"
#include "InfoCard.h"
#include <QString>
#include <QIcon>

class QEnterEvent;

class QToolButton;
class QMenu;
class QAction;

class FileMessage : public AttachmentMessage
{
    Q_OBJECT

public:
    explicit FileMessage(const QString &fileName,
                         qint64 fileSize,
                         const QString &fileUrl,
                         const QString &senderInfo,
                         MessageDirection direction,
                         const QString &serverHost = "localhost",
                         const QString &timestamp = QString(),
                         QWidget *parent = nullptr);
    ~FileMessage();

    // Public methods to control state
    void markAsSent();  // Change from pending (🕒) to sent (✓)
    void setTransferProgress(qint64 bytes, qint64 total);  // Update progress during upload/download
    bool isSent() const { return m_isSent; }  // Check if message was sent
    void setTimestamp(const QString &timestamp);

signals:
    void deleteRequested(FileMessage *item);
    void sizeChanged();

protected:
    void setupUI() override;
    
    // Override virtual methods from base class
    void setIdleState() override;
    void setInProgressState(qint64 bytes, qint64 total) override;
    void setCompletedState() override;
    void onClicked() override;

private:
    // --- Member variables ---
    InfoCard* m_infoCard;
    bool m_isSent = false;  // Track if message was sent (for Completed_Sent state)
    QToolButton *m_moreButton = nullptr;
    QMenu *m_actionMenu = nullptr;
    QAction *m_deleteAction = nullptr;
    bool m_menuVisible = false;

    void initializeActionButton();
    void applyActionMenuStyle();
    void showActionMenu();
    QIcon deleteIcon(bool emphasizeGlobal) const;

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    void enterEvent(QEnterEvent *event) override;
#else
    void enterEvent(QEvent *event) override;
#endif
    void leaveEvent(QEvent *event) override;
};

#endif // FILEMESSAGE_H