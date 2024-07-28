/*************************************************************************
 * @file   tcp_connect_mgr.h
 * @brief  TCP connection manager class declaration
 * @author stanjiang
 * @date   2024-07-17
 * @copyright
***/

#ifndef TRADING_PLATFORM_COMMON_TCP_CONNECT_MGR_H_
#define TRADING_PLATFORM_COMMON_TCP_CONNECT_MGR_H_

#include <uv.h>
#include "tcp_comm.h"

class TcpConnectMgr {
public:
    TcpConnectMgr();
    ~TcpConnectMgr();

    // Create an instance of TcpConnectMgr
    static TcpConnectMgr* create_instance();

    // Calculate the size needed for the connection manager
    static int count_size();

    // Overload new operator to allocate memory in shared memory
    static void* operator new(size_t size);

    // Overload delete operator to free memory in shared memory
    static void operator delete(void* mem);

    // Initialize the TCP connection manager
    int init();

    // Handle a new connection
    void handle_new_connection(uv_tcp_t* client);

    // Process received client data
    int process_client_data(uv_stream_t* client, const char* data, ssize_t nread);

    // Check for data waiting to be sent
    void check_wait_send_data();

    // Check for timed-out connections
    void check_timeout();

    // Static callback for reading data from a client
    static void on_read(uv_stream_t* client, ssize_t nread, const uv_buf_t* buf);

    // Static callback for allocating buffer for reading
    static void alloc_buffer(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf);

    // Static callback for write completion
    static void on_write(uv_write_t* req, int status);

    // Send data to a client
    static int tcp_send_data(uv_stream_t* client, const char* databuf, int len);

private:
    static char* current_shmptr_;  // Pointer to the shared memory
    SocketConnInfo client_sockconn_list_[MAX_SOCKET_NUM];  // List of client socket connections
    char send_client_buf_[SOCK_SEND_BUFFER];  // Buffer for sending messages to clients

    int cur_conn_num_;   // Current number of connections
    int send_pkg_count_;  // Count of sent packages
    int recv_pkg_count_;  // Count of received packages
    time_t laststat_time_;   // Last statistics time
};

#endif // TRADING_PLATFORM_COMMON_TCP_CONNECT_MGR_H_



