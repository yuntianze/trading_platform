#ifndef TCP_CLIENT_H
#define TCP_CLIENT_H

#include <uv.h>
#include <string>
#include <vector>
#include <memory>
#include "tcp_comm.h"

// Connection IP and Port
const char CONNECT_IP[] = "140.238.154.0";

class TcpClient {
public:
    // Constructor and destructor
    TcpClient(uv_loop_t* loop, const char* ip, int port);
    ~TcpClient();

    // Initialize the client
    int init();

    // Run the client
    void run();

    // Send data to server
    int send_data(const char* data, size_t len);

private:
    // Callback for connection
    static void on_connect(uv_connect_t* req, int status);

    // Callback for reading data
    static void on_read(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buf);

    // Callback for writing data
    static void on_write(uv_write_t* req, int status);

    // Callback for allocating buffer
    static void alloc_buffer(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf);

    // Send account login request
    int send_account_login_req(uint32_t uin);

    // Process account login response
    void process_account_login_res(const char* data, size_t len);

    // Send futures order
    int send_futures_order();

    // Process order response
    void process_order_response(const char* data, size_t len);

    uv_loop_t* loop_;
    uv_tcp_t client_;
    std::unique_ptr<uv_connect_t> connect_req_;  // Added unique_ptr for connect request
    std::string server_ip_;
    int server_port_;
    char read_buf_[MAX_BUFFER_SIZE];
    std::vector<std::unique_ptr<uv_write_t>> write_reqs_;
    bool is_logged_in_;  // Flag to track login status
    bool login_response_received_;  // Flag to track if login response is received
};

#endif // TCP_CLIENT_H