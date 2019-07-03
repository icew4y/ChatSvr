#include "nethelper.h"
#include <QDebug>
NetHelper::NetHelper(QObject *pObj)
    : QTcpSocket (pObj)
{
    init();
}

int32_t NetHelper::send(const google::protobuf::Message &msg)
{
    qDebug () << "send:" << msg.Utf8DebugString().c_str();
    auto strData = msg.SerializeAsString();
    auto strTypeName = msg.GetTypeName();
    uint32_t nMsgTypeSize = strTypeName.size();
    uint32_t nPacketSize = strData.size() + strTypeName.size() + sizeof(nPacketSize) + sizeof(nMsgTypeSize);
    //都用大端
    nPacketSize = qToBigEndian(nPacketSize);
    nMsgTypeSize = qToBigEndian(nMsgTypeSize);
    static char tmpBuf[10240];
    uint32_t nCurLen = 0;
    memcpy(tmpBuf, &nPacketSize, sizeof(nPacketSize));
    nCurLen += sizeof(nPacketSize);
    memcpy(tmpBuf + nCurLen, &nMsgTypeSize, sizeof(nMsgTypeSize));
    nCurLen += sizeof(nMsgTypeSize);
    memcpy(tmpBuf + nCurLen, strTypeName.c_str(), strTypeName.size());
    nCurLen += strTypeName.size();
    memcpy(tmpBuf + nCurLen, strData.c_str(), strData.size());
    nCurLen += strData.size();
    return this->write(tmpBuf, nCurLen);
}

std::shared_ptr<ProtoBufDispather> NetHelper::GetDispather() const
{
    return m_pDispather;
}

//暂时都放在一个线程处理把....因为Qt自带的Socket也在主线程,意义不是很大
void NetHelper::onRead()
{
    int32_t nPacketSize = 0, nMsgTypeSize = 0;
    while (this->bytesAvailable() >= sizeof (int32_t)*2) {
        this->read((char*)&nPacketSize, 4);
        this->read((char*)&nMsgTypeSize, 4);
        nPacketSize = qFromBigEndian (nPacketSize);
        nMsgTypeSize = qFromBigEndian (nMsgTypeSize);
        int32_t nPacketData = nPacketSize - sizeof (int32_t)*2;
        while (this->bytesAvailable() < nPacketData);
        std::shared_ptr<char> pBuf(new char[nPacketData+1], std::default_delete<char[]>());
        memset(pBuf.get(), 0, nPacketData+1);
        this->read(pBuf.get(), nPacketData);
        std::string strMsgType(pBuf.get(), pBuf.get() + nMsgTypeSize);
        auto pMsg = m_pDispather->CreateMsg(strMsgType);
        pMsg->ParseFromString(std::string(pBuf.get() + nMsgTypeSize, pBuf.get() + nPacketData));
        qDebug() <<"recv:"<< strMsgType.c_str() << pMsg->ShortDebugString().c_str();
        m_pDispather->onMsgCallBack(pMsg);
//        pMsg->ParseFromString(strMsg);
//        MsgItem item;
//        item.pMsg = pMsg;
//        m_queue.push(item);
    }
}

bool NetHelper::init()
{
//    if (!m_pThread) {
//        m_pThread = std::make_shared<std::thread>(std::bind(&NetHelper::run, this));
//    }
    m_pDispather = std::make_shared<ProtoBufDispather>();
    connect(this, &NetHelper::readyRead, this, &NetHelper::onRead);
    return true;
}

void NetHelper::run() {
//    MsgItem item;
//    while(m_queue.get(item)) {
//        switch (item.eType) {
//        case MsgItem::eMsgType_Proto:
//            break;
//        case MsgItem::eMsgType_End:
//            return;
//        default:
//            qDebug () << "unkown proto";
//            break;
//        }
//    }
}
