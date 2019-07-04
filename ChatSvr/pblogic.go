package main

import (
    "./chatpb"
    "github.com/golang/protobuf/proto"
    "fmt"
    "time"
    "reflect"
    "encoding/json"
    l4g "github.com/alecthomas/log4go"
)

const (
    KeyUser = "USER"
    KeyConfig = "Config:Normal"
    KeyChat = "Chat:"
    KeyChatCnt = "Chat:Task:Count"
    FiledCurUID = "CurUserID"

    ChatSaveType_Self = 1
    ChatSaveType_Other = 2
)


var (
    ProtoMap = make(map[string]IProto)
    ChatMapCache = make(map[int64]* chatpb.S2CChat, 10)
)

type IProto interface {
    Execute(msg proto.Message, pUser *User)
}

func ProtoRegister() {
    ProtoMap["chatpb.C2SLogin"] = C2SLogin{}
    ProtoMap["chatpb.C2SRegister"] = C2SRegister{}
    ProtoMap["chatpb.C2SOnlineUsers"] = C2SOnlineUsers{}
    ProtoMap["chatpb.C2SChat"] = C2SChat{}
    ProtoMap["chatpb.C2SChatCntTopUsers"] = C2SChatCntTopUsers{}
    ProtoMap["chatpb.C2SChatCnt"] = C2SChatCnt{}
    ProtoMap["chatpb.C2SHeartBeat"] = C2SHeartBeat{}
}

type C2SHeartBeat chatpb.C2SHeartBeat
func (this C2SHeartBeat) Execute (msg proto.Message, pUser *User) {
    l4g.Info("[heartbeat]connid:%d, userid:%d, nickname:%s", pUser.connid, pUser.id, pUser.nickname)
}

type C2SChatCnt chatpb.C2SChatCnt
func (this C2SChatCnt) Execute (msg proto.Message, pUser *User) {
    pb := msg.(*chatpb.C2SChatCnt)
    r, err := RedisCli.Cmd("zscore", KeyChatCnt, pb.Nick).Int64()
    retMsg := &chatpb.S2CChatCnt {
        ERet: chatpb.ERetType_Success,
        Nick: pb.GetNick(),
    }
    defer pUser.Send(retMsg)
    if (RedisErrHndlr(err)){
        retMsg.Chatcnt = int32(r)
    } else {
        retMsg.ERet = chatpb.ERetType_RedisError
    }
}

type C2SChatCntTopUsers chatpb.C2SChatCntTopUsers
func (this C2SChatCntTopUsers) Execute (msg proto.Message, pUser *User) {
    pb := msg.(*chatpb.C2SChatCntTopUsers)
    retMsg := &chatpb.S2CChatCntTopUsers {
        ERet: chatpb.ERetType_Success,
    }
    defer pUser.Send(retMsg)
    r, err := RedisCli.Cmd("zrevrange", KeyChatCnt, pb.Start-1, pb.Start-1+pb.Cnt, "withscores").Hash()
    users := []*chatpb.StructUser {}
    if (RedisErrHndlr(err)) {
        for k, _ := range r{
            pTempUser := &chatpb.StructUser {
                Username: []byte(k),
                Chatcnt: AtoInt32FromHash(r, k),
            }
            users = append(users, pTempUser)
        }
        retMsg.Users = users
    } else {
        retMsg.ERet = chatpb.ERetType_RedisError
        return
    }
}

type C2SChat chatpb.C2SChat
func (this C2SChat) Execute (msg proto.Message, pUser *User) {
    pb := msg.(*chatpb.C2SChat)
    l4g.Info(pb)
    var retMsg = &chatpb.S2CChat {
        EMsgType: pb.GetEMsgType(),
        Msg: pb.GetMsg(),
        FromUid: pUser.id,
        FromNick: []byte(pUser.nickname),
        ToUid: pb.GetToUid(),
        ToNick: pb.GetToNick(),
        SendTime: time.Now().UnixNano(),
        ERet: chatpb.ERetType_Success,
    }
    defer pUser.Send(retMsg)
    if pb.GetEMsgType() == chatpb.EMsgType_eMsg_All {
        if err := pUser.SendToAll(retMsg); err != nil {
            l4g.Error("send chat failed,from %s to %s, type %d", pUser.nickname, pb.GetToNick(), pb.GetEMsgType())
            retMsg.ERet = chatpb.ERetType_NormalError
            return
        }
    } else if pb.GetEMsgType() == chatpb.EMsgType_eMsg_One {
        //判断下是否在线
        if _, err := OnlineUsers.GetUserByID(pb.GetToUid()); err != nil {
            retMsg.ERet = chatpb.ERetType_UserOffline
            return
        }
        if err := pUser.SendTo(retMsg, pb.GetToUid()); err != nil {
            l4g.Error("send chat failed,from %s to %s, type %d", pUser.nickname, pb.GetToNick(), pb.GetEMsgType())
            retMsg.ERet = chatpb.ERetType_NormalError
            return
        }
    } else {
        l4g.Info("unknow msg type %s", pb.GetEMsgType())
        retMsg.ERet = chatpb.ERetType_NormalError
        return
    }

    if (retMsg.ERet == chatpb.ERetType_Success) {
        //聊天记录写到缓存
        if jsonBuf, err := json.Marshal(retMsg); err == nil{
            key := fmt.Sprintf("%s%s", KeyChat,pUser.nickname)
            _, err := RedisCli.Cmd("zadd", key, retMsg.SendTime, string(jsonBuf)).Int64()
            if (!RedisErrHndlr(err)) {
                retMsg.ERet = chatpb.ERetType_RedisError
                return
            }
        } else {
            l4g.Error("%s", err)
            retMsg.ERet = chatpb.ERetType_NormalError
            return
        }
        //更新下聊天数量
        n, err := RedisCli.Cmd("zincrby", KeyChatCnt, 1, pUser.nickname).Int64()
        if (RedisErrHndlr(err)) {
            pUser.chatcnt = int32(n)
        }
        //保存最近10条广播消息
        if retMsg.EMsgType != chatpb.EMsgType_eMsg_All {
            return
        }
        l4g.Info("len......%d", len(ChatMapCache))
        if len(ChatMapCache) >= 10 {
            var min int64 = time.Now().UnixNano()
            for k, _ := range ChatMapCache {
                if min > k{
                    min = k
                }
            }
            delete(ChatMapCache, min)
        }
        ChatMapCache[retMsg.SendTime]= retMsg
    }
}

type C2SOnlineUsers chatpb.C2SOnlineUsers
func (this C2SOnlineUsers) Execute(msg proto.Message, pUser *User) {
    retMsg := &chatpb.S2COnlineUsers {ERet: chatpb.ERetType_Success}
    users := []*chatpb.StructUser{}
    for _, pUser := range OnlineUsers.UserPool {
        user := &chatpb.StructUser {
            Username: []byte(pUser.nickname),
            Lastlogintime: pUser.lastLoginTime,
            Regtime: pUser.regTime,
            Chatcnt: pUser.chatcnt,
            Lastleavetime: pUser.leaveTime,
            Uid: pUser.id,
        }
        users = append(users, user)
    }
    retMsg.Users = users
    pUser.Send(retMsg)
}

type C2SLogin chatpb.C2SLogin
func (this C2SLogin) Execute (msg proto.Message, pUser *User) {
    pb := msg.(*chatpb.C2SLogin)
    var retMsg = &chatpb.S2CLogin {
        ERet : chatpb.ERetType_Success,
    }
    defer pUser.Send(retMsg)
    key := fmt.Sprintf("%s:%s", KeyUser, string(pb.GetUsername()))
    r, err := RedisCli.Cmd("hgetall", key).Hash()
    if (RedisErrHndlr(err)) {
        if len(r) == 0 {
            retMsg.ERet = chatpb.ERetType_UnError
            l4g.Error("[login]username err:%s", string(pb.GetUsername()))
            return
        }
        //密码认证成功
        if string(pb.GetPassword()) == r["password"] {
           //是否已经在线 
            if (OnlineUsers.Existed(string(pb.GetUsername()))){
                retMsg.ERet = chatpb.ERetType_UserOnline
                return
            }
            //更新登录信息
            _, err := RedisCli.Cmd(
            "hmset",
            key,
            "lastlogintime",
            time.Now().Unix(),
            ).Bool()
            RedisErrHndlr(err)
            //拉取用户信息到协议
            name, _ := r["username"]
            pUser.nickname = name
            pUser.id = AtoInt32FromHash(r, "uid")
            pUser.regTime = AtoInt64FromHash(r, "regtime")
            pUser.lastLoginTime = time.Now().Unix()
            pUser.leaveTime = AtoInt64FromHash(r, "leavetime")
            pUser.chatcnt = AtoInt32FromHash(r, "chatcnt")
            OnlineUsers.NotifyUserStatusChange(pUser)
            //最近广播的记录
            lastChat := []*chatpb.S2CChat{}
            for _, v := range ChatMapCache {
                lastChat = append(lastChat, v)
            }
            retMsg.Lastchat = lastChat
            //拉取聊天次数
            pbUser := &chatpb.StructUser {
                Username: []byte(pUser.nickname),
                Uid: pUser.id,
                Regtime: pUser.regTime,
                Chatcnt: pUser.chatcnt,
                Lastlogintime: pUser.lastLoginTime,
                Lastleavetime: pUser.leaveTime,
            }
            retMsg.Info = pbUser
            pUser.Online(true)
            l4g.Info("[login]username:%s login success, time:%s", string(pb.GetUsername()), time.Now())
            return
        } else {
            retMsg.ERet = chatpb.ERetType_PwError
            l4g.Error("[login]password err")
        }
    } else {
        retMsg.ERet = chatpb.ERetType_RedisError
        return
    }

}


type C2SRegister chatpb.C2SRegister
func (this C2SRegister) Execute (msg proto.Message, pUser *User) {
    pb := msg.(*chatpb.C2SRegister)
    var retMsg = &chatpb.S2CRegister {
        ERet : chatpb.ERetType_Success,
    }
    defer pUser.Send(retMsg)
    key := fmt.Sprintf("%s:%s", KeyUser, string(pb.GetUsername()))
    r, err := RedisCli.Cmd("exists", key).Int64()
    if (RedisErrHndlr(err)) {
        if r == 1 {
            retMsg.ERet = chatpb.ERetType_UnExistError
            return
        }
    } else {
        retMsg.ERet = chatpb.ERetType_RedisError
        return
    }
    pwLen := len(pb.GetPassword())
    unLen := len(pb.GetUsername())
    if (unLen < 2 || unLen > 20) {
        retMsg.ERet = chatpb.ERetType_UnNotStandard
        return
    }
    if (pwLen < 6 || pwLen > 20) {
        retMsg.ERet = chatpb.ERetType_PwNotStandard
        return
    }
    //获取当前UID
    curid ,err := RedisCli.Cmd("hincrby", KeyConfig, FiledCurUID, 1).Int64()
    if !RedisErrHndlr(err) {
        retMsg.ERet = chatpb.ERetType_RedisError
        return
    }
    //写入信息
    bR, err := RedisCli.Cmd(
        "hmset",
        key,
        "username",
        string(pb.GetUsername()),
        "password",
        string(pb.GetPassword()),
        "uid",
        curid,
        "regtime",
        time.Now().Unix(),
        "lastlogintime",
        time.Now().Unix(),
        "leavetime",
        0,
    ).Bool()
    if (RedisErrHndlr(err)) {
        if !bR {
            retMsg.ERet =  chatpb.ERetType_RedisError
        }
    } else {
        retMsg.ERet =  chatpb.ERetType_RedisError
    }
}

func ProtoDispather (pMsg *UserMsg) {
    cb, has := ProtoMap[pMsg.typeName]
    if has {
        cb.Execute(pMsg.msg, pMsg.user)
    } else {
        l4g.Error("ProtoDispather: not found proto:%s", pMsg.typeName)
    }
}

func GetUserMsg (typeName string, dataBuf []byte) (proto.Message) {
    _, has := ProtoMap[typeName]
    if has {
        msgType := proto.MessageType(typeName)
        var msg = reflect.Indirect(reflect.New(msgType.Elem())).Addr().Interface().(proto.Message)
        if err := proto.Unmarshal(dataBuf, msg); err != nil {
             l4g.Error("unmarshal [%s] error", typeName)
        }
        return msg
    } else {
        l4g.Error("GetUserMsg: not found proto:%s", typeName)
    }
    return nil
}

func SendUserStatusChange (pUser *User) {
    msg := &chatpb.S2CStatusChange {
        ERet: chatpb.ERetType_Success,
    }
     pbUser := &chatpb.StructUser {
                Username: []byte(pUser.nickname),
                Uid: pUser.id,
                Regtime: pUser.regTime,
                Chatcnt: pUser.chatcnt,
                Lastlogintime: pUser.lastLoginTime,
                Lastleavetime: pUser.leaveTime,
            }
    msg.User = pbUser
    if (pUser.status == EUserOnline) {
        l4g.Info("send online user:%s", pUser.nickname)
        msg.Status = chatpb.EUserStatus_eStatus_Online
    } else if (pUser.status == EUserOffline) {
        l4g.Info("send offline user:%s", pUser.nickname)
        key := fmt.Sprintf("%s:%s", KeyUser, pUser.nickname)
        msg.Status = chatpb.EUserStatus_eStatus_Offline
        //更新下最后登录时间
        _, err := RedisCli.Cmd(
        "hmset",
        key,
        "leavetime",
        time.Now().Unix(),
        ).Bool()
        RedisErrHndlr(err)
    }
    pUser.SendToAll(msg)
}
