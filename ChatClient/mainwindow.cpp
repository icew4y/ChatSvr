﻿#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDebug>
#include <QDataStream>
#include <QtEndian>
#include "chat.pb.h"
#include <NetHelper.h>
#include <logindialog.h>
#include <functional>
#include <rankdialog.h>
#include <userinfodialog.h>
#include <QMessageBox>
#include <QDateTime>
#define NOTIFY_ALL_STRING QStringLiteral("群发")

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{

    ui->setupUi(this);
    init();
    LoginDialog loginDlg;
    if(loginDlg.exec() == QDialog::Accepted) {
        this->show();
    }
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::onListViewDBClicked(const QModelIndex &index)
{
    auto strNick = m_listModel.data(index).toString().toStdString();
    auto iter = m_mCurUsers.find(strNick);
    if (iter != m_mCurUsers.end()){
        UserInfoDialog infoDlg;
        infoDlg.SetUserInfo(iter->second);
        infoDlg.exec();
        infoDlg.close();
    } else if (strNick != NOTIFY_ALL_STRING.toStdString()){
        QMessageBox::warning(this, QStringLiteral("警告"), QStringLiteral("没有找到此用户的信息"), QMessageBox::Ok);
    }
}

bool MainWindow::init()
{
    NET->connectToHost("www.cxc233.com", 8000);
    ui->to_uid_label->setText(NOTIFY_ALL_STRING);
    ui->userList_listView->setModel(&m_listModel);
    ui->userList_listView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    connect(ui->userList_listView, &QListView::clicked, this, &MainWindow::onListViewClicked);
    connect(ui->userList_listView, &QListView::doubleClicked, this, &MainWindow::onListViewDBClicked);
    auto pDispather = NET->GetDispather();
    pDispather->registerMsgCallBack<chatpb::S2COnlineUsers>(std::bind(&MainWindow::onOnlineUsers, this, std::placeholders::_1));
    pDispather->registerMsgCallBack<chatpb::S2CLogin>(std::bind(&MainWindow::onLogin, this, std::placeholders::_1));
    pDispather->registerMsgCallBack<chatpb::S2CChat>(std::bind(&MainWindow::onChat, this, std::placeholders::_1));
    pDispather->registerMsgCallBack<chatpb::S2CStatusChange>(std::bind(&MainWindow::onUserStatusChange, this, std::placeholders::_1));
    connect(&m_timer, &QTimer::timeout, [this](){
        NET->send(chatpb::C2SHeartBeat());
        qDebug() << "heartbeat";
    });
    m_timer.setInterval(10*1000);
    m_timer.start();
    return true;
}

void MainWindow::onListViewClicked(const QModelIndex &index)
{
    ui->to_uid_label->setText(m_listModel.data(index).toString());
}

void MainWindow::onLogin(std::shared_ptr<chatpb::S2CLogin> pMsg)
{
    if (pMsg->eret() == chatpb::Success) {
        m_selfInfo = pMsg->info();
        ui->nick_label->setText(QString::fromStdString(m_selfInfo.username()));
        chatpb::C2SOnlineUsers req;
        std::map<int64_t, chatpb::S2CChat> mChats;
        for (int32_t i = 0; i < pMsg->lastchat_size(); i++) {
            auto &chatMsg = pMsg->lastchat(i);
            mChats[chatMsg.sendtime()] = chatMsg;
        }
        for (const auto& mIter : mChats){
            QString strTime = QDateTime::fromTime_t(mIter.first/1000000000).toString("yyyy-MM-dd HH:mm:ss");
            auto strMsg = QString("[%4]FROM:%1->%2\n%3\n").arg(mIter.second.fromnick().c_str()).arg(NOTIFY_ALL_STRING).arg(mIter.second.msg().c_str()).arg(strTime);
            ui->textBrowser->append(strMsg);
        }
        NET->send(req);
    }
}

void MainWindow::updateUserList()
{
    QStringList strList = {NOTIFY_ALL_STRING};
    for (const auto & mIter : m_mCurUsers) {
        strList << QString::fromStdString(mIter.first);
    }
    m_listModel.setStringList(strList);
}

void MainWindow::onUserStatusChange(std::shared_ptr<chatpb::S2CStatusChange> pMsg)
{
    auto strUserName = pMsg->user().username();
    if (pMsg->eret() == chatpb::Success) {
        if (pMsg->status() == chatpb::eStatus_Online) {
            m_mCurUsers[strUserName] = pMsg->user();
        } else if ( pMsg->status() == chatpb::eStatus_Offline) {
            m_mCurUsers.erase(strUserName);
            if (strUserName == ui->to_uid_label->text().toStdString()) {
                 ui->to_uid_label->setText(NOTIFY_ALL_STRING);
            }
        }
        updateUserList();
    }
}

void MainWindow::onOnlineUsers(std::shared_ptr<chatpb::S2COnlineUsers> pMsg)
{
    auto nCnt = pMsg->users_size();
    QStringList strList = {NOTIFY_ALL_STRING};
    std::map<std::string, chatpb::StructUser> emptyMap;
    m_mCurUsers.swap(emptyMap);
    for (int32_t i = 0; i < nCnt; i++){
        auto user = pMsg->users(i);
        m_mCurUsers[user.username()] = user;
    }
    updateUserList();
}

void MainWindow::onChat(std::shared_ptr<chatpb::S2CChat> pMsg)
{
    qDebug() << __FUNCTION__;
    QString strMsg;
    QString strTime = QDateTime::fromTime_t(pMsg->sendtime()/1000000000).toString("yyyy-MM-dd HH:mm:ss");
    if (pMsg->eret() == chatpb::NormalError) {
        auto tmpMsg = QStringLiteral("消息发送失败:") + QString("TO:%2,MSG:%1").arg(pMsg->msg().c_str(), pMsg->tonick().c_str());
        strMsg = "<font color=\"#FF0000\">" + tmpMsg + "</font>" + "\n";
        //收到成功标志
    } else if (pMsg->eret() == chatpb::Success) {
        //自己的发送成功
        if ( pMsg->fromuid() == m_selfInfo.uid()) {
            auto tmpMsg = QString("TO:%2,MSG:%1").arg(pMsg->msg().c_str(), pMsg->tonick().c_str());
            strMsg = "<font color=\"#00FF00\">" + tmpMsg + "</font>" + "\n";
         //别人的成功
        }else {
            if (pMsg->emsgtype() == chatpb::eMsg_All) {
                strMsg = QString("[%4]FROM:%1->%2\n%3\n").arg(pMsg->fromnick().c_str()).arg(NOTIFY_ALL_STRING).arg(pMsg->msg().c_str()).arg(strTime);
            } else if (pMsg->emsgtype() == chatpb::eMsg_One) {
                strMsg = QString("[%4]FROM:%1->%2\n%3\n").arg(pMsg->fromnick().c_str()).arg(m_selfInfo.username().c_str()).arg(pMsg->msg().c_str()).arg(strTime);
            }
        }
    } else if (pMsg->eret() == chatpb::UserOffline) {
        auto tmpMsg = QStringLiteral("消息发送失败:") + QString("[%3]TO:%2,MSG:%1,用户已下线！").arg(pMsg->msg().c_str(), pMsg->tonick().c_str(), strTime);
        strMsg = "<font color=\"#FF0000\">" + tmpMsg + "</font>" + "\n";
    }

    if (!strMsg.isEmpty()) {
        ui->textBrowser->append(strMsg);
    }
}

void MainWindow::on_rank_pushButton_clicked()
{
    RankDialog rankdlg;
    rankdlg.exec();
    rankdlg.close();
}



void MainWindow::on_send_pushButton_clicked()
{
    chatpb::C2SChat req;
    QString strTarget = ui->to_uid_label->text();
    QString strMsg = ui->textEdit->toPlainText();
    if (strMsg.isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("警告"), QStringLiteral("发送消息为空"), QMessageBox::Ok);
        return;
    }
    qDebug() << "send>>>>>>>>>>" << strTarget << m_selfInfo.username().c_str();
    if (strTarget.toStdString() == m_selfInfo.username()) {
        QMessageBox::warning(this, QStringLiteral("警告"), QStringLiteral("不能发送给自己"), QMessageBox::Ok);
        return;
    }
    if (strTarget == NOTIFY_ALL_STRING) {
        req.set_emsgtype(chatpb::eMsg_All);
        req.set_tonick(strTarget.toStdString());
    } else {
        auto iter = m_mCurUsers.find(strTarget.toStdString());
        if (iter != m_mCurUsers.end()){
            req.set_emsgtype(chatpb::eMsg_One);
            req.set_touid(iter->second.uid());
            req.set_tonick(strTarget.toStdString());
        } else {
            QMessageBox::warning(this, QStringLiteral("警告"), QStringLiteral("没有找到此用户的信息"), QMessageBox::Ok);
            return;
        }
    }
    req.set_msg(strMsg.toStdString());
    qDebug () << "send" << NET->send(req);
}
