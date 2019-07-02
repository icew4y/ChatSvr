package main

import (
    "./chatpb"
    "github.com/golang/protobuf/proto"
    "fmt"
    "time"
    "reflect"
    l4g "github.com/alecthomas/log4go"
)

const (
   KeyUser = "USER"
)

var ProtoMap = make(map[string]IProto)

type IProto interface {
    Execute(msg proto.Message, pUser *User)
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
        if string(pb.GetPassword()) == r["password"] {
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

func ProtoRegister() {
    ProtoMap["chatpb.C2SLogin"] = C2SLogin{}
    ProtoMap["chatpb.C2SRegister"] = C2SRegister{}
}

type C2SRegister chatpb.C2SRegister
func (this C2SRegister) Execute (msg proto.Message, pUser *User) {
    pb := msg.(*chatpb.C2SRegister)
    fmt.Println("test")
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
    bR, err := RedisCli.Cmd("hmset", key, "username", string(pb.GetUsername()), "password", string(pb.GetPassword())).Bool()
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

