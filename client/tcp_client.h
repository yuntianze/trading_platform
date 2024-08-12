#ifndef TCP_CLIENT_H
#define TCP_CLIENT_H

#include <uv.h>
#include <string>
#include <vector>
#include <memory>
#include <random>
#include <chrono>
#include <thread>
#include <fstream>
#include <csignal>
#include <nlohmann/json.hpp>
#include "tcp_comm.h"
#include "role.pb.h"
#include "futures_order.pb.h"

// Connection IP and Port
const char CONNECT_IP[] = "140.238.154.0";
const int CONNECT_PORT = 9218;

class TcpClient {
public:
    // Constructor and destructor
    TcpClient(uv_loop_t* loop, const char* ip, int port, uint32_t uin);
    ~TcpClient();

    // Initialize the client
    int init();

    // Run the client
    void run();

    // Send data to server
    int send_data(const char* data, size_t len);

    // Get the client's UIN
    uint32_t get_uin() const { return uin_; }

    // Generating random order
    cs_proto::FuturesOrder generate_random_order();

    // Automatic retry
    void retry_connect();

    // Add getters for request and response counters
    size_t get_requests_sent() const { return requests_sent_; }
    size_t get_responses_received() const { return responses_received_; }

    // Reset request and response counters
    void reset_counters() {
        requests_sent_ = 0;
        responses_received_ = 0;
    }

    // Performance metrics
    std::chrono::steady_clock::time_point connect_start_time_;
    std::chrono::steady_clock::time_point login_request_time_;
    std::chrono::steady_clock::time_point order_request_time_;

    double connection_time_ = 0.0;
    double login_time_ = 0.0;
    double order_time_ = 0.0;

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
    int send_account_login_req();

    // Process account login response
    void process_account_login_res(const char* data, size_t len);

    // Send futures order
    int send_futures_order();

    // Process order response
    void process_order_response(const char* data, size_t len);

    static constexpr int MAX_RETRY_ATTEMPTS = 3;

    uv_loop_t* loop_;
    uv_tcp_t client_;
    std::unique_ptr<uv_connect_t> connect_req_;
    std::string server_ip_;
    int server_port_;
    char read_buf_[MAX_BUFFER_SIZE];
    std::vector<std::unique_ptr<uv_write_t>> write_reqs_;
    bool is_logged_in_;
    bool login_response_received_;
    uint32_t uin_;  // Unique identifier for each client
    int retry_count_ = 0;
    std::mt19937 gen_{std::random_device{}()};
    std::uniform_int_distribution<> dis_{0, 1};  // For random side selection

    // Add counters for requests and responses
    size_t requests_sent_ = 0;
    size_t responses_received_ = 0;
};

// New class to manage multiple TcpClient instances
class MultiUserSimulator {
public:
    MultiUserSimulator(uv_loop_t* loop, const std::string& config_file);
    ~MultiUserSimulator();

    // Initialize all clients
    int init();

    // Run the simulation
    void run();

    // Load configuration
    void load_config(const std::string& config_file);

    // Collect and report performance metrics
    void report_performance_metrics();

    // Add a static method for signal handling
    static void signal_handler(int signal);

    // Add a method to stop the simulation
    void stop();

    // Add a method to start the periodic timer
    void start_periodic_report();

private:
    // Add a static pointer to the current instance
    static MultiUserSimulator* current_instance;

    // Add a static callback for the timer
    static void on_timer(uv_timer_t* handle);

    // Add a method to reset counters after reporting
    void reset_counters();

    uv_loop_t* loop_;
    std::string server_ip_;
    int server_port_;
    int num_users_;
    std::vector<std::unique_ptr<TcpClient>> clients_;
    std::random_device rd_;
    std::mt19937 gen_;
    std::uniform_int_distribution<> dis_;

    // Performance metrics
    std::vector<double> connection_times_;
    std::vector<double> login_response_times_;
    std::vector<double> order_response_times_;

    // Configuration
    int max_retry_attempts_;
    int retry_delay_ms_;
    bool collect_metrics_;
    int report_interval_ms_;

    // Add a timer for periodic reporting
    uv_timer_t report_timer_;

    // Add a flag to indicate if the simulation should continue running
    bool running_;

    // Add counters for total requests and responses
    size_t total_requests_sent_ = 0;
    size_t total_responses_received_ = 0;
};

#endif // TCP_CLIENT_H