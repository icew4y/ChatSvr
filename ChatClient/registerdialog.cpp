#include "registerdialog.h"
#include "ui_registerdialog.h"
#include <common.hpp>
#include <NetHelper.h>
#include <QDebug>
#include <functional>
#include <QMessageBox>
RegisterDialog::RegisterDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::RegisterDialog)
{
    ui->setupUi(this);
    init();
}

RegisterDialog::~RegisterDialog()
{
    delete ui;
}

void RegisterDialog::on_cancel_pushButton_clicked()
{
    this->close();
}

void RegisterDialog::on_reg_pushButton_clicked()
{
    chatpb::C2SRegister reqRegister;
    reqRegister.set_password(ui->pw_lineEdit->text().toStdString());
    reqRegister.set_username(ui->un_lineEdit->text().toStdString());
    NET->send(reqRegister);
}

bool RegisterDialog::init()
{
    this->setFixedSize(QSize(300, 200));
    this->setWindowTitle(QStringLiteral("注册"));
    auto pDispather = NET->GetDispather();
    std::function<void()> func;
    pDispather->registerMsgCallBack<chatpb::S2CRegister>(std::bind(&RegisterDialog::onRegister, this, std::placeholders::_1));
    return true;
}

void RegisterDialog::onRegister(std::shared_ptr<chatpb::S2CRegister> pMsg)
{
    qDebug() << pMsg->eret();
    QString strData;
    switch (pMsg->eret()) {
    case chatpb::Success:
       emit sigRegistered(ui->un_lineEdit->text(), ui->pw_lineEdit->text());
       break;
    case chatpb::UnExistError:
        strData = QStringLiteral("账号已存在");
        break;
    case chatpb::UnNotStandard:
        strData = QStringLiteral("账号不符合规则");
        break;
    case chatpb::PwNotStandard:
        strData = QStringLiteral("密码不符合规则");
        break;
    default:
        strData = QStringLiteral("其他错误");
    }
    if (!strData.isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("警告"), strData, QMessageBox::Ok);
    } else {
        QMessageBox::information(this, QStringLiteral("提示"), QStringLiteral("注册成功"), QMessageBox::Ok);
        this->accept();
    }
}
