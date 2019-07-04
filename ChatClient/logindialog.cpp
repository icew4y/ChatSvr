#include "logindialog.h"
#include "ui_logindialog.h"
#include <common.hpp>
#include <NetHelper.h>
#include <chat.pb.h>
#include <QDebug>
#include <functional>
#include <registerdialog.h>
#include <QMessageBox>
LoginDialog::LoginDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::LoginDialog)
{
    ui->setupUi(this);
    init();
}

LoginDialog::~LoginDialog()
{
    delete ui;
}

void LoginDialog::on_login_pushButton_clicked()
{

    chatpb::C2SLogin reqLogin;
    reqLogin.set_password(ui->pw_lineEdit->text().toStdString());
    reqLogin.set_username(ui->un_lineEdit->text().toStdString());
    qDebug() << reqLogin.descriptor()->name().c_str() << reqLogin.GetTypeName().c_str() << reqLogin.descriptor()->full_name().c_str();
    NET->send(reqLogin);
}

void LoginDialog::onLogin(std::shared_ptr<chatpb::S2CLogin> pMsg)
{
    QString strData;
    switch (pMsg->eret()) {
    case chatpb::Success:
       this->accept();
       break;
    case chatpb::UnError:
        strData = QStringLiteral("没有此账号");
        break;
    case chatpb::PwError:
        strData = QStringLiteral("密码错误");
        break;
    case chatpb::UserOnline:
        strData = QStringLiteral("账号已登陆");
        break;
    }
    if (!strData.isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("警告"), strData, QMessageBox::Ok);
    }
}

void LoginDialog::onRegister(std::shared_ptr<chatpb::S2CRegister> pMsg)
{
    qDebug() << __FUNCTION__ << pMsg->eret();
}

bool LoginDialog::init()
{
    this->setFixedSize(QSize(300, 200));
    this->setWindowTitle(QStringLiteral("登录"));
    auto pDispather = NET->GetDispather();
    pDispather->registerMsgCallBack<chatpb::S2CLogin>(std::bind(&LoginDialog::onLogin, this, std::placeholders::_1));
    pDispather->registerMsgCallBack<chatpb::S2CRegister>(std::bind(&LoginDialog::onRegister, this, std::placeholders::_1));
    return true;
}

void LoginDialog::on_reg_pushButton_clicked()
{
    RegisterDialog  regDlg(this);
    connect(&regDlg, &RegisterDialog::sigRegistered, this, &LoginDialog::onRegisterSuccessed);
    regDlg.exec();
    regDlg.close();
}

void LoginDialog::onRegisterSuccessed(QString strUn, QString strPw)
{
    ui->un_lineEdit->setText(strUn);
    ui->pw_lineEdit->setText(strPw);
}
