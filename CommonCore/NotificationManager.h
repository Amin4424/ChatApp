#ifndef NOTIFICATIONMANAGER_H
#define NOTIFICATIONMANAGER_H

#include <QObject>
#include <QSystemTrayIcon>
#include <QWidget>

class NotificationManager : public QObject
{
    Q_OBJECT

public:
    // Singleton access
    static NotificationManager& instance();

    // Initialize with the main window (needed for parenting Toasts and Tray Icon)
    void setup(QWidget *mainWindow);

    /**
     * @brief Decides whether to show a notification and which type (Tray vs Toast)
     * @param senderId The Raw ID of the person sending the message (e.g., "User #1 - 127.0.0.1")
     * @param messagePreview The text to display
     * @param currentOpenChatId The Raw ID of the chat currently open in the UI (e.g. "User #2 - ...")
     * @param isBroadcastMode Whether the UI is currently in "All Users" mode
     */
    void checkAndNotify(const QString &senderId,
                        const QString &messagePreview,
                        const QString &currentOpenChatId,
                        bool isBroadcastMode);
    void clearNotifications();
private:
    explicit NotificationManager(QObject *parent = nullptr);
    ~NotificationManager();

    // No copying
    NotificationManager(const NotificationManager&) = delete;
    NotificationManager& operator=(const NotificationManager&) = delete;

    QString cleanId(const QString &rawId) const;
    void showSystemTrayNotification(const QString &title, const QString &message);
    void showInAppToast(const QString &message);

    QWidget *m_mainWindow;
    QSystemTrayIcon *m_trayIcon;
};

#endif // NOTIFICATIONMANAGER_H
