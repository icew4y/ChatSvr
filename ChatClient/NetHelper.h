#ifndef NETHELPER_H
#define NETHELPER_H
#include "QTcpSocket"
#include <QtEndian>
#include <sstream>
#include <google/protobuf/message.h>
#include <common.hpp>
#include <msgqueue.hpp>
#include <QDebug>
#include <thread>
#include <memory>
#include "protodispather.hpp"
#define NET Singleton<NetHelper>::instance()

struct MsgItem {
    enum MsgType{
        eMsgType_End = 1,
        eMsgType_Proto = 2,
    };
    std::shared_ptr<google::protobuf::Message> pMsg = nullptr;
    MsgType eType = eMsgType_Proto;
};

class NetHelper : public QTcpSocket
{
    Q_OBJECT
public:
    friend Singleton<NetHelper>;
    NetHelper(QObject *pObj = nullptr);

    //|--all len--|--msg type len--|--msg type string--|--data len--|
    //|----32-----|-------32-------|--------x----------|-----x------|
    int32_t send(const google::protobuf::Message &msg);

    std::shared_ptr<ProtoBufDispather> GetDispather() const;
private slots:
    void onRead();
private:
    bool init();
    void run();
    std::shared_ptr<std::thread> m_pThread = nullptr;
    XMsgQueue<MsgItem> m_queue;
    std::shared_ptr<ProtoBufDispather> m_pDispather;
};

#endif // NETHELPER_H
