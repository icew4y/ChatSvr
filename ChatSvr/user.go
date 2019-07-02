package main
import (
    "net"
    "bytes"
    "sync"
    "fmt"
    "encoding/binary"
    l4g "github.com/alecthomas/log4go"
    "github.com/golang/protobuf/proto"
)

type User struct {
    id int32
    uid, connid int32
    nickname string
    net.Conn
}

type IDList []int32
type UsersManager map[int32]*User
var OnlineUsers = make(UsersManager)
var UnAuthUsers = make(UsersManager)
var userLck sync.Mutex
type UserMsg struct {
    typeName string
    msg proto.Message
    user *User
}

//单独发给用户
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

func (pUM *UsersManager) Append(pU * User) error {
    userLck.Lock()
    defer userLck.Unlock()
    if _, has := (*pUM)[pU.id]; has {
        return fmt.Errorf("id:%s,addr:%s,already had", pU.id, pU.RemoteAddr().Network()) 
    }
    (*pUM)[pU.id] =  pU
    return nil
}

func (pUM *UsersManager) Delete(pU *User) error {
    userLck.Lock()
    defer userLck.Unlock()
    if _, has := (*pUM)[pU.id]; !has {
        return fmt.Errorf("delete id:%d,addr:%s not found", pU.id, pU.RemoteAddr().Network())
    }
    delete(OnlineUsers, pU.id)
    return nil
}

func (pUM *UsersManager) GetOnlineList() IDList {
    userLck.Lock()
    defer userLck.Unlock()
    var idList = IDList{}
    for k, _ := range *pUM {
        idList = append(idList, k)
    }
    return idList
}

func (this *UsersManager) GetUserByID (id int32) (*User, error) {
    userLck.Lock()
    defer userLck.Unlock()
    if pUser, has := (*this)[id]; has {
        return pUser, nil
    }
    return nil, fmt.Errorf("not found user id:%d",id)
}
