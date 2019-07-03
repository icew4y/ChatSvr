package main

import (
    "bytes"
    "os"
    "encoding/binary"
    "net"
    l4g "github.com/alecthomas/log4go"
)


type Server struct {
    Addr        string
    CurConnId   int32
    CurUserId   int32
    MaxMsgBuf   int32
    inChannel   chan *UserMsg
}

func (this *Server) Listen () {
    listener, err := net.Listen("tcp", this.Addr)
    if (err != nil) {
        l4g.Error(err)
        os.Exit(1)
    }
    defer listener.Close()
    l4g.Info("listen successed")
    for {
        conn, err := listener.Accept()
        if err != nil {
            l4g.Error(err)
            os.Exit(0)
        }
        this.CurConnId++
        user := User {id:this.CurConnId, Conn:conn}
        l4g.Info(">>>>> new connection %s >>>>>" , conn.RemoteAddr())
        go this.ClientCb(&user)
    }
}

func (this *Server)ClientCb(pUser *User) {
    defer func () {
        pUser.Offline()
        OnlineUsers.NotifyUserStatusChange(pUser)
        ConnUsers.NotifyUserStatusChange(pUser)
        pUser.Close()
    }()
    readBuf :=make([]byte, this.MaxMsgBuf)
    for {
        n, err := pUser.Read(readBuf)
        if err != nil{
            l4g.Error(err)
            break
        }
        if n >= 4 {
            headBuf := bytes.NewBuffer(readBuf[0:4])
            var pkgLen int32
            binary.Read(headBuf, binary.BigEndian, &pkgLen)
            for int32(n) >= pkgLen {
                if int32(n) > this.MaxMsgBuf {
                    l4g.Error("Read size failed %d", n)
                    return
                }
                var typeLen int32
                typeBuf := bytes.NewBuffer(readBuf[4:8])
                binary.Read(typeBuf, binary.BigEndian, &typeLen)
                typeEndPos := 8+typeLen
                if (typeEndPos > int32(n)) {
                    l4g.Error("Read type len failed end pos:%d,cur end pos:%d", typeEndPos, n)
                }
                var strTypeName = string(readBuf[8:typeEndPos])
                pb := GetUserMsg(strTypeName, readBuf[typeEndPos:n]) 
                um := &UserMsg {
                    typeName: strTypeName,
                    msg: pb,
                    user: pUser,
                }
                this.inChannel <- um
                l4g.Info("msg push chanlen:%d %d", len(this.inChannel), cap(this.inChannel))
                n = n - int(pkgLen)
            }
        }
    }
}
