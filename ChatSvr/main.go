package main

import (
    "time"
    "strconv"
    l4g "github.com/alecthomas/log4go"
    "github.com/fzzy/radix/redis"
)

var (
    RedisCli  *redis.Client
    svr *Server
    ticker *time.Timer
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
    ticker = time.NewTimer(5 * time.Second)
    svr = &Server {
        Addr: "0.0.0.0:8000",
        CurConnId: 0,
        MaxMsgBuf: 10240,
        inChannel: make(chan *UserMsg, 8192),
    }
    SvrReadConfig()
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

func SvrReadConfig() {
    r, err := RedisCli.Cmd("hgetall", KeyConfig).Hash()
    if (RedisErrHndlr(err)) {
        if len(r) == 0 {
           SvrConfigInit() 
        } else {
            id, _ := r[FiledCurUID]
            nId, _ := strconv.Atoi(id)
            svr.CurUserId = int32(nId)
        }
    }
}

func SvrConfigInit() {
    l4g.Info("svr config install")
    r, err := RedisCli.Cmd(
        "hmset", 
        KeyConfig,
        FiledCurUID,
        1000,
    ).Bool()
    if (RedisErrHndlr(err)) {
        if r {
            l4g.Info("install config successed")
        } else {
            l4g.Error("install config failed")
        }
    }
}

func Run() {
    for {
        l4g.Info("loop:%d cap:%d", len(svr.inChannel), cap(svr.inChannel))
        select {
            case msg := <-svr.inChannel:
                l4g.Info(msg)
                ProtoDispather(msg)
            case pUser := <-OnlineUsers.UserCh:
                l4g.Debug("user change")
                OnlineUsers.StatusChange(pUser)
            case pUser := <-ConnUsers.UserCh:
                l4g.Debug("conn change")
                ConnUsers.StatusChange(pUser)
            case tm := <-ticker.C:
                ticker.Reset(5*time.Second)
                l4g.Info("[TIMER EVENT][%s] ONLINE:%d, CONN:%d", tm, len(OnlineUsers.UserPool), len(ConnUsers.UserPool))
            //default:
                //l4g.Debug("run loop default")
        }
    }
    l4g.Debug("exit run loop")
}
