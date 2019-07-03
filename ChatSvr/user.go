package main
import (
    "net"
    "bytes"
    "fmt"
    "encoding/binary"
    l4g "github.com/alecthomas/log4go"
    "github.com/golang/protobuf/proto"
)

const (
    EUserOnline = iota
    EUserOffline
)

type User struct {
    id, chatcnt int32
    nickname string
    net.Conn
    status int32
    lastLoginTime, regTime, leaveTime int64
}

type IDList []int32
//type UsersManager map[int32]*User

type UsersManager struct {
    UserPool map[int32]*User
    UserCh chan *User
}

type UserMsg struct {
    typeName string
    msg proto.Message
    user *User
}
var (
    OnlineUsers = UsersManager{
        UserPool: make(map[int32]*User),
        UserCh: make(chan *User, 1024),
    }//在线用户
    ConnUsers = UsersManager{
        UserPool: make(map[int32]*User),
        UserCh: make(chan *User, 1024),
    }//连接管理
)

func (this *User) Online () {
    this.status = EUserOnline
    SendUserStatusChange(this)
}

func (this *User) Offline() {
    this.status = EUserOffline
    SendUserStatusChange(this)
}

//单独发回给用户
func (this *User) Send (msg proto.Message) error {
    buf, err := proto.Marshal(msg)
    if (err != nil) {
        l4g.Error("Send Proto Marahal failed:%s",err)
        return err
    }
    msgType := proto.MessageName(msg)
    var nLen uint32 = 4 + 4 + uint32(len(msgType)) + uint32(len(buf))
    var sendBuf bytes.Buffer
    var headerBuf = []byte{0,0,0,0}
    binary.BigEndian.PutUint32(headerBuf, nLen)
    sendBuf.Write(headerBuf)
    var msgTypeLenBuf = []byte{0,0,0,0}
    binary.BigEndian.PutUint32(msgTypeLenBuf, uint32(len(msgType)))
    sendBuf.Write(msgTypeLenBuf)
    sendBuf.Write([]byte(msgType))
    sendBuf.Write(buf)
    this.Write(sendBuf.Bytes())
    return nil
}
//发送除了自己的用户
func (this *User) SendToAll (msg proto.Message) error{
    for k, v := range OnlineUsers.UserPool {
        if k != this.id {
            if err := v.Send(msg); err != nil{
                return err
            }
        }
    }
    return nil;
}
//发送给指定用户
func (this *User) SendTo (msg proto.Message, targetId int32) (error) {
    pUser, has := OnlineUsers.UserPool[targetId]
    if has {
         return pUser.Send(msg)
    } else {
        l4g.Error("%d send to %d failed", this.id, targetId)
    }
    return nil
}

func (this *UsersManager) NotifyUserStatusChange (pUser *User) {
    this.UserCh <- pUser
}

func (this *UsersManager) StatusChange(pUser *User) {
    if pUser.status == EUserOnline {
        this.Append(pUser)
    } else if pUser.status == EUserOffline{
        this.Delete(pUser)
    } else {
        l4g.Error("user status unKnow:%d", pUser.status)
    }
}

func (pUM *UsersManager) Append(pU * User) error {
    if _, has := pUM.UserPool[pU.id]; has {
        return fmt.Errorf("id:%s,addr:%s,already had", pU.id, pU.RemoteAddr().Network()) 
    }
    pUM.UserPool[pU.id] =  pU
    return nil
}

func (pUM *UsersManager) Delete(pU *User) error {
    if _, has := pUM.UserPool[pU.id]; !has {
        return fmt.Errorf("delete id:%d,addr:%s not found", pU.id, pU.RemoteAddr().Network())
    }
    delete(pUM.UserPool, pU.id)
    return nil
}

func (pUM *UsersManager) GetOnlineList() IDList {
    var idList = IDList{}
    for k, _ := range pUM.UserPool {
        idList = append(idList, k)
    }
    return idList
}

func (this *UsersManager) GetUserByID (id int32) (*User, error) {
    if pUser, has := this.UserPool[id]; has {
        return pUser, nil
    }
    return nil, fmt.Errorf("not found user id:%d",id)
}
