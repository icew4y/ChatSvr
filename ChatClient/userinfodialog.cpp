#include "userinfodialog.h"
#include "ui_userinfodialog.h"
#include <QDateTime>
#include <NetHelper.h>
#include <QMessageBox>
UserInfoDialog::UserInfoDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::UserInfoDialog)
{
    ui->setupUi(this);
    init();
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
    chatpb::C2SChatCnt req;
    req.set_nick(user.username());
    NET->send(req);
}

void UserInfoDialog::onChatCnt(std::shared_ptr<chatpb::S2CChatCnt> pMsg)
{
    if (pMsg->eret() != chatpb::Success) {
        QMessageBox::warning(this, QStringLiteral("警告"), QStringLiteral("获取聊天数量失败"), QMessageBox::Ok);
        return;
    }
    ui->chatcnt_label->setText(QString::number(pMsg->chatcnt()));
}

bool UserInfoDialog::init()
{
    this->setWindowTitle(QStringLiteral("用户信息"));
    auto pDispather = NET->GetDispather();
    pDispather->registerMsgCallBack<chatpb::S2CChatCnt>(std::bind(&UserInfoDialog::onChatCnt, this, std::placeholders::_1));
    return true;
}
