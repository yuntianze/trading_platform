/*************************************************************************
 * @file   tcp_connect_mgr.h
 * @brief  TCP connection manager class declaration
 * @author stanjiang
 * @date   2024-07-17
 * @copyright
***/

#ifndef _TRADING_PLATFORM_COMMON_TCP_CONNECT_MGR_H_
#define _TRADING_PLATFORM_COMMON_TCP_CONNECT_MGR_H_

#include <uv.h>
#include <unordered_map>
#include <vector>
#include <string>
#include <chrono>
#include <atomic>
#include "tcp_comm.h"
#include "role.pb.h"
#include "futures_order.pb.h"

// Class to manage and log statistics for the TCP connection manager
class StatisticsManager {
public:
    StatisticsManager();

    void increment_sent_packages();
    void increment_received_packages();
    void increment_active_connections();
    void decrement_active_connections();
    void update_connection_time(double time_ms);
    void update_processing_time(double time_ms);

    void reset();
    void log_statistics();

private:
    std::atomic<uint64_t> sent_packages_;
    std::atomic<uint64_t> received_packages_;
    std::atomic<uint64_t> active_connections_;
    std::atomic<uint64_t> total_connections_;
    std::atomic<double> total_connection_time_;
    std::atomic<double> total_processing_time_;
    std::chrono::steady_clock::time_point last_reset_time_;

    // Helper function to calculate rate
    double calculate_rate(uint64_t count, double elapsed_seconds) const;
};

// Main TCP connection manager class
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
    int process_client_data(uv_stream_t* client, ssize_t nread);

    // Check for data waiting to be sent
    void check_wait_send_data();

    // Check for timed-out connections
    void check_timeout();

    // Get the index for a given client handle
    int get_index_for_client(uv_tcp_t* client);

    // Get the client handle for a given index
    uv_tcp_t* get_client_by_index(int index);

    // Get the client handle for a given account
    uv_tcp_t* get_client_by_account(uint32_t account);

    // Remove a client connection
    void remove_connection(uv_tcp_t* client);

    // Get the current number of connections
    size_t get_connection_count() const;

    // Static callback for reading data from a client
    static void on_read(uv_stream_t* client, ssize_t nread, const uv_buf_t* buf);

    // Static callback for allocating buffer for reading
    static void alloc_buffer(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf);

    // Static callback for write completion
    static void on_write(uv_write_t* req, int status);

    // Send data to a client
    static int tcp_send_data(uv_stream_t* client, const char* databuf, int len);

    // Get the statistics manager
    StatisticsManager& get_statistics_manager() { return stats_manager_; }

private:
    // Handle login request
    void handle_login_request(uv_stream_t* client, const cspkg::AccountLoginReq& login_req, int client_index);

    // Handle futures order
    void handle_futures_order(uv_stream_t* client, const cs_proto::FuturesOrder& order, int client_index);

    // Add a new client connection
    int add_new_connection(uv_tcp_t* client);

    static char* current_shmptr_;  // Pointer to the shared memory

    char send_client_buf_[SOCK_SEND_BUFFER];  // Buffer for sending messages to clients
    int cur_conn_num_;   // Current number of connections
    time_t laststat_time_;   // Last statistics time

    // Map to store client handle to index mapping
    std::unordered_map<uv_tcp_t*, int> client_to_index_;
    // Map to store account to index mapping
    std::unordered_map<uint32_t, int> account_to_index_;
    // Vector to store client connection information
    std::vector<SocketConnInfo> client_sockconn_list_;
    // Next available index for new connections
    int next_index_;
    // Kafka topic for gateway to order messages
    std::string gateway_to_order_topic_;

    // Statistics manager
    StatisticsManager stats_manager_;
};

// Implementation of inline methods

inline int TcpConnectMgr::get_index_for_client(uv_tcp_t* client) {
    auto it = client_to_index_.find(client);
    return (it != client_to_index_.end()) ? it->second : -1;
}

inline uv_tcp_t* TcpConnectMgr::get_client_by_index(int index) {
    if (index >= 0 && index < MAX_SOCKET_NUM) {
        return client_sockconn_list_[index].handle;
    }
    return nullptr;
}

inline void TcpConnectMgr::remove_connection(uv_tcp_t* client) {
    auto it = client_to_index_.find(client);
    if (it != client_to_index_.end()) {
        client_sockconn_list_[it->second] = SocketConnInfo();  // Reset the slot
        client_to_index_.erase(it);
        --cur_conn_num_;
        stats_manager_.decrement_active_connections();
    }
}

inline size_t TcpConnectMgr::get_connection_count() const {
    return cur_conn_num_;
}

inline uv_tcp_t* TcpConnectMgr::get_client_by_account(uint32_t account) {
    auto it = account_to_index_.find(account);
    if (it != account_to_index_.end()) {
        int index = it->second;
        return client_sockconn_list_[index].handle;
    }
    return nullptr;
}

#endif // _TRADING_PLATFORM_COMMON_TCP_CONNECT_MGR_H_