#include "tcp_server.h"
#include <string>
#include "tcp_code.h"
#include "logger.h"

const char* LOGFILE = "./log/tcpsvr.log";

using std::string;

TcpServer::TcpServer() 
    : loop_(nullptr), conn_mgr_(nullptr), run_flag_(RUN_INIT),
      kafka_manager_(KafkaManager::instance()) {
}

TcpServer::~TcpServer() {
    // Clean up resources
    if (loop_) {
        uv_loop_close(loop_);
        free(loop_);
    }
    if (conn_mgr_) {
        delete conn_mgr_;
    }
    LOG(INFO, "TcpServer destroyed");
}

TcpServer& TcpServer::instance() {
    static TcpServer s_inst;
    return s_inst;
}

int TcpServer::init(ServerStartModel model) {
    // Initialize as daemon if required
    if (init_daemon(model) != 0) {
        return -1;
    }

    // Set up signal handlers
    signal(SIGUSR1, TcpServer::sigusr1_handle);
    signal(SIGUSR2, TcpServer::sigusr2_handle);

    // Initialize libuv loop
    loop_ = (uv_loop_t*)malloc(sizeof(uv_loop_t));
    if (!loop_) {
        LOG(ERROR, "Failed to allocate memory for uv_loop_t");
        return -1;
    }
    if (uv_loop_init(loop_) != 0) {
        LOG(ERROR, "Failed to initialize uv loop");
        return -1;
    }

    // Initialize TCP server
    if (uv_tcp_init(loop_, &server_) != 0) {
        LOG(ERROR, "Failed to initialize TCP server");
        return -1;
    }

    // Initialize connection manager
    conn_mgr_ = TcpConnectMgr::create_instance();
    if (conn_mgr_ == nullptr) {
        LOG(ERROR, "Failed to create TcpConnectMgr instance");
        return -1;
    }
    if (conn_mgr_->init() != 0) {
        LOG(ERROR, "Failed to initialize TcpConnectMgr");
        return -1;
    }

    // Store connection manager in loop data for easy access in callbacks
    loop_->data = conn_mgr_;

    // Bind server to address
    struct sockaddr_in addr;
    uv_ip4_addr("0.0.0.0", CONNECT_PORT, &addr);
    if (uv_tcp_bind(&server_, (const struct sockaddr*)&addr, 0) != 0) {
        LOG(ERROR, "Failed to bind server");
        return -1;
    }

    // Start listening for connections
    if (uv_listen((uv_stream_t*)&server_, SOMAXCONN, on_new_connection) != 0) {
        LOG(ERROR, "Failed to start listening");
        return -1;
    }

    // Initialize the async handle for signal processing
    uv_async_init(loop_, &async_handle_, on_async);
    async_handle_.data = this;

    // Initialize the timer for periodic checks
    uv_timer_init(loop_, &check_timer_);
    check_timer_.data = this;

    // Start the timer to run every 100ms
    uv_timer_start(&check_timer_, on_timer, 100, 100);

    // Initialize KafkaManager
    if (!kafka_manager_.init(
        "cell-1.streaming.ca-toronto-1.oci.oraclecloud.com:9092",
        "stanjiang2010/stanjiang2010@gmail.com/ocid1.streampool.oc1.ca-toronto-1.amaaaaaauz54kbqapjf3estamgf42ivwojfaktgruwh6frqw2acpodjuxlaq",
        "WIe46t6kj<Z[]cN+Y3ug")) {
        LOG(ERROR, "Failed to initialize Kafka manager");
        return -1;
    }

    // Start consuming from the order response topic
    if (!kafka_manager_.start_consuming({"kafka_topic"}, "gateway_server_consumer_group", 
        [this](const google::protobuf::Message& message) {
            this->handle_kafka_message(message);
        })) {
        LOG(ERROR, "Failed to start consuming Kafka messages");
        return -1;
    }

    LOG(INFO, "Server initialized successfully");
    return 0;
}

void TcpServer::run() {
    LOG(INFO, "Starting server main loop");
    try {
        int result = uv_run(loop_, UV_RUN_DEFAULT);
        if (result != 0) {
            LOG(ERROR, "uv_run returned with error: {}", uv_strerror(result));
        }

        // Process Kafka messages
        kafka_manager_.process_messages();

        // Small sleep to prevent CPU hogging
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    } catch (const std::exception& e) {
        LOG(ERROR, "Unhandled exception in server main loop: {}", e.what());
    } catch (...) {
        LOG(ERROR, "Unknown exception in server main loop");
    }
    LOG(INFO, "Server main loop ended");
}

void TcpServer::handle_kafka_message(const google::protobuf::Message& message) {
    // Handle the message from order_server
    // This might involve sending the response back to the client
    const cs_proto::OrderResponse& response = dynamic_cast<const cs_proto::OrderResponse&>(message);
    
    // Find the client connection using the client ID in the response
    // and send the response back to the client
    int client_index = response.client_id();
    uv_tcp_t* client = conn_mgr_->get_client_by_index(client_index);
    if (client) {
        std::string encoded_response = TcpCode::encode(response);
        TcpConnectMgr::tcp_send_data((uv_stream_t*)client, encoded_response.c_str(), encoded_response.size());
        LOG(INFO, "Sent response to client {}", client_index);
    } else {
        LOG(ERROR, "Client not found for index: {}", client_index);
    }
}

void TcpServer::reload_config() {
    run_flag_ = RELOAD_CFG;
    uv_async_send(&async_handle_);
}

void TcpServer::stop() {
    run_flag_ = TCP_EXIT;
    uv_async_send(&async_handle_);
}

void TcpServer::on_async(uv_async_t* handle) {
    TcpServer* server = static_cast<TcpServer*>(handle->data);
    server->process_run_flag();
}

void TcpServer::on_timer(uv_timer_t* handle) {
    TcpServer* server = static_cast<TcpServer*>(handle->data);
    server->perform_periodic_checks();
}

void TcpServer::process_run_flag() {
    switch (run_flag_) {
        case RELOAD_CFG:
            LOG(INFO, "Reloading configuration...");
            // Add code to reload configuration here
            run_flag_ = RUN_INIT;
            break;
        case TCP_EXIT:
            LOG(INFO, "Exiting server...");
            uv_timer_stop(&check_timer_);
            uv_stop(loop_);
            break;
        default:
            break;
    }
}

void TcpServer::perform_periodic_checks() {
    // Perform periodic checks on TcpConnectMgr
    conn_mgr_->check_wait_send_data();
    conn_mgr_->check_timeout();
}

void TcpServer::on_new_connection(uv_stream_t* server, int status) {
    if (status < 0) {
        LOG(ERROR, "New connection error: {}", uv_strerror(status));
        return;
    }

    LOG(INFO, "New connection received");

    TcpConnectMgr* conn_mgr = static_cast<TcpConnectMgr*>(server->loop->data);

    uv_tcp_t* client = (uv_tcp_t*)malloc(sizeof(uv_tcp_t));
    if (uv_tcp_init(server->loop, client) != 0) {
        LOG(ERROR, "Failed to initialize client connection");
        free(client);
        return;
    }

    if (uv_accept(server, (uv_stream_t*)client) == 0) {
        conn_mgr->handle_new_connection(client);
    } else {
        uv_close((uv_handle_t*)client, on_close);
    }
}

void TcpServer::on_close(uv_handle_t* handle) {
    TcpConnectMgr* conn_mgr = static_cast<TcpConnectMgr*>(handle->loop->data);
    conn_mgr->remove_connection((uv_tcp_t*)handle);
    free(handle);
    LOG(INFO, "Connection closed. Total connections: {}", conn_mgr->get_connection_count());
}

void TcpServer::sigusr1_handle(int sigval) {
    (void)sigval;
    TcpServer::instance().reload_config();
}

void TcpServer::sigusr2_handle(int sigval) {
    (void)sigval;
    TcpServer::instance().stop();
}

int TcpServer::init_daemon(ServerStartModel model) {
    // Check if another instance is running
    const char* lockFilePath = "./tcplock.lock";
    int lock_fd = open(lockFilePath, O_RDWR | O_CREAT, 0640);
    if (lock_fd < 0) {
        LOG(ERROR, "Open lock file failed: {}", strerror(errno));
        return -1;
    }
    if (flock(lock_fd, LOCK_EX | LOCK_NB) < 0) {
        LOG(ERROR, "Lock file failed, another instance is running.");
        close(lock_fd);
        return -1;
    }

    // Daemonize if requested
    if (model != SERVER_START_DAEMON) {
        return 0; // Not daemonizing
    }

    // Fork child process
    pid_t pid = fork();
    if (pid < 0) {
        LOG(ERROR, "Fork failed: {}", strerror(errno));
        close(lock_fd);
        return -1;
    } else if (pid > 0) {
        // Parent process exits
        _exit(EXIT_SUCCESS);
    }

    // Child process continues

    // Create new session
    if (setsid() < 0) {
        LOG(ERROR, "Setsid failed: {}", strerror(errno));
        return -1;
    }

    // Change working directory
    if (chdir("/") < 0) {
        LOG(ERROR, "Chdir failed: {}", strerror(errno));
        return -1;
    }

    // Close standard file descriptors
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);

    // Redirect standard file descriptors to /dev/null
    open("/dev/null", O_RDONLY); // stdin
    open("/dev/null", O_WRONLY); // stdout
    open("/dev/null", O_WRONLY); // stderr

    // Ignore SIGHUP signal
    signal(SIGHUP, SIG_IGN);

    // Fork again to avoid becoming a session leader
    pid = fork();
    if (pid < 0) {
        LOG(ERROR, "Fork failed: {}", strerror(errno));
        return -1;
    } else if (pid > 0) {
        // First child process exits
        _exit(EXIT_SUCCESS);
    }

    // Grandchild process continues

    // Reset file creation mask
    umask(0);

    return 0;
}

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;

    try {
        // Initialize logger
        Logger::init(LOGFILE);

        // Initialize and start server
        ServerStartModel model = SERVER_START_NODAEMON;
        TcpServer& server = TcpServer::instance();
        
        if (server.init(model) != 0) {
            LOG(ERROR, "Failed to initialize TCP server");
            return -1;
        }

        LOG(INFO, "TCP server started successfully");
        printf("TCP server started successfully\n");

        // Run the server
        server.run();

        return 0;
    } catch (const std::exception& e) {
        LOG(ERROR, "Unhandled exception: {}", e.what());
        return -1;
    } catch (...) {
        LOG(ERROR, "Unknown exception occurred");
        return -1;
    }
}
