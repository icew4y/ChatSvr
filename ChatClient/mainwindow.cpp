#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDebug>
#include <QDataStream>
#include <QtEndian>
#include "chat.pb.h"
#include <NetHelper.h>
#include <logindialog.h>
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

bool MainWindow::init()
{
    NET->connectToHost("ipad", 8000);
//    if (!m_pSocket){
//        m_pSocket = std::make_shared<QTcpSocket>();
//        connect(m_pSocket.get(), &QTcpSocket::readyRead, this, &MainWindow::onRead);
//        m_mCb[eProto_UserId] = std::bind(&MainWindow::onUserID, this, std::placeholders::_1);
//        m_mCb[eProto_UserList] = std::bind(&MainWindow::onUserList, this, std::placeholders::_1);
//        m_mCb[eProto_Chat] = std::bind(&MainWindow::onChat, this, std::placeholders::_1);
//    }
    ui->to_uid_label->setText("广播");
    ui->userList_listView->setModel(&m_listModel);
    ui->userList_listView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    connect(ui->userList_listView, &QListView::clicked, this, &MainWindow::onListViewClicked);
    return true;
}

void MainWindow::protoDispath(uint32_t nProtoId, const std::string &strMsg)
{
    auto mIter = m_mCb.find(static_cast<ProtoId>(nProtoId));
    if (mIter != m_mCb.end()){
        mIter->second(strMsg);
    }else{
        qDebug() << "unknow proto" << nProtoId;
    }
}

void MainWindow::onUserID(const std::string & strData)
{
    ui->uid_label->setText(QString::fromStdString(strData));
    m_nSelfId = std::atoi(strData.c_str());
}

void MainWindow::onUserList(const std::string &strData)
{
    qDebug() << strData.c_str();
    QJsonParseError jsonError;
    QJsonDocument doucment = QJsonDocument::fromJson(strData.c_str(), &jsonError);
    if (!doucment.isNull() && (jsonError.error == QJsonParseError::NoError)) {
        if (doucment.isArray()) {
            auto jsonArr = doucment.array();
            QStringList strList = {QString("广播")};
            for (auto jsonData : jsonArr){
                strList << QString::number(jsonData.toObject().value("UId").toInt());
            }
            m_listModel.setStringList(strList);
        }
    }else{
        qDebug() << jsonError.errorString();
    }
}

void MainWindow::onChat(const std::string &strData)
{
    qDebug() << strData.c_str();
    QJsonParseError jsonError;
    QJsonDocument doucment = QJsonDocument::fromJson(strData.c_str(), &jsonError);
    if (!doucment.isNull() && (jsonError.error == QJsonParseError::NoError)) {
        if (doucment.isObject()) {
            qDebug() << "chat";
            auto jsonObj = doucment.object();
            auto nTarget = jsonObj.value("Target").toInt();
            auto nFrom = jsonObj.value("From").toInt();
            auto strData = jsonObj.value("Data").toString();
            QString strMsg;
            if (nTarget == NOTITY_ALL){
                strMsg = QString("FROM:%1->%2\n%3\n").arg(nFrom).arg("广播").arg(strData);
            }else{
                strMsg = QString("FROM:%1->%2\n%3\n").arg(nFrom).arg(nTarget).arg(strData);
            }
            ui->textBrowser->append(strMsg);
        }
    }else{
        qDebug() << jsonError.errorString();
    }
}

void MainWindow::on_send_pushButton_clicked()
{
    if (m_pSocket){
        int32_t nTargetId = 0;
        QString strTarget = ui->to_uid_label->text();
        if (strTarget == "广播"){
            nTargetId = NOTITY_ALL;
        }else{
            nTargetId = strTarget.toInt();
        }
        QJsonObject json;
        json.insert("Target", nTargetId);
        json.insert("From", m_nSelfId);
        json.insert("Data", ui->textEdit->toPlainText());
        QJsonDocument document;
        document.setObject(json);
        QByteArray byteArray = document.toJson(QJsonDocument::Compact);
        QString strJson(byteArray);
        MsgSt stMsg;
        stMsg.nUid = m_nSelfId;
        stMsg.nPId = eProto_Chat;
        stMsg.strMsg = strJson.toStdString();
        qDebug() << m_pSocket->write(QString::fromStdString(stMsg.GetData()).toUtf8()+"\n");
    }
}

void MainWindow::onRead()
{
    //todo解包错误
    int32_t nPacketSize = 0;
    while (m_pSocket->bytesAvailable() >= sizeof (int32_t)) {
        qDebug() << m_pSocket->read((char*)&nPacketSize, 4);
        nPacketSize = qFromBigEndian (nPacketSize);
        int32_t nPacketData = nPacketSize - sizeof (int32_t);
        while (m_pSocket->bytesAvailable() < nPacketData);
        char *pBuf = new char[nPacketData+1];
        memset(pBuf, 0, nPacketData+1);
        m_pSocket->read(pBuf, nPacketData);
        QJsonParseError jsonError;
        QJsonDocument doucment = QJsonDocument::fromJson(pBuf, &jsonError);
        if (!doucment.isNull() && (jsonError.error == QJsonParseError::NoError)) {
            if (doucment.isObject()) {
                auto jsonObj = doucment.object();
                auto nId = jsonObj.value("Id").toInt();
                auto nPId = jsonObj.value("PId").toInt();
                auto strMsgData = jsonObj.value("MsgData").toString().toStdString();
                protoDispath(nPId, strMsgData);
            }
        }
        delete []pBuf;
    }

}

void MainWindow::onListViewClicked(const QModelIndex &index)
{
    ui->to_uid_label->setText(m_listModel.data(index).toString());
}
