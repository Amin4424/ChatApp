#ifndef CONNECTPAGE_H
#define CONNECTPAGE_H

#include <QWidget>

namespace Ui {
class ConnectPage;
}

class ConnectPage : public QWidget
{
    Q_OBJECT

public:
    explicit ConnectPage(QWidget *parent = nullptr);
    ~ConnectPage();

signals:
    void connectClient(QString ip);
    void createServer();
private slots:

    void on_connectBtn_clicked();

    void on_createServerBtn_clicked();


private:
    Ui::ConnectPage *ui;
};

#endif // CONNECTPAGE_H
