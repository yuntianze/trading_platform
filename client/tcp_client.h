/**************************************************************************************
 *  @file    tcp_client.h
 *  @brief   tcp通讯客户端
 *  @author  stanjiang
 *  @date    2024-07-10
 *  @copyright 
 ***/

#ifndef _USERS_JIANGPENG_CODE_TRADING_PLATFORM_CLIENT_TCP_CLIENT_H_
#define _USERS_JIANGPENG_CODE_TRADING_PLATFORM_CLIENT_TCP_CLIENT_H_

#include "tcp_comm.h"
#include <string>


struct ClientOptInfo {
    int fd;  // socket
    bool send_flag;  // 数据发送标志,true:可以发送数据,false:不可以发送
    bool recv_flag;  // 数据接收标志,true:可以接收数据,false:不可以接收
    int recv_bytes;  // 接收的字节数
    char recv_buf[RECV_BUF_LEN];  // 接收到的client请求包
    int uin;
};


class TcpClient {
 public:
    TcpClient() {}
    ~TcpClient() { destroy();}

    /***
     * @brief   创建实例,即内存托管在共享内存中
     * @return  实例指针
     ***/
    static TcpClient*  create_instance(void);

    /***
     * @brief   重载new与delete操作,使其对象在共享内存中分配与回收空间
     * @return  void
     ***/
    static void* operator new(size_t size);
    static void operator delete(void* mem);

    /***
     * @brief   初始化
     * @param   port: 端口地址
     * @param   addr: 套接字地址     
     * @return    void
     ***/
    int init(const char* ip, USHORT port);

    /***
     * @brief   进入客户端事件循环
     * @return  void
     ***/
    void run(void);

 private:
    /***
     * @brief   创建客户端套接字
     * @param   port: 端口地址
     * @param   addr: 套接字地址
     * @return  fd
     ***/
    int create_socket(const char* ip, USHORT port);

    /***
     * @brief   预处理IO事件
     * @return    void
     ***/
    void prepare_io_event(void);

    /***
     * @brief   定时处理
     * @return  void
     ***/
    void tick_handle(void);

    /***
     * @brief   发送CS消息
     * @return   void
     ***/
    void send_data_to_server(void);

    /***
     * @brief   接收CS消息
     * @return   void
     ***/
    void recv_data_from_server(void);

    /***
     * @brief   打印消息包内容
     * @param   addr: 套接字地址
     * @return   void
     ***/
    void print_msg(const std::string& buf);

    /***
     * @brief   发送AccountLoginReq消息
     * @param   fd: client套接字
     * @param   uin: client 账号
     * @return    void
     ***/
    int send_account_login_req(int fd, UINT uin);

    /***
     * @brief   接收AccountLoginRes消息
     * @param   fd: client套接字
     * @return    void
     ***/
    int recv_account_login_res(int fd, const char* pkg, int len);

    /***
     * @brief   进程退出前清理数据
     * @return    void
     ***/
    void destroy(void);

 private:
    static char* current_shmptr_;  // 客户端共享内存地址

    int client_conn_[MAX_CLIENT_CONN];  // 客户端连接socket
    int maxfd_;  // 最大文件描述符
    ClientOptInfo client_opt_[MAX_CLIENT_CONN+1];  // 客户端操作信息
    int client_epoll_fd_;  // 客户端epoll创建的句柄
    struct epoll_event client_events_[MAX_CLIENT_CONN];  // 客户端epoll事件
    int cur_conn_num_;  // 当前连接数

    int send_pkg_count_;  // 发送包数
    int recv_pkg_count_;  // 接收包数
    time_t laststat_time_;  // 上次统计时间
};


#endif  // _USERS_JIANGPENG_CODE_TRADING_PLATFORM_CLIENT_TCP_CLIENT_H_
