#include "rankdialog.h"
#include "ui_rankdialog.h"
#include "NetHelper.h"
#include <QTableWidgetItem>
RankDialog::RankDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::RankDialog)
{
    ui->setupUi(this);
    init();
}

RankDialog::~RankDialog()
{
    delete ui;
}

void RankDialog::onChatCntTopUsers(std::shared_ptr<chatpb::S2CChatCntTopUsers> pMsg)
{
    ui->tableWidget->clear();
    QStringList header = {QStringLiteral("昵称"),QStringLiteral("聊天次数")};
    ui->tableWidget->setColumnCount(2);
    ui->tableWidget->setHorizontalHeaderLabels(header);
    ui->tableWidget->setEditTriggers(QTableWidget::NoEditTriggers);
    std::map<int32_t, chatpb::StructUser> mUsers;
    for (int i = 0; i < pMsg->users_size(); i++) {
        auto user = pMsg->users(i);
        mUsers[user.chatcnt()] = user;
    }
    for (const auto& mIter : mUsers ) {
        ui->tableWidget->insertRow(0);
        auto user = mIter.second;
        ui->tableWidget->setItem(0, 0, new QTableWidgetItem(QString::fromStdString(user.username())));
        ui->tableWidget->setItem(0, 1, new QTableWidgetItem(QString::number(user.chatcnt())));
    }
}

bool RankDialog::init()
{
    this->setWindowTitle(QStringLiteral("聊天排行榜"));
    auto pDispather = NET->GetDispather();
    pDispather->registerMsgCallBack<chatpb::S2CChatCntTopUsers>(std::bind(&RankDialog::onChatCntTopUsers, this, std::placeholders::_1));
    chatpb::C2SChatCntTopUsers req;
    req.set_cnt(10);
    req.set_start(1);
    NET->send(req);
    return true;
}
