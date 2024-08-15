#include "tcp_connect_mgr.h"
#include <algorithm>
#include "shm_mgr.h"
#include "tcp_code.h"
#include "kafka_manager.h"
#include "logger.h"
#include "config_manager.h"

// Implementation of StatisticsManager

StatisticsManager::StatisticsManager()
    : sent_packages_(0), received_packages_(0), active_connections_(0),
      total_connections_(0), total_connection_time_(0), total_processing_time_(0),
      last_reset_time_(std::chrono::steady_clock::now()) {}

void StatisticsManager::increment_sent_packages() {
    sent_packages_++;
}

void StatisticsManager::increment_received_packages() {
    received_packages_++;
}

void StatisticsManager::increment_active_connections() {
    active_connections_++;
    total_connections_++;
}

void StatisticsManager::decrement_active_connections() {
    if (active_connections_ > 0) {
        active_connections_--;
    }
}

void StatisticsManager::update_connection_time(double time_ms) {
    double old_value = total_connection_time_.load(std::memory_order_relaxed);
    double new_value = old_value + time_ms;
    while (!total_connection_time_.compare_exchange_weak(old_value, new_value,
                                                         std::memory_order_release,
                                                         std::memory_order_relaxed)) {
        new_value = old_value + time_ms;
    }
}

void StatisticsManager::update_processing_time(double time_ms) {
    double old_value = total_processing_time_.load(std::memory_order_relaxed);
    double new_value = old_value + time_ms;
    while (!total_processing_time_.compare_exchange_weak(old_value, new_value,
                                                         std::memory_order_release,
                                                         std::memory_order_relaxed)) {
        new_value = old_value + time_ms;
    }
}

void StatisticsManager::reset() {
    sent_packages_ = 0;
    received_packages_ = 0;
    total_connections_ = active_connections_.load();
    total_connection_time_ = 0;
    total_processing_time_ = 0;
    last_reset_time_ = std::chrono::steady_clock::now();
}

double StatisticsManager::calculate_rate(uint64_t count, double elapsed_seconds) const {
    return elapsed_seconds > 0 ? count / elapsed_seconds : 0;
}

void StatisticsManager::log_statistics() {
    auto now = std::chrono::steady_clock::now();
    double elapsed_seconds = std::chrono::duration<double>(now - last_reset_time_).count();

    double sent_rate = calculate_rate(sent_packages_, elapsed_seconds);
    double received_rate = calculate_rate(received_packages_, elapsed_seconds);
    double avg_connection_time = total_connections_ > 0 ? total_connection_time_ / total_connections_ : 0;
    double avg_processing_time = received_packages_ > 0 ? total_processing_time_ / received_packages_ : 0;

    LOG(INFO, "Gateway Server Statistics:");
    LOG(INFO, "  Elapsed time: {:.2f} seconds", elapsed_seconds);
    LOG(INFO, "  Sent packages: {} (Rate: {:.2f} pkg/s)", sent_packages_.load(), sent_rate);
    LOG(INFO, "  Received packages: {} (Rate: {:.2f} pkg/s)", received_packages_.load(), received_rate);
    LOG(INFO, "  Active connections: {}", active_connections_.load());
    LOG(INFO, "  Total connections: {}", total_connections_.load());
    LOG(INFO, "  Average connection time: {:.2f} ms", avg_connection_time);
    LOG(INFO, "  Average processing time: {:.2f} ms", avg_processing_time);
}

// Implementation of TcpConnectMgr

char* TcpConnectMgr::current_shmptr_ = nullptr;

TcpConnectMgr::TcpConnectMgr() :
    cur_conn_num_(0),
    laststat_time_(0),
    next_index_(0) {
}

TcpConnectMgr::~TcpConnectMgr() {
    LOG(INFO, "TcpConnectMgr destroyed");
}

int TcpConnectMgr::add_new_connection(uv_tcp_t* client) {
    if (cur_conn_num_ >= MAX_SOCKET_NUM) {
        return -1;  // No more slots available
    }
    int index = next_index_++;
    if (next_index_ >= MAX_SOCKET_NUM) {
        next_index_ = 0;  // Wrap around to reuse slots
    }
    client_to_index_[client] = index;
    ++cur_conn_num_;
    return index;
}

void TcpConnectMgr::handle_new_connection(uv_tcp_t* client) {
    // Add a new connection and get its index
    int index = add_new_connection(client);
    if (index == -1) {
        LOG(ERROR, "Maximum number of connections reached or no available slot");
        uv_close((uv_handle_t*)client, [](uv_handle_t* handle) { free(handle); });
        return;
    }

    // Initialize client information
    client_sockconn_list_[index].handle = client;
    time(&client_sockconn_list_[index].create_Time);
    client_sockconn_list_[index].recv_bytes = 0;
    client_sockconn_list_[index].buf_start = 0;  // Initialize buffer start position
    client_sockconn_list_[index].recv_data_time = 0;
    client_sockconn_list_[index].uin = 0;

    // Get peer address
    struct sockaddr_storage peer_addr;
    int addr_len = sizeof(peer_addr);
    char addr[32] = {'\0'};
    if (uv_tcp_getpeername(client, (struct sockaddr*)&peer_addr, &addr_len) == 0) {
        uv_ip4_name((struct sockaddr_in*)&peer_addr, addr, sizeof(addr));
        client_sockconn_list_[index].client_ip = inet_addr(addr);
    } else {
        LOG(ERROR, "Failed to get peer name");
    }

    // Set the data pointer of the uv_tcp_t to the index in our array
    client->data = (void*)(intptr_t)index;

    // Start reading from the client
    int read_start_result = uv_read_start((uv_stream_t*)client, alloc_buffer, on_read);
    if (read_start_result != 0) {
        LOG(ERROR, "Failed to start reading from client: {}", uv_strerror(read_start_result));
        uv_close((uv_handle_t*)client, [](uv_handle_t* handle) { free(handle); });
        return;
    }

    // Increment active connections count
    stats_manager_.increment_active_connections();

    LOG(INFO, "Handle new connection, index:{}, client ip:{}, total connections: {}",
            index, addr, cur_conn_num_);
}

void TcpConnectMgr::alloc_buffer(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf) {
    // Get the TcpConnectMgr instance
    TcpConnectMgr* mgr = static_cast<TcpConnectMgr*>(handle->loop->data);

    // Get the index from the handle's data
    int index = (int)(intptr_t)handle->data;
    SocketConnInfo& conn = mgr->client_sockconn_list_[index];

    // Calculate the end position of data in the circular buffer
    int buf_end = (conn.buf_start + conn.recv_bytes) % RECV_BUF_LEN;

    // Calculate available space
    size_t available_space;
    if (buf_end >= conn.buf_start) {
        available_space = RECV_BUF_LEN - buf_end;
    } else {
        available_space = conn.buf_start - buf_end;
    }

    // Allocate buffer based on available space and suggested size
    size_t alloc_size = std::min(available_space, suggested_size);

    if (alloc_size > 0) {
        buf->base = conn.recv_buf + buf_end;
        buf->len = alloc_size;
    } else {
        buf->base = (char*)malloc(1);
        buf->len = 0;
    }

    LOG(DEBUG, "Buffer allocated for client {}: size {}", index, buf->len);
}

TcpConnectMgr* TcpConnectMgr::create_instance() {
    int shm_key = ConfigManager::instance().get_int("SOCKET_SHM_KEY");
    int shm_size = count_size();
    int assign_size = shm_size;
    current_shmptr_ = static_cast<char*>(ShmMgr::instance().create_shm(shm_key, shm_size, assign_size));

    return new TcpConnectMgr();
}

int TcpConnectMgr::count_size() {
    return sizeof(TcpConnectMgr);
}

void* TcpConnectMgr::operator new(size_t size) {
    (void)size;  // Unused
    return static_cast<void*>(current_shmptr_);
}

void TcpConnectMgr::operator delete(void* mem) {
    (void)mem;
    // Do nothing, as memory is managed in shared memory
}

int TcpConnectMgr::init() {
    // Initialize connection-related variables
    laststat_time_ = 0;
    cur_conn_num_ = 0;

    client_sockconn_list_.resize(MAX_SOCKET_NUM);
    for (int i = 0; i < MAX_SOCKET_NUM; ++i) {
        client_sockconn_list_[i].handle = nullptr;
        client_sockconn_list_[i].recv_bytes = 0;
        client_sockconn_list_[i].buf_start = 0;
    }

    gateway_to_order_topic_ = ConfigManager::instance().get_string("GATEWAY_TO_ORDER_TOPIC");

    LOG(INFO, "TcpConnectMgr initialized successfully");
    return 0;
}

void TcpConnectMgr::on_read(uv_stream_t* client, ssize_t nread, const uv_buf_t* buf) {
    TcpConnectMgr* mgr = static_cast<TcpConnectMgr*>(client->loop->data);
    int index = (int)(intptr_t)client->data;

    if (index < 0 || index >= MAX_SOCKET_NUM) {
        LOG(ERROR, "Invalid client index: {}", index);
        uv_close((uv_handle_t*)client, [](uv_handle_t* handle) { free(handle); });
        return;
    }

    SocketConnInfo& conn = mgr->client_sockconn_list_[index];

    if (nread > 0) {
        LOG(DEBUG, "Read {} bytes from client {}", nread, index);
        
        // Update the received bytes count
        conn.recv_bytes += nread;
        
        // Process the received data
        mgr->process_client_data(client, nread);
    } else if (nread < 0) {
        if (nread != UV_EOF) {
            LOG(ERROR, "Read error for client {}: {}", index, uv_strerror(nread));
        } else {
            LOG(INFO, "Client {} disconnected", index);
        }

        // Close the client connection
        uv_close((uv_handle_t*)client, [](uv_handle_t* handle) {
            TcpConnectMgr* mgr = static_cast<TcpConnectMgr*>(handle->loop->data);
            int index = (int)(intptr_t)handle->data;
            LOG(INFO, "Connection closed. Client index{}, Total connections: {}", index, mgr->cur_conn_num_);
            mgr->remove_connection((uv_tcp_t*)handle);
            free(handle);
        });
    }

    // Free the buffer if it was dynamically allocated
    if (buf->base && buf->len == 0) {
        free(buf->base);
    }
}

int TcpConnectMgr::process_client_data(uv_stream_t* client, ssize_t nread) {
    int index = get_index_for_client((uv_tcp_t*)client);
    if (index < 0 || index >= MAX_SOCKET_NUM) {
        LOG(ERROR, "Invalid client index: {}", index);
        uv_close((uv_handle_t*)client, [](uv_handle_t* handle) {
            free(handle);
        });
        return -1;
    }

    // Start processing time
    auto start_time = std::chrono::steady_clock::now();

    LOG(DEBUG, "Processing {} bytes from client {}", nread, index);

    SocketConnInfo& cur_conn = client_sockconn_list_[index];

    // Update receive time
    time(&cur_conn.recv_data_time);

    // Process complete packets
    int total_processed = 0;
    while (cur_conn.recv_bytes >= PKGHEAD_FIELD_SIZE) {
        int header_pos = (cur_conn.buf_start + total_processed) % RECV_BUF_LEN;
        int packet_size = TcpCode::convert_int32(cur_conn.recv_buf + header_pos);

        LOG(INFO, "Header pos:{}, Packet size: {}", header_pos, packet_size);

        if (packet_size <= 0 || packet_size > MAX_CSPKG_LEN) {
            LOG(ERROR, "Invalid packet size {} for client {}", packet_size, index);
            uv_close((uv_handle_t*)client, [](uv_handle_t* handle) {
                free(handle);
            });
            return -1;
        }

        if (cur_conn.recv_bytes >= packet_size) {
            std::string message(cur_conn.recv_buf + header_pos, packet_size);
            
            std::unique_ptr<google::protobuf::Message> parsed_message(TcpCode::decode(message));
            if (parsed_message) {
                if (const auto* login_req = dynamic_cast<const cspkg::AccountLoginReq*>(parsed_message.get())) {
                    // Handle login request
                    LOG(INFO, "Received AccountLoginReq from client {}, account {}", index, login_req->account());
                    handle_login_request(client, *login_req, index);
                } else if (const auto* order = dynamic_cast<const cs_proto::FuturesOrder*>(parsed_message.get())) {
                    // Handle futures order
                    LOG(INFO, "Received FuturesOrder from client {}", index);
                    handle_futures_order(client, *order, index);
                } else {
                    LOG(ERROR, "Unknown message type for client {}", index);
                }
            } else {
                LOG(ERROR, "Failed to parse client message for client {}", index);
            }

            total_processed += packet_size;
            cur_conn.recv_bytes -= packet_size;
        } else {
            LOG(DEBUG, "Incomplete packet, waiting for more data");
            break;
        }
    }

    cur_conn.buf_start = (cur_conn.buf_start + total_processed) % RECV_BUF_LEN;

    // Update statistics
    auto end_time = std::chrono::steady_clock::now();
    double processing_time = std::chrono::duration<double, std::milli>(end_time - start_time).count();
    stats_manager_.update_processing_time(processing_time);
    stats_manager_.increment_received_packages();

    LOG(INFO, "Processed {} bytes from client {}", total_processed, index);
    return 0;
}

void TcpConnectMgr::handle_login_request(uv_stream_t* client, const cspkg::AccountLoginReq& login_req, int client_index) {
    (void)client;  // Unused
    // Store the account to index mapping
    account_to_index_[login_req.account()] = client_index;

    // Forward the login request to order_server via Kafka
    if (KafkaManager::instance().produce(gateway_to_order_topic_, login_req, client_index)) {
        LOG(INFO, "Sent AccountLoginReq to Kafka for client:{}, topic:{}", client_index, gateway_to_order_topic_);
    } else {
        LOG(ERROR, "Failed to send AccountLoginReq to Kafka for client {}", client_index);
    }
}

void TcpConnectMgr::handle_futures_order(uv_stream_t* client, const cs_proto::FuturesOrder& order, int client_index) {
    (void)client;  // Unused
    if (KafkaManager::instance().produce(gateway_to_order_topic_, order, client_index)) {
        LOG(INFO, "Sent FuturesOrder to Kafka for client {}, topic {}", client_index, gateway_to_order_topic_);
    } else {
        LOG(ERROR, "Failed to send FuturesOrder to Kafka for client {}", client_index);
    }
}

int TcpConnectMgr::tcp_send_data(uv_stream_t* client, const char* databuf, int len) {
    uv_buf_t buffer = uv_buf_init((char*)databuf, len);
    uv_write_t* req = (uv_write_t*)malloc(sizeof(uv_write_t));
    req->data = client->data;  // Store client index in write request
    
    return uv_write(req, client, &buffer, 1, on_write);
}

void TcpConnectMgr::on_write(uv_write_t* req, int status) {
    TcpConnectMgr* mgr = static_cast<TcpConnectMgr*>(req->handle->loop->data);
    int client_index = (int)(intptr_t)req->data;

    if (status < 0) {
        LOG(ERROR, "Write error for client {}: {}", client_index, uv_strerror(status));
    }
    else {
        LOG(DEBUG, "Write successful for client {}", client_index);
        // Update statistics
        mgr->stats_manager_.increment_sent_packages();
    }
    free(req);
}

void TcpConnectMgr::check_wait_send_data() {
    // TODO: Implement logic to check for data waiting to be sent
    // This might involve checking a queue or buffer of outgoing messages
}

void TcpConnectMgr::check_timeout() {
    static const int STATS_INTERVAL = 300;  // Log statistics every 300 seconds
    static time_t last_stats_time = 0;

    time_t current_time = time(NULL);
    
    // Update statistics
    if (current_time >= last_stats_time + STATS_INTERVAL) {
        stats_manager_.log_statistics();
        stats_manager_.reset();
        last_stats_time = current_time;
    }

    // Check for timed-out connections
    for (int i = 0; i < MAX_SOCKET_NUM; ++i) {
        if (client_sockconn_list_[i].handle != nullptr) {
            time_t last_activity = std::max(client_sockconn_list_[i].create_Time, 
                                            client_sockconn_list_[i].recv_data_time);
            if (current_time - last_activity > CLIENT_TIMEOUT) {
                LOG(INFO, "Client {} timed out", i);
                uv_handle_t* handle = (uv_handle_t*)client_sockconn_list_[i].handle;
                if (!uv_is_closing(handle)) {
                    uv_close(handle, [](uv_handle_t* handle) {
                        TcpConnectMgr* mgr = static_cast<TcpConnectMgr*>(handle->loop->data);
                        int index = (int)(intptr_t)handle->data;
                        LOG(INFO, "Closed handle for client {}", index);
                        mgr->remove_connection((uv_tcp_t*)handle);
                        free(handle);
                    });
                }
            }
        }
    }
}