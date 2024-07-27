/*************************************************************************
 *  @file    tcp_comm.h
 *  @brief   Common header file, containing data structures, constants, etc.
 *  @author  stanjiang
 *  @date    2024-07-10
 *  @copyright
***/

#ifndef _TRADING_PLATFORM_COMMON_TCP_COMM_H_
#define _TRADING_PLATFORM_COMMON_TCP_COMM_H_

// System headers
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <cerrno>
#include <netinet/in.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/file.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <cassert>
#include <csignal>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <cstdint>
#include <uv.h>

// System type definitions
typedef unsigned char UCHAR;
typedef unsigned int UINT;
typedef std::uint16_t USHORT;
typedef std::uint64_t ULONG;

// System macro definitions
#define INVALID_SOCKET    -1        // Invalid socket handle
#define IP_LENGTH         20        // IP address length
#define RECV_BUF_LEN      16384     // Buffer size for receiving client information

// System constant parameter definitions
const USHORT CONNECT_PORT = 8000;  // Connection port

// Number of test connections for tcpclient
const int MAX_CLIENT_CONN = 100;

const size_t MAX_BUFFER_SIZE = 65536;  // Maximum buffer size for reading and writing

// Maximum number of connections handled by tcpsvr
const int MAX_SOCKET_NUM = 200;

const int SOCK_RECV_BUFFER = 512*1024;
const int SOCK_SEND_BUFFER = 512*1024;
const int STR_COMM_LEN = 128;
const int LISTEN_BACKLOG = 512;

// Shared memory key for client usage
const int CLIENT_SHM_KEY = 1110;

// Shared memory key for storing socket connections
const int SOCKET_SHM_KEY = 1111;

// Size of the length field in the CS communication package header
const int PKGHEAD_FIELD_SIZE = sizeof(int);
// Minimum length of CS communication package
const int MIN_CSPKG_LEN = 10;
// Maximum length of CS communication package
const int MAX_CSPKG_LEN = RECV_BUF_LEN;
// Temporary buffer size for CS packaging
const int CSPKG_OPT_BUFFSIZE = RECV_BUF_LEN*2;

// Maximum number of message packages retrieved from the message queue at once by tcpsvr
const int MAX_SEND_PKGNUM = 512;

// Client timeout in seconds
const int CLIENT_TIMEOUT = 300;  // 5 minutes

// System common data structure definitions

// Mode for creating shared memory
enum ShmMode {
    MODE_NONE = -1,
    MODE_INIT = 0,    // New, i.e., no shared memory was created before
    MODE_RESUME = 1,  // Resume, i.e., shared memory already existed, resuming use
    MODE_MAX
};

// Communication server running type
enum SvrRunFlag {
    RUN_INIT = 0,
    RELOAD_CFG = 1,
    TCP_EXIT = 2,
};

// Network connection error code definitions
enum SocketErrors {
    ERROR_OK              =      0,   // Normal processing
    ERROR_CLIENT_CLOSE    =     -1,   // Client closed
    ERROR_ClIENT_TIMEOUT  =     -2,   // Client timeout
    ERROR_WRITE_BUFFOVER  =     -3,   // TCP write buffer is full
    ERROR_READ_BUFFEMPTY  =     -4,   // TCP read buffer is empty
    ERROR_PACKET_INVALID  =     -5,   // Invalid package sent by client
};

// Socket structure for communication between tcpsvr and client
struct SocketConnInfo {
    uv_tcp_t* handle;  // libuv handle
    ULONG uin;         // User account
    int recv_bytes;    // Number of bytes received
    char recv_buf[RECV_BUF_LEN];  // Buffer for received client request package
    ULONG client_ip;   // Client IP address
    time_t create_Time;  // Socket creation time
    time_t recv_data_time;  // Timestamp of received data package
};

// Package header for communication between tcpsvr and gamesvr
struct CSPkgHead {
    int pkg_size;    // Total CS package length
    int fd;          // CS communication socket
    ULONG client_ip; // Client IP address, for gamesvr to use in some statistics during operation
};

// Statistics interval time
const int STAT_TIME = 20;

// Statistical information
struct StatInfo {
    int fd;
    int count;
};

#endif  // _TRADING_PLATFORM_COMMON_TCP_COMM_H_