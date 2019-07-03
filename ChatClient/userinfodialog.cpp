#include "userinfodialog.h"
#include "ui_userinfodialog.h"
#include <QDateTime>
UserInfoDialog::UserInfoDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::UserInfoDialog)
{
    ui->setupUi(this);
}

UserInfoDialog::~UserInfoDialog()
{
    delete ui;
}

void UserInfoDialog::SetUserInfo(chatpb::StructUser &user)
{
    this->setWindowTitle(QStringLiteral("用户信息"));
    ui->nick_label->setText(QString::fromStdString(user.username()));
    ui->regtime_label->setText(QDateTime::fromTime_t(user.regtime()).toString("yyyy-MM-dd HH:mm:ss"));
    ui->lastlogintime_label->setText(QDateTime::fromTime_t(user.lastlogintime()).toString("yyyy-MM-dd HH:mm:ss"));
    ui->lastleave_label->setText(QDateTime::fromTime_t(user.lastleavetime()).toString("yyyy-MM-dd HH:mm:ss"));
    ui->chatcnt_label->setText(QString::number(user.chatcnt()));
    ui->id_label->setText(QString::number(user.uid()));
}
