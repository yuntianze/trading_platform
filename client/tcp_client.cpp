#include "tcp_client.h"
#include <cstring>
#include "tcp_code.h"
#include "logger.h"

// Initialize the static member
MultiUserSimulator* MultiUserSimulator::current_instance = nullptr;

TcpClient::TcpClient(uv_loop_t* loop, const char* ip, int port, uint32_t uin)
    : loop_(loop), server_ip_(ip), server_port_(port), is_logged_in_(false), 
      login_response_received_(false), uin_(uin) {
    uv_tcp_init(loop_, &client_);
    client_.data = this;
}

TcpClient::~TcpClient() {
    if (!uv_is_closing((uv_handle_t*)&client_)) {
        uv_close((uv_handle_t*)&client_, nullptr);
    }
}

int TcpClient::init() {
    struct sockaddr_in dest;
    uv_ip4_addr(server_ip_.c_str(), server_port_, &dest);

    connect_req_ = std::make_unique<uv_connect_t>();
    connect_req_->data = this;

    connect_start_time_ = std::chrono::steady_clock::now();
    return uv_tcp_connect(connect_req_.get(), &client_, (const struct sockaddr*)&dest, on_connect);
}

void TcpClient::run() {
    // This method is now empty as the loop is managed by MultiUserSimulator
}

void TcpClient::on_connect(uv_connect_t* req, int status) {
    TcpClient* client = static_cast<TcpClient*>(req->data);

    if (status < 0) {
        LOG(ERROR, "Connection failed for client {}: {}", client->uin_, uv_strerror(status));
        client->retry_connect();
        return;
    }

    auto end_time = std::chrono::steady_clock::now();
    double connection_time = std::chrono::duration<double, std::milli>(end_time - client->connect_start_time_).count();
    client->connection_time_ = connection_time;  // Store the connection time
    LOG(INFO, "Client {} connected to server. Connection time: {:.2f}ms", client->uin_, connection_time);

    // Start reading from the server
    uv_read_start((uv_stream_t*)&client->client_, alloc_buffer, on_read);

    // Send login request
    client->login_request_time_ = std::chrono::steady_clock::now();
    client->send_account_login_req();
}

void TcpClient::on_read(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buf) {
    TcpClient* client = static_cast<TcpClient*>(stream->data);

    if (nread > 0) {
        std::string received_data(buf->base, nread);
        std::unique_ptr<google::protobuf::Message> msg(TcpCode::decode(received_data));
        
        if (msg) {
            if (dynamic_cast<cspkg::AccountLoginRes*>(msg.get())) {
                client->process_account_login_res(buf->base, nread);
            } else if (dynamic_cast<cs_proto::OrderResponse*>(msg.get())) {
                client->process_order_response(buf->base, nread);
            } else {
                LOG(ERROR, "Unknown message type received for client {}", client->uin_);
            }
        } else {
            LOG(ERROR, "Failed to decode message for client {}", client->uin_);
        }
    } else if (nread < 0) {
        if (nread != UV_EOF) {
            LOG(ERROR, "Read error for client {}: {}", client->uin_, uv_strerror(nread));
        }
        uv_close((uv_handle_t*)stream, nullptr);
    }
}

void TcpClient::on_write(uv_write_t* req, int status) {
    TcpClient* client = static_cast<TcpClient*>(req->data);
    if (client == nullptr || status < 0) {
        LOG(ERROR, "Write error for client {}: {}", client ? client->uin_ : 0, uv_strerror(status));
    } else {
        LOG(INFO, "Data sent successfully for client {}", client->uin_);
    }
    delete req;
}

void TcpClient::alloc_buffer(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf) {
    (void)suggested_size;
    TcpClient* client = static_cast<TcpClient*>(handle->data);
    buf->base = client->read_buf_;
    buf->len = sizeof(client->read_buf_);
}

int TcpClient::send_data(const char* data, size_t len) {
    uv_buf_t buf = uv_buf_init(const_cast<char*>(data), len);
    uv_write_t* req = new uv_write_t;
    req->data = this;

    int result = uv_write(req, (uv_stream_t*)&client_, &buf, 1, on_write);
    if (result < 0) {
        LOG(ERROR, "Failed to send data for client {}: {}", uin_, uv_strerror(result));
        delete req;
        return result;
    }
    return 0;
}

int TcpClient::send_account_login_req() {
    cspkg::AccountLoginReq acc_login_req;
    acc_login_req.set_account(uin_);
    std::string session = "Session key for client " + std::to_string(uin_);
    acc_login_req.set_session_key(session);

    std::string pkg = TcpCode::encode(acc_login_req);
    if (pkg.empty()) {
        return -1;
    }

    LOG(INFO, "Sending account login request: account={}", uin_);

    int result = send_data(pkg.c_str(), pkg.size());
    if (result == 0) {
        requests_sent_++;
    }
    return result;
}

void TcpClient::process_account_login_res(const char* data, size_t len) {
    std::string buf(data, len);
    std::unique_ptr<google::protobuf::Message> msg(TcpCode::decode(buf));
    if (!msg) {
        LOG(ERROR, "Failed to decode login response for client {}", uin_);
        return;
    }

    const cspkg::AccountLoginRes* acc_login_res = dynamic_cast<const cspkg::AccountLoginRes*>(msg.get());
    if (!acc_login_res) {
        LOG(ERROR, "Message is not AccountLoginRes for client {}", uin_);
        return;
    }

    auto end_time = std::chrono::steady_clock::now();
    double login_time = std::chrono::duration<double, std::milli>(end_time - login_request_time_).count();
    login_time_ = login_time;  // Store the login time
    LOG(INFO, "Received account login response: account={}, result={}, time={:.2f}ms", 
                acc_login_res->account(), acc_login_res->result(), login_time);

    login_response_received_ = true;

    if (acc_login_res->result() == 0) {  // Assuming 0 means success
        is_logged_in_ = true;
        LOG(INFO, "Login successful for client {}, sending futures order", uin_);
        send_futures_order();
    } else {
        LOG(ERROR, "Login failed for client {}", uin_);
    }
    responses_received_++;
}

int TcpClient::send_futures_order() {
    if (!is_logged_in_ || !login_response_received_) {
        LOG(ERROR, "Cannot send futures order: not logged in or login response not received for client {}", uin_);
        return -1;
    }

    cs_proto::FuturesOrder order = generate_random_order();
    std::string pkg = TcpCode::encode(order);
    if (pkg.empty()) {
        return -1;
    }

    order_request_time_ = std::chrono::steady_clock::now();
    int result = send_data(pkg.c_str(), pkg.size());
    if (result == 0) {
        requests_sent_++;
    }
    return result;
}

void TcpClient::process_order_response(const char* data, size_t len) {
    std::string buf(data, len);
    std::unique_ptr<google::protobuf::Message> msg(TcpCode::decode(buf));
    if (!msg) {
        LOG(ERROR, "Failed to decode order response for client {}", uin_);
        return;
    }

    const cs_proto::OrderResponse* order_res = dynamic_cast<const cs_proto::OrderResponse*>(msg.get());
    if (!order_res) {
        LOG(ERROR, "Message is not OrderResponse for client {}", uin_);
        return;
    }

    responses_received_++;

    auto end_time = std::chrono::steady_clock::now();
    double order_time = std::chrono::duration<double, std::milli>(end_time - order_request_time_).count();
    order_time_ = order_time;  // Store the order time
    LOG(INFO, "Received order response for client {}: order_id={}, status={}, message={}, time={:.2f}ms", 
                uin_, order_res->order_id(), cs_proto::OrderStatus_Name(order_res->status()), order_res->message(), order_time);
}

cs_proto::FuturesOrder TcpClient::generate_random_order() {
    cs_proto::FuturesOrder order;
    order.set_order_id("ord" + std::to_string(uin_));
    order.set_user_id("user" + std::to_string(uin_));
    order.set_symbol("BTCUSD");
    order.set_side(dis_(gen_) == 0 ? cs_proto::OrderSide::BUY : cs_proto::OrderSide::SELL);
    order.set_type(static_cast<cs_proto::OrderType>(dis_(gen_) % 4));
    order.set_quantity(1.0 + (dis_(gen_) % 10));
    order.set_price(45000.0 + (dis_(gen_) % 10000));
    order.set_status(cs_proto::OrderStatus::PENDING);
    order.set_timestamp(std::time(nullptr));
    order.set_client_id(uin_);
    return order;
}

void TcpClient::retry_connect() {
    if (retry_count_ < MAX_RETRY_ATTEMPTS) {
        retry_count_++;
        LOG(INFO, "Retrying connection for client {} (Attempt {})", uin_, retry_count_);
        std::this_thread::sleep_for(std::chrono::seconds(1));  // Wait before retrying
        init();
    } else {
        LOG(ERROR, "Max retry attempts reached for client {}", uin_);
    }
}

// MultiUserSimulator implementation

MultiUserSimulator::MultiUserSimulator(uv_loop_t* loop, const std::string& config_file)
    : loop_(loop), gen_(rd_()), dis_(10000, 99999), running_(true) {
    load_config(config_file);
    clients_.reserve(num_users_);
    for (int i = 0; i < num_users_; ++i) {
        uint32_t uin = dis_(gen_);
        clients_.push_back(std::make_unique<TcpClient>(loop, server_ip_.c_str(), server_port_, uin));
    }
    current_instance = this;

    // Initialize the timer
    uv_timer_init(loop, &report_timer_);
    report_timer_.data = this;
}

MultiUserSimulator::~MultiUserSimulator() = default;

int MultiUserSimulator::init() {
    for (auto& client : clients_) {
        if (client->init() != 0) {
            LOG(ERROR, "Failed to initialize client {}", client->get_uin());
            return -1;
        }
    }
    return 0;
}

void MultiUserSimulator::run() {
    start_periodic_report();
    while (running_) {
        uv_run(loop_, UV_RUN_ONCE);
    }
    report_performance_metrics();
}

void MultiUserSimulator::stop() {
    running_ = false;
}

void MultiUserSimulator::signal_handler(int signal) {
    LOG(INFO, "Received signal: {}", signal);
    if (current_instance) {
        current_instance->stop();
    }
}

void MultiUserSimulator::on_timer(uv_timer_t* handle) {
    MultiUserSimulator* simulator = static_cast<MultiUserSimulator*>(handle->data);
    simulator->report_performance_metrics();
    // simulator->reset_counters();
}

void MultiUserSimulator::start_periodic_report() {
    uv_timer_start(&report_timer_, on_timer, 60000, 60000);  // 60000 ms = 1 minute
}

void MultiUserSimulator::reset_counters() {
    for (auto& client : clients_) {
        // Reset client-specific counters if needed
        client->reset_counters();
    }
    connection_times_.clear();
    login_response_times_.clear();
    order_response_times_.clear();
    total_requests_sent_ = 0;
    total_responses_received_ = 0;
}

void MultiUserSimulator::load_config(const std::string& config_file) {
    std::ifstream file(config_file);
    if (!file.is_open()) {
        throw std::runtime_error("Unable to open config file: " + config_file);
    }

    nlohmann::json config;
    file >> config;

    server_ip_ = config["server"]["ip"];
    server_port_ = config["server"]["port"];
    num_users_ = config["simulation"]["num_users"];
    max_retry_attempts_ = config["simulation"]["max_retry_attempts"];
    retry_delay_ms_ = config["simulation"]["retry_delay_ms"];

    // Set logging level and file
    std::string log_level = config["logging"]["level"];
    std::string log_file = config["logging"]["file"];
    // Note: You may need to implement these methods in your Logger class
    // Logger::set_level(log_level);
    // Logger::set_file(log_file);

    collect_metrics_ = config["performance"]["collect_metrics"];
    report_interval_ms_ = config["performance"]["report_interval_ms"];
}

void MultiUserSimulator::report_performance_metrics() {
    if (!collect_metrics_) {
        LOG(INFO, "Performance metrics collection is disabled");
        return;
    }

    // Get current timestamp
    auto now = std::chrono::system_clock::now();
    auto now_c = std::chrono::system_clock::to_time_t(now);

    // Clear previous data
    connection_times_.clear();
    login_response_times_.clear();
    order_response_times_.clear();

    // Collect timing data from all clients
    for (const auto& client : clients_) {
        if (client->connection_time_ > 0) {
            connection_times_.push_back(client->connection_time_);
        }
        if (client->login_time_ > 0) {
            login_response_times_.push_back(client->login_time_);
        }
        if (client->order_time_ > 0) {
            order_response_times_.push_back(client->order_time_);
        }
    }

    // Calculate averages
    double avg_connection_time = connection_times_.empty() ? 0.0 : 
        std::accumulate(connection_times_.begin(), connection_times_.end(), 0.0) / connection_times_.size();
    double avg_login_response_time = login_response_times_.empty() ? 0.0 : 
        std::accumulate(login_response_times_.begin(), login_response_times_.end(), 0.0) / login_response_times_.size();
    double avg_order_response_time = order_response_times_.empty() ? 0.0 : 
        std::accumulate(order_response_times_.begin(), order_response_times_.end(), 0.0) / order_response_times_.size();

    // Calculate total requests and responses
    total_requests_sent_ = 0;
    total_responses_received_ = 0;
    for (const auto& client : clients_) {
        total_requests_sent_ += client->get_requests_sent();
        total_responses_received_ += client->get_responses_received();
    }

    LOG(INFO, "Performance Metrics at {}", std::ctime(&now_c));
    LOG(INFO, "Connection Time: avg {:.2f}ms (from {} samples)", avg_connection_time, connection_times_.size());
    LOG(INFO, "Login Response Time: avg {:.2f}ms (from {} samples)", avg_login_response_time, login_response_times_.size());
    LOG(INFO, "Order Response Time: avg {:.2f}ms (from {} samples)", avg_order_response_time, order_response_times_.size());
    LOG(INFO, "Total Requests Sent: {}", total_requests_sent_);
    LOG(INFO, "Total Responses Received: {}", total_responses_received_);

    if (total_requests_sent_ != total_responses_received_) {
        LOG(ERROR, "Mismatch between requests sent and responses received!");
    }
}

// Main function
int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <config_file>" << std::endl;
        return 1;
    }

    Logger::init("../log/tcpclient.log");

    uv_loop_t* loop = uv_default_loop();
    MultiUserSimulator simulator(loop, argv[1]);

    if (simulator.init() != 0) {
        LOG(ERROR, "Failed to initialize multi-user simulator");
        return 1;
    }

    LOG(INFO, "Multi-user simulator initialized successfully");

    simulator.run();

    return 0;
}