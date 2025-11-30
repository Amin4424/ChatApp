#ifndef CHATHEADER_H
#define CHATHEADER_H

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QHBoxLayout>
#include <QVBoxLayout>

class ChatHeader : public QWidget
{
    Q_OBJECT

public:
    explicit ChatHeader(QWidget *parent = nullptr);

    void setUserName(const QString &name);
    void setUserIP(const QString &ip);
    void setAvatar(const QPixmap &avatar);

protected:
    void paintEvent(QPaintEvent *event) override;

signals:
    void searchClicked();
    void deleteClicked();
    void moreClicked();

private:
    void setupUI();
    QPixmap getRoundedPixmap(const QPixmap &src, int radius);

    QLabel *m_avatarLabel;
    QLabel *m_nameLabel;
    QLabel *m_ipLabel;
    
    QPushButton *m_searchBtn;
    QPushButton *m_deleteBtn;
    QPushButton *m_moreBtn;
};

#endif // CHATHEADER_H
