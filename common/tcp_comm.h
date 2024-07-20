/*************************************************************************
 *  @file    tcp_comm.h
 *  @brief   公用头文件，数据结构、常量等
 *  @author  stanjiang
 *  @date    2024-07-10
 *  @copyright
***/

#ifndef _USERS_JIANGPENG_CODE_TRADING_PLATFORM_COMMON_TCP_COMM_H_
#define _USERS_JIANGPENG_CODE_TRADING_PLATFORM_COMMON_TCP_COMM_H_

// 系统头文件
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdarg.h>
#include <errno.h>
#include <netinet/in.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/file.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <assert.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/epoll.h>
#include <cstdint>


/***系统类型定义 ***/
typedef unsigned char UCHAR;
typedef unsigned int UINT;
typedef std::uint16_t USHORT;
typedef std::uint64_t ULONG;


/***系统宏定义***/
#define INVALID_SOCKET    -1        /*无效socket句柄*/
#define IP_LENGTH         20        /*IP地址长度*/
#define RECV_BUF_LEN      16384     /*接收客户端信息的缓冲区*/

/***系统常量参数定义***/

// 连接IP和Port
const char CONNECT_IP[] = "127.0.0.1";
const USHORT CONNECT_PORT = 8000;

// tcpclient测试连接数量
const int MAX_CLIENT_CONN = 100;

// tcpsvr处理最大连接数量
const int MAX_SOCKET_NUM = 200;

const int SOCK_RECV_BUFFER = 512*1024;
const int SOCK_SEND_BUFFER = 512*1024;
const int STR_COMM_LEN = 128;
const int LISTEN_BACKLOG = 512;

const char TCPSVR_PIPE_FILE[] = "tcppipefile";

// 客户端占用共享内存 key
const int CLIENT_SHM_KEY = 1110;

// 存储socket连接的共享内存key
const int SOCKET_SHM_KEY = 1111;


// CS通讯包包头中长度字段占用的大小
const int PKGHEAD_FIELD_SIZE = sizeof(int);
// CS通信包最小长度
const int MIN_CSPKG_LEN = 10;
// CS通信包最大长度
const int MAX_CSPKG_LEN = RECV_BUF_LEN;
// CS打包临时缓冲区大小
const int CSPKG_OPT_BUFFSIZE = RECV_BUF_LEN*2;

// tcpsvr一次从消息队列中取得的最大消息包数,即一次最多发送给client的消息回包数
const int MAX_SEND_PKGNUM = 512;

/***系统公共数据结构定义 ***/

// 创建共享内存的模式
enum ShmMode {
    MODE_NONE = -1,
    MODE_INIT = 0,    // 全新，即原来没有共享内存新创建
    MODE_RESUME = 1,  // 恢复，即共享原来已经存在，恢复使用
    MODE_MAX
};


// 通信服务器运行类型
enum SvrRunFlag {
    RUN_INIT = 0,
    RELOAD_CFG = 1,
    TCP_EXIT = 2,
};

// 网络连接出错码定义
enum SocketErrors {
    ERROR_OK              =      0,   /*处理正常*/
    ERROR_CLIENT_CLOSE    =      -1,  /*客户端关闭*/
    ERROR_ClIENT_TIMEOUT  =      -2,  /*客户端超时*/
    ERROR_WRITE_BUFFOVER  =      -3,  /*Tcp写缓冲区已满*/
    ERROR_READ_BUFFEMPTY  =      -4,  /*Tcp读缓冲区为空*/
    ERROR_PACKET_INVALID  =      -5,  /*客户端发送的包错误*/
};

// tcpsvr和客户端通讯的socket结构
struct SocketConnInfo {
    int socket_fd;  // socket句柄
    ULONG uin;  // 用户账号
    int recv_bytes;  // 接收的字节数
    char recv_buf[RECV_BUF_LEN];  // 接收到的client请求包
    ULONG client_ip;  // 客户端IP地址
    time_t  create_Time;  // socket的创建时间
    time_t  recv_data_time;  // 接收到数据包的时间戳
};

// tcpsvr与gamesvr通讯的包头
struct CSPkgHead {
    int pkg_size;  // cs总包长
    int fd;  // cs通信socket
    ULONG client_ip;  // 客户端IP地址,以便gamesvr运营时作一些统计用
};

// 统计间隔时间
const int STAT_TIME = 20;

// 统计信息
struct StatInfo {
    int fd;
    int count;
};

#endif  // _USERS_JIANGPENG_CODE_TRADING_PLATFORM_COMMON_TCP_COMM_H_

