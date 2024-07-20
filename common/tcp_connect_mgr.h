/*************************************************************************
 * @file   tcp_connect_mgr.h
 * @brief  tcp连接管理器
 * @author stanjiang
 * @date   2024-07-17
 * @copyright
***/

#ifndef _USERS_JIANGPENG_CODE_TRADING_PLATFORM_COMMON_TCP_CONNECT_MGR_H_
#define _USERS_JIANGPENG_CODE_TRADING_PLATFORM_COMMON_TCP_CONNECT_MGR_H_

#include "tcp_comm.h"


class TcpConnectMgr {
 public:
    TcpConnectMgr();
    ~TcpConnectMgr();

    /***
     * @brief   创建tcp连接管理器实例,即内存托管在共享内存中
     * @return  实例指针
     ***/
    static TcpConnectMgr*  create_instance(void);

    /***
     * @brief   计算连接器所需内存大小
     * @return   内存大小
     ***/

    static int count_size(void);

    /***
     * @brief   重载new操作,使其对象在共享内存中分配空间
     * @param   size: 内存大小
     * @return  void*
     ***/
    static void* operator new(size_t size);

    /***
     * @brief   重载delete操作,使其对象在共享内存中回收空间
     * @param   mem: 内存指针
     * @return  void
     ***/
    static void operator delete(void* mem);

    /***
     * @brief   设置运行标志
     * @param   flag: 通信服务器运行标志
     * @return  void
     ***/
    void set_run_flag(int flag) {run_flag_ = flag;}

    /***
     * @brief   初始化tcp连接管理器
     * @return   0: ok , -1: error
     ***/
    int init(void);

    /***
     * @brief   运行tcp连接管理器,即进入tcp连接处理主循环
     * @return  void
     ***/
    void run(void);

    /***
     * @brief  基于Tcp连接的发送数据接口
     * @param   fd: 连接socket
     * @param   databuf: 待发送数据buf
     * @param   len: 待发送数据长度
     * @return   0: ok , -1: error
     ***/
    static int tcp_send_data(int fd, const char* databuf, int len);

    /***
     * @brief  基于Tcp连接的接收数据接口
     * @param   fd: 连接socket
     * @param   databuf: 待接收数据buf
     * @param   len: 接收到数据的实际长度
     * @return   0: ok , -1: error
     ***/
    static int tcp_recv_data(int fd, char* databuf, int& len);

    /***
     * @brief   设置套接字为非阻塞状态
     * @param   fd: 套接字
     * @return  0: ok , -1: error
     ***/
    static int set_nonblock(int fd);

    /***
     * @brief   设置套接字各特定选项,如:是否重用,send/recv buffer等
     * @param   fd: 套接字
     * @return  0: ok , -1: error
     ***/
    static int set_socket_opt(int fd);

    /***
     * @brief   设置套接字地址信息
     * @param   ip: ip字符串
     * @param   port: 端口地址
     * @param   addr: 套接字地址
     * @return    fd
     ***/
    static void set_address(const char* ip, USHORT port, struct sockaddr_in* addr);

 private:
    /***
     * @brief   读取客户端请求
     * @return  void
     ***/
    void get_client_message(void);

    /***
     * @brief   读取tcpsvr<-->gamesvr的消息队列中收取待发送至client的数据
     * @return  void
     ***/
    void check_wait_send_data(void);

    /***
     * @brief   检测通讯超时的socket
     * @return  void
     ***/
    void check_timeout(void);

 private:
    /***
     * @brief   初始化tcpsvr监听socket
     * @param   ip: ip字符串
     * @param   port: 端口地址
     * @return  0: ok , -1: error
     ***/
    int init_listen_socket(const char* ip, USHORT port);

    /***
     * @brief   因通信错误而清除socket相关信息
     * @param   fd: socket句柄
     * @param   type: 错误码
     * @return  void
     ***/
    void clear_socket_info(int fd, SocketErrors type);

    /***
     * @brief   接收客户端连接的数据
     * @param   fd: accept生成的socket
     * @return  0: ok , -1: error
     ***/
    int recv_client_data(int fd);

    /***
     * @brief   增加/减少连接数量
     * @return   
     ***/	
    void inc_sock_conn(void) {++cur_conn_num_;}
    void dec_sock_conn(void) {--cur_conn_num_;}

 private:
    static char* current_shmptr_;  // attath共享内存地址
    int run_flag_;  // 运行标志
    SocketConnInfo client_sockconn_list_[MAX_SOCKET_NUM];  // 客户端socket连接信息列表
    char send_client_buf_[SOCK_SEND_BUFFER];  // 发送消息至client的buf

    struct epoll_event  events_[MAX_SOCKET_NUM];  // epoll事件集合
    int epoll_fd_;   // 服务器epoll创建的句柄
    int listen_fd_;  // 监听socket句柄
    int maxfds_;  // 最大sockt句柄数
    int epoll_timeout_;  // epoll wait超时时间
    int cur_conn_num_;   // 当前连接数

    int send_pkg_count_;  // 发送包数
    int recv_pkg_count_;  // 接收包数
    int laststat_time_;   // 上次统计时间
};

#endif  // _USERS_JIANGPENG_CODE_TRADING_PLATFORM_COMMON_TCP_CONNECT_MGR_H_



