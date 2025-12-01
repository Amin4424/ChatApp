#ifndef USERLISTMANAGER_H
#define USERLISTMANAGER_H

#include <QObject>
#include <QListWidget>
#include <QLabel>
#include <QStackedWidget>

class UserListManager : public QObject
{
    Q_OBJECT
public:
    explicit UserListManager(QListWidget *listWidget, QLabel *countLabel, QStackedWidget *stackedWidget, QObject *parent = nullptr);

    void updateUsers(const QStringList &users);
    void updateSelection(const QString &currentUser, bool isPrivateChat);
    void updateUserCount(int count);

signals:
    void userSelected(const QString &userId);

private slots:
    void onItemClicked(QListWidgetItem *item);

private:
    QListWidget *m_listWidget;
    QLabel *m_countLabel;
    QStackedWidget *m_stackedWidget;
};

#endif // USERLISTMANAGER_H
