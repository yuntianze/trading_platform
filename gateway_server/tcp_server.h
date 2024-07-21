/*************************************************************************
 * @file    tcp_server.h
 * @brief   tcp通讯服务器
 * @author  stanjiang
 * @date    2024-07-20
 * @copyright
***/

#ifndef GATEWAY_SERVER_TCP_SERVER_H_
#define GATEWAY_SERVER_TCP_SERVER_H_


// 服务器启动模式
enum ServerStartModel {
    SERVER_START_NODAEMON = 0,
    SERVER_START_DAEMON = 1,
    SERVER_START_INVALID
};


class TcpServer {
 public:
    ~TcpServer() {}

    static TcpServer& instance(void) {
        static TcpServer s_inst;
        return s_inst;
    }

    /***
     * @brief   初始化tcpsver
     * @param   model: 服务器启动模式
     * @return   0: ok , -1: error
     ***/
    int init(ServerStartModel model);

 private:
    /***
     * @brief   初始化tcpsver为后台服务
     * @param   model: 服务器启动模式
     * @return   0: ok , -1: error
     ***/
    int init_daemon(ServerStartModel model);

    static void sigusr1_handle(int iSigVal);
    static void sigusr2_handle(int iSigVal);

 private:
    TcpServer() {}
    TcpServer(const TcpServer&);
    TcpServer& operator=(const TcpServer&);
};


#endif  // GATEWAY_SERVER_TCP_SERVER_H_

