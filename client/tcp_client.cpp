#include "tcp_client.h"
#include <cstring>
#include "tcp_code.h"
#include "logger.h"
#include "role.pb.h"
#include "futures_order.pb.h"


TcpClient::TcpClient(uv_loop_t* loop, const char* ip, int port)
    : loop_(loop), server_ip_(ip), server_port_(port), is_logged_in_(false) {
    uv_tcp_init(loop_, &client_);
    client_.data = this;
}

TcpClient::~TcpClient() {
    uv_close((uv_handle_t*)&client_, nullptr);
}

int TcpClient::init() {
    struct sockaddr_in dest;
    uv_ip4_addr(server_ip_.c_str(), server_port_, &dest);

    uv_connect_t* connect = new uv_connect_t;
    connect->data = this;

    return uv_tcp_connect(connect, &client_, (const struct sockaddr*)&dest, on_connect);
}

void TcpClient::run() {
    uv_run(loop_, UV_RUN_DEFAULT);
}

void TcpClient::on_connect(uv_connect_t* req, int status) {
    TcpClient* client = static_cast<TcpClient*>(req->data);
    delete req;

    if (status < 0) {
        LOG(ERROR, "Connection failed: {}", uv_strerror(status));
        return;
    }

    LOG(INFO, "Connected to server");

    // Start reading from the server
    uv_read_start((uv_stream_t*)&client->client_, alloc_buffer, on_read);

    // Send login request
    client->send_account_login_req(10000);  // Example UIN
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
                LOG(ERROR, "Unknown message type received");
            }
        } else {
            LOG(ERROR, "Failed to decode message");
        }
    } else if (nread < 0) {
        if (nread != UV_EOF) {
            LOG(ERROR, "Read error: {}", uv_strerror(nread));
        }
        uv_close((uv_handle_t*)stream, nullptr);
    }
}

void TcpClient::on_write(uv_write_t* req, int status) {
    if (status < 0) {
        LOG(ERROR, "Write error: {}", uv_strerror(status));
    }
    delete req;
}

void TcpClient::alloc_buffer(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf) {
    (void)suggested_size;  // Unused
    TcpClient* client = static_cast<TcpClient*>(handle->data);
    buf->base = client->read_buf_;
    buf->len = sizeof(client->read_buf_);
}

int TcpClient::send_data(const char* data, size_t len) {
    uv_buf_t buf = uv_buf_init(const_cast<char*>(data), len);
    uv_write_t* req = new uv_write_t;
    return uv_write(req, (uv_stream_t*)&client_, &buf, 1, on_write);
}

int TcpClient::send_account_login_req(uint32_t uin) {
    cspkg::AccountLoginReq acc_login_req;
    acc_login_req.set_account(uin);
    std::string session = "Example session key";
    acc_login_req.set_session_key(session);

    std::string pkg = TcpCode::encode(acc_login_req);
    if (pkg.empty()) {
        return -1;
    }

    return send_data(pkg.c_str(), pkg.size());
}

void TcpClient::process_account_login_res(const char* data, size_t len) {
    std::string buf(data, len);
    std::unique_ptr<google::protobuf::Message> msg(TcpCode::decode(buf));
    if (!msg) {
        LOG(ERROR, "Failed to decode message");
        return;
    }

    const cspkg::AccountLoginRes* acc_login_res = dynamic_cast<const cspkg::AccountLoginRes*>(msg.get());
    if (!acc_login_res) {
        LOG(ERROR, "Message is not AccountLoginRes");
        return;
    }

    LOG(INFO, "Received account login response: account={}, result={}", 
                acc_login_res->account(), acc_login_res->result());

    if (acc_login_res->result() == 0) {  // Assuming 0 means success
        is_logged_in_ = true;
        LOG(INFO, "Login successful, sending futures order");
        send_futures_order();
    } else {
        LOG(ERROR, "Login failed");
    }
}

int TcpClient::send_futures_order() {
    if (!is_logged_in_) {
        LOG(ERROR, "Cannot send futures order: not logged in");
        return -1;
    }

    cs_proto::FuturesOrder order;
    order.set_order_id("ord123");
    order.set_user_id("user1");
    order.set_symbol("BTCUSD");
    order.set_side(cs_proto::OrderSide::BUY);
    order.set_type(cs_proto::OrderType::LIMIT);
    order.set_quantity(1.0);
    order.set_price(50000.0);
    order.set_status(cs_proto::OrderStatus::PENDING);
    order.set_timestamp(std::time(nullptr));
    order.set_client_id(1);  // Assuming client ID is 1

    std::string pkg = TcpCode::encode(order);
    if (pkg.empty()) {
        return -1;
    }

    return send_data(pkg.c_str(), pkg.size());
}

void TcpClient::process_order_response(const char* data, size_t len) {
    std::string buf(data, len);
    std::unique_ptr<google::protobuf::Message> msg(TcpCode::decode(buf));
    if (!msg) {
        LOG(ERROR, "Failed to decode message");
        return;
    }

    const cs_proto::OrderResponse* order_res = dynamic_cast<const cs_proto::OrderResponse*>(msg.get());
    if (!order_res) {
        LOG(ERROR, "Message is not OrderResponse");
        return;
    }

    LOG(INFO, "Received order response: order_id={}, status={}, message={}", 
                order_res->order_id(), cs_proto::OrderStatus_Name(order_res->status()), order_res->message());
}

int main() {
    Logger::init("../log/tcpclient.log");

    uv_loop_t* loop = uv_default_loop();
    TcpClient client(loop, CONNECT_IP, CONNECT_PORT);

    if (client.init() != 0) {
        LOG(ERROR, "Failed to initialize client");
        return 1;
    }

    LOG(INFO, "Client initialized successfully");

    client.run();

    return 0;
}