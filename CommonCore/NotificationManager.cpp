#include "NotificationManager.h"
#include "ToastNotification.h" // Your existing Toast class
#include <QApplication>
#include <QGuiApplication>
#include <QProcess>
#include <QIcon>
#include <QDebug>
#include <QStyle>
NotificationManager& NotificationManager::instance()
{
    static NotificationManager _instance;
    return _instance;
}

NotificationManager::NotificationManager(QObject *parent)
    : QObject(parent)
    , m_mainWindow(nullptr)
    , m_trayIcon(nullptr)
{
}

NotificationManager::~NotificationManager()
{
    // Parent child system will clean up tray icon if parented to MainWindow
}

void NotificationManager::setup(QWidget *mainWindow)
{
    m_mainWindow = mainWindow;

    // Initialize System Tray Icon
    if (!m_trayIcon && QSystemTrayIcon::isSystemTrayAvailable()) {
        m_trayIcon = new QSystemTrayIcon(this);

        // Try to load your app icon
        QIcon icon("assets/Avatar.png");
        if (icon.isNull()) {
            // Fallback if asset not found
            icon = QApplication::style()->standardIcon(QStyle::SP_ComputerIcon);
        }

        m_trayIcon->setIcon(icon);
        m_trayIcon->setVisible(true);
    }
}

void NotificationManager::checkAndNotify(const QString &senderId,
                                         const QString &messagePreview,
                                         const QString &currentOpenChatId,
                                         bool isBroadcastMode)
{
    QString cleanSender = cleanId(senderId);
    QString cleanTarget = cleanId(currentOpenChatId);

    // Special case for Client: If I am the client, I am likely chatting with "Server".
    // If the sender is "Server", cleanTarget should match.
    // If the sender is "User #2" (relayed via Server), it depends on your UI.
    // Assuming Client UI has only ONE view (the main chat):
    // Then 'cleanTarget' is effectively *always* the active chat.

    bool isChattingWithUser = (!isBroadcastMode && cleanTarget == cleanSender);

    // If we are a Client app, we might treat "Server" messages as always "current chat"
    // if the window is focused.

    bool isWindowActive = (QApplication::activeWindow() != nullptr);

    // LOGIC:
    // 1. If window is NOT active (minimized/background) -> Always Notify (Tray)
    // 2. If window IS active:
    //    a. If chatting with this user -> No Notify
    //    b. If NOT chatting with this user -> In-App Toast

    if (!isWindowActive) {
        showSystemTrayNotification(QString("Message from %1").arg(cleanSender), messagePreview);
    }
    else if (!isChattingWithUser) {
        // Only show toast if we are looking at a DIFFERENT chat
        // (For single-view clients, this might never happen, which is correct)
        showInAppToast(QString("%1: %2").arg(cleanSender, messagePreview));
    }
}

void NotificationManager::clearNotifications()
{
    // Clear system tray messages
    if (m_trayIcon) {
        // Hide any lingering tooltip/message
        m_trayIcon->hide();
        m_trayIcon->show();
    }
    
#ifdef Q_OS_LINUX
    // Clear Unity badge on Linux using DBus
    QProcess::execute("dbus-send", QStringList() 
        << "--session"
        << "--dest=org.ayatana.bamf"
        << "--type=method_call"
        << "/org/ayatana/bamf/matcher"
        << "org.ayatana.bamf.matcher.ApplicationForXid"
        << "uint32:0");
#endif

}

QString NotificationManager::cleanId(const QString &rawId) const
{
    if (rawId.isEmpty()) return "";
    return rawId.split(" - ").first().trimmed();
}

void NotificationManager::showSystemTrayNotification(const QString &title, const QString &message)
{
    if (m_trayIcon && m_trayIcon->isSystemTrayAvailable()) {
        m_trayIcon->showMessage(title, message, QSystemTrayIcon::Information, 3000);
    } else {

#ifdef Q_OS_LINUX
        // Fallback for Linux/Ubuntu if Tray is missing
        QString cmd = QString("notify-send \"%1\" \"%2\"").arg(title, message);
        system(cmd.toUtf8().constData());
#endif
    }
}

void NotificationManager::showInAppToast(const QString &message)
{
    if (m_mainWindow) {
        ToastNotification::showMessage(m_mainWindow, message, 3000);
    }
}
