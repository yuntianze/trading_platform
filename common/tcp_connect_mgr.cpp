#include "tcp_connect_mgr.h"
#include <algorithm>
#include "shm_mgr.h"
#include "tcp_code.h"
#include "logger.h"

char* TcpConnectMgr::current_shmptr_ = nullptr;

TcpConnectMgr::TcpConnectMgr() :
    cur_conn_num_(0),
    send_pkg_count_(0),
    recv_pkg_count_(0),
    laststat_time_(0) {
    // Initialize socket list data
    for (int i = 0; i < MAX_SOCKET_NUM; ++i) {
        client_sockconn_list_[i].handle = nullptr;
    }
}

TcpConnectMgr::~TcpConnectMgr() {
    Logger::log(INFO, "TcpConnectMgr destroyed");
}

TcpConnectMgr* TcpConnectMgr::create_instance() {
    int shm_key = SOCKET_SHM_KEY;
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
    send_pkg_count_ = 0;
    recv_pkg_count_ = 0;
    laststat_time_ = 0;
    cur_conn_num_ = 0;

    Logger::log(INFO, "TcpConnectMgr initialized successfully");
    return 0;
}

void TcpConnectMgr::handle_new_connection(uv_tcp_t* client) {
    if (cur_conn_num_ >= MAX_SOCKET_NUM) {
        Logger::log(ERROR, "Maximum number of connections reached");
        uv_close((uv_handle_t*)client, [](uv_handle_t* handle) { free(handle); });
        return;
    }

    // Increment connection count
    ++cur_conn_num_;

    // Find an available slot in the connection list
    int index = -1;
    for (int i = 0; i < MAX_SOCKET_NUM; ++i) {
        if (client_sockconn_list_[i].handle == nullptr) {
            index = i;
            break;
        }
    }

    if (index == -1) {
        Logger::log(ERROR, "No available slot for new connection");
        uv_close((uv_handle_t*)client, [](uv_handle_t* handle) { free(handle); });
        --cur_conn_num_;
        return;
    }

    // Store client information
    client_sockconn_list_[index].handle = client;
    time(&client_sockconn_list_[index].create_Time);
    client_sockconn_list_[index].recv_bytes = 0;
    client_sockconn_list_[index].recv_data_time = 0;
    client_sockconn_list_[index].uin = 0;

    // Get peer address
    struct sockaddr_storage peer_addr;
    int addr_len = sizeof(peer_addr);
    if (uv_tcp_getpeername(client, (struct sockaddr*)&peer_addr, &addr_len) == 0) {
        char addr[17] = {'\0'};
        uv_ip4_name((struct sockaddr_in*)&peer_addr, addr, sizeof(addr));
        client_sockconn_list_[index].client_ip = inet_addr(addr);
    }

    // Set the data pointer of the uv_tcp_t to the index in our array
    client->data = (void*)(intptr_t)index;

    // Start reading from the client
    uv_read_start((uv_stream_t*)client, alloc_buffer, on_read);

    Logger::log(INFO, "New connection accepted. Total connections: {0:d}", cur_conn_num_);
}

void TcpConnectMgr::alloc_buffer(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf) {
    // Get the TcpConnectMgr instance
    TcpConnectMgr* mgr = static_cast<TcpConnectMgr*>(handle->loop->data);

    // Get the index from the handle's data
    int index = (int)(intptr_t)handle->data;
    SocketConnInfo& conn = mgr->client_sockconn_list_[index];

    // Calculate remaining space in the receive buffer
    size_t available_space = RECV_BUF_LEN - conn.recv_bytes;

    // Allocate buffer based on available space and suggested size
    size_t alloc_size = std::min(available_space, suggested_size);

    if (alloc_size > 0) {
        // Use the remaining space in the receive buffer
        buf->base = conn.recv_buf + conn.recv_bytes;
        buf->len = alloc_size;
    } else {
        // No space left, allocate a small buffer to avoid NULL
        buf->base = (char*)malloc(1);
        buf->len = 0;
    }

    // Log the allocation
    Logger::log(DEBUG, "Buffer allocated for client {}: size {}", index, buf->len);
}

void TcpConnectMgr::on_read(uv_stream_t* client, ssize_t nread, const uv_buf_t* buf) {
    TcpConnectMgr* mgr = static_cast<TcpConnectMgr*>(client->loop->data);
    int index = (int)(intptr_t)client->data;
    SocketConnInfo& conn = mgr->client_sockconn_list_[index];

    if (nread > 0) {
        // Data is already in the receive buffer, just update the count
        conn.recv_bytes += nread;
        
        // Process the received data
        mgr->process_client_data(client, conn.recv_buf + conn.recv_bytes - nread, nread);
    } else if (nread < 0) {
        if (nread != UV_EOF) {
            Logger::log(ERROR, "Read error for client {}: {}", index, uv_strerror(nread));
        } else {
            Logger::log(INFO, "Client {} disconnected", index);
        }

        // Close the client connection
        uv_close((uv_handle_t*)client, [](uv_handle_t* handle) {
            TcpConnectMgr* mgr = static_cast<TcpConnectMgr*>(handle->loop->data);
            int index = (int)(intptr_t)handle->data;
            mgr->client_sockconn_list_[index].handle = nullptr;
            free(handle);
            mgr->cur_conn_num_--;
            Logger::log(INFO, "Connection closed. Total connections: {}", mgr->cur_conn_num_);
        });
    }

    // If we allocated a small buffer due to full receive buffer, free it
    if (buf->base != mgr->client_sockconn_list_[index].recv_buf) {
        free(buf->base);
    }
}

int TcpConnectMgr::process_client_data(uv_stream_t* client, const char* data, ssize_t nread) {
    int index = (int)(intptr_t)client->data;
    SocketConnInfo& cur_conn = client_sockconn_list_[index];

    // Update receive time
    time(&cur_conn.recv_data_time);

    // Append new data to the receive buffer
    if (cur_conn.recv_bytes + nread > RECV_BUF_LEN) {
        Logger::log(ERROR, "Receive buffer overflow for client {0:d}", index);
        return -1;
    }
    memcpy(cur_conn.recv_buf + cur_conn.recv_bytes, data, nread);
    cur_conn.recv_bytes += nread;

    // Process complete packets
    int total_processed = 0;
    while (cur_conn.recv_bytes - total_processed >= PKGHEAD_FIELD_SIZE) {
        int packet_size = TcpCode::convert_int32(cur_conn.recv_buf + total_processed);

        if (packet_size <= 0 || packet_size > MAX_CSPKG_LEN) {
            Logger::log(ERROR, "Invalid packet size {0:d} for client {0:d}", packet_size, index);
            return -1;
        }

        if (cur_conn.recv_bytes - total_processed >= packet_size) {
            // Process the complete packet
            // Here you should implement your packet processing logic
            // For example, you might want to decode the protobuf message and handle it

            total_processed += packet_size;
            ++recv_pkg_count_;
        } else {
            // Incomplete packet, wait for more data
            break;
        }
    }

    // Remove processed data from the buffer
    if (total_processed > 0) {
        memmove(cur_conn.recv_buf, cur_conn.recv_buf + total_processed, cur_conn.recv_bytes - total_processed);
        cur_conn.recv_bytes -= total_processed;
    }

    Logger::log(INFO, "Processed {0:d} bytes from client {0:d}", total_processed, index);
    return 0;
}

int TcpConnectMgr::tcp_send_data(uv_stream_t* client, const char* databuf, int len) {
    uv_buf_t buffer = uv_buf_init((char*)databuf, len);
    uv_write_t* req = (uv_write_t*)malloc(sizeof(uv_write_t));
    
    return uv_write(req, client, &buffer, 1, on_write);
}

void TcpConnectMgr::on_write(uv_write_t* req, int status) {
    if (status < 0) {
        Logger::log(ERROR, "Write error: {}", uv_strerror(status));
    }
    free(req);
}

void TcpConnectMgr::check_wait_send_data() {
    // Implement your logic to check for data waiting to be sent
    // This might involve checking a queue or buffer of outgoing messages
}

void TcpConnectMgr::check_timeout() {
    time_t current_time = time(NULL);
    
    // Update statistics
    if (current_time >= laststat_time_ + STAT_TIME) {
        Logger::log(INFO, "Statistics: sent packages: {}, received packages: {}", 
                    send_pkg_count_ / STAT_TIME, recv_pkg_count_ / STAT_TIME);

        send_pkg_count_ = 0;
        recv_pkg_count_ = 0;
        laststat_time_ = current_time;
    }

    // Check for timed-out connections
    for (int i = 0; i < MAX_SOCKET_NUM; ++i) {
        if (client_sockconn_list_[i].handle != nullptr) {
            time_t last_activity = std::max(client_sockconn_list_[i].create_Time, 
                                            client_sockconn_list_[i].recv_data_time);
            if (current_time - last_activity > CLIENT_TIMEOUT) {
                Logger::log(INFO, "Client {} timed out", i);
                uv_handle_t* handle = (uv_handle_t*)client_sockconn_list_[i].handle;
                if (!uv_is_closing(handle)) {
                    uv_close(handle, [](uv_handle_t* handle) {
                        free(handle);
                    });
                }
                client_sockconn_list_[i].handle = nullptr;
                --cur_conn_num_;
                Logger::log(INFO, "Connection closed due to timeout. Total connections: {}", cur_conn_num_);
            }
        }
    }
    
    // Implement your logic to check for timed-out connections
    // You might want to iterate through client_sockconn_list_ and check last activity time
}