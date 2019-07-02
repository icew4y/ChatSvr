package main

import (
    "time"
    l4g "github.com/alecthomas/log4go"
    "github.com/fzzy/radix/redis"
)

var (
    RedisCli  *redis.Client
    svr *Server
)

func main () {
    l4g.LoadConfiguration("config/log.xml")
    if cl ,err := redis.DialTimeout("tcp", "127.0.0.1:7777", time.Duration(10)*time.Second); err == nil {
        r, err := cl.Cmd("auth", "cxc120047898").Bool()
        if RedisErrHndlr(err); r {
            l4g.Info("redis:%s connected", "127.0.0.1:7777")
            RedisCli = cl
        }
    } else {
       RedisErrHndlr(err) 
    }
    defer RedisCli.Close()
    ProtoRegister()
    svr = &Server {
        Addr: "0.0.0.0:8000",
        CurConnId: 0,
        MaxMsgBuf: 10240,
        inChannel: make(chan *UserMsg, 8192),
    }
    go svr.Listen()
    Run()
}

func RedisErrHndlr(err error) (bool) {
    if err != nil {
        l4g.Error("redis err:%s", err)
        return false
    }
    return true
}

func Run() {
    for {
        l4g.Info("loop:%d cap:%d", len(svr.inChannel), cap(svr.inChannel))
        select {
            case msg := <-svr.inChannel:
                l4g.Info(msg)
                ProtoDispather(msg)
            //default:
                //l4g.Debug("run loop default")
        }
    }
    l4g.Debug("exit run loop")
}
