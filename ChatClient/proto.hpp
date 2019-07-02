#pragma once
#include "cstdint"
#include "string"
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>

#define NOTITY_ALL 999

enum ProtoId {
    eProto_UserId = 1000,
    eProto_Chat,
    eProto_UserList,
};

struct MsgSt{
    int32_t nUid = 0;
    ProtoId nPId = eProto_Chat;
    std::string strMsg;

    std::string GetData() const{
        QJsonObject json;
        json.insert("UId", nUid);
        json.insert("PId", nPId);
        json.insert("MsgData", QString::fromStdString(strMsg));

        QJsonDocument document;
        document.setObject(json);
        QByteArray byteArray = document.toJson(QJsonDocument::Compact);
        QString strJson(byteArray);
        return strJson.toStdString();
    }
};

