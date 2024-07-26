#include "tcp_server.h"
#include <string>
#include "logger.h"

const char* LOGFILE = "./log/tcpsvr.log";

using std::string;

TcpServer::TcpServer() : loop_(nullptr), conn_mgr_(nullptr) {}

TcpServer::~TcpServer() {
    // Clean up resources
    if (loop_) {
        uv_loop_close(loop_);
        free(loop_);
    }
    if (conn_mgr_) {
        delete conn_mgr_;
    }
}

TcpServer& TcpServer::instance() {
    static TcpServer s_inst;
    return s_inst;
}

void TcpServer::sigusr1_handle(int sigval) {
    (void)sigval;
    // Set reload configuration flag
    TcpServer::instance().conn_mgr_->set_run_flag(RELOAD_CFG);
}

void TcpServer::sigusr2_handle(int sigval) {
    (void)sigval;
    // Set exit flag
    TcpServer::instance().conn_mgr_->set_run_flag(TCP_EXIT);
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
        Logger::log(ERROR, "Failed to allocate memory for uv_loop_t");
        return -1;
    }
    if (uv_loop_init(loop_) != 0) {
        Logger::log(ERROR, "Failed to initialize uv loop");
        return -1;
    }

    // Initialize TCP server
    if (uv_tcp_init(loop_, &server_) != 0) {
        Logger::log(ERROR, "Failed to initialize TCP server");
        return -1;
    }

    loop_->data = conn_mgr_;  // Store connection manager in loop data for easy access in callbacks

    // Bind server to address
    struct sockaddr_in addr;
    uv_ip4_addr("0.0.0.0", CONNECT_PORT, &addr);
    if (uv_tcp_bind(&server_, (const struct sockaddr*)&addr, 0) != 0) {
        Logger::log(ERROR, "Failed to bind server");
        return -1;
    }

    // Start listening for connections
    if (uv_listen((uv_stream_t*)&server_, SOMAXCONN, on_new_connection) != 0) {
        Logger::log(ERROR, "Failed to start listening");
        return -1;
    }

    // Initialize connection manager
    conn_mgr_ = TcpConnectMgr::create_instance();
    if (conn_mgr_ == nullptr) {
        Logger::log(ERROR, "Failed to create TcpConnectMgr instance");
        return -1;
    }
    if (conn_mgr_->init() != 0) {
        Logger::log(ERROR, "Failed to initialize TcpConnectMgr");
        return -1;
    }

    // Store connection manager in loop data for easy access in callbacks
    loop_->data = conn_mgr_;

    Logger::log(INFO, "Server initialized successfully");
    return 0;
}

void TcpServer::run() {
    Logger::log(INFO, "Starting server main loop");
    uv_run(loop_, UV_RUN_DEFAULT);
}

int TcpServer::init_daemon(ServerStartModel model) {
    // Check if another instance is running
    const char* lockFilePath = "./tcplock.lock";
    int lock_fd = open(lockFilePath, O_RDWR | O_CREAT, 0640);
    if (lock_fd < 0) {
        Logger::log(ERROR, "Open lock file failed: {}", strerror(errno));
        return -1;
    }
    if (flock(lock_fd, LOCK_EX | LOCK_NB) < 0) {
        Logger::log(ERROR, "Lock file failed, another instance is running.");
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
        Logger::log(ERROR, "Fork failed: {}", strerror(errno));
        close(lock_fd);
        return -1;
    } else if (pid > 0) {
        // Parent process exits
        _exit(EXIT_SUCCESS);
    }

    // Child process continues

    // Create new session
    if (setsid() < 0) {
        Logger::log(ERROR, "Setsid failed: {}", strerror(errno));
        return -1;
    }

    // Change working directory
    if (chdir("/") < 0) {
        Logger::log(ERROR, "Chdir failed: {}", strerror(errno));
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
        Logger::log(ERROR, "Fork failed: {}", strerror(errno));
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

void TcpServer::on_new_connection(uv_stream_t* server, int status) {
    if (status < 0) {
        Logger::log(ERROR, "New connection error: {}", uv_strerror(status));
        return;
    }

    TcpConnectMgr* conn_mgr = static_cast<TcpConnectMgr*>(server->loop->data);

    uv_tcp_t* client = (uv_tcp_t*)malloc(sizeof(uv_tcp_t));
    if (uv_tcp_init(server->loop, client) != 0) {
        Logger::log(ERROR, "Failed to initialize client connection");
        free(client);
        return;
    }

    if (uv_accept(server, (uv_stream_t*)client) == 0) {
        conn_mgr->handle_new_connection(client);
    } else {
        uv_close((uv_handle_t*)client, [](uv_handle_t* handle) {
            free(handle);
        });
    }
}

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;

    // Initialize logger
    Logger::init(LOGFILE);

    // Initialize and start server
    ServerStartModel model = SERVER_START_NODAEMON;
    TcpServer& server = TcpServer::instance();
    
    if (server.init(model) != 0) {
        Logger::log(ERROR, "Failed to initialize TCP server");
        return -1;
    }

    Logger::log(INFO, "TCP server started successfully");
    printf("TCP server started successfully\n");

    // Run the server
    server.run();

    return 0;
}