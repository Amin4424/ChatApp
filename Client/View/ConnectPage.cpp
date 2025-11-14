#include "View/ConnectPage.h"
#include "ui_ConnectPage.h"

ConnectPage::ConnectPage(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::ConnectPage)
{
    ui->setupUi(this);
}

ConnectPage::~ConnectPage()
{
    delete ui;
}


void ConnectPage::on_connectBtn_clicked()
{
    QString ip = ui->ipTxt->toPlainText();
    if (!ip.isEmpty()) {
        emit connectClient(ip);
    }
}


void ConnectPage::on_createServerBtn_clicked()
{
    emit createServer();
}

