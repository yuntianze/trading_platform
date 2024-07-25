#include "tcp_server.h"
#include <string>
#include "logger.h"

const char* LOGFILE = "../log/tcpsvr.log";

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
    uv_ip4_addr(CONNECT_IP, CONNECT_PORT, &addr);
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
    string cmd = "touch ";
    cmd += TCPSVR_PIPE_FILE;
    system(cmd.c_str());
    int lock_fd = open("./tcplock.lock", O_RDWR|O_CREAT, 0640);
    if (lock_fd < 0) {
        Logger::log(ERROR, "Open Lock File Failed, Tcp Server Init Failed!");
        return -1;
    }

    int ret = flock(lock_fd, LOCK_EX | LOCK_NB);
    if (ret < 0) {
        Logger::log(ERROR, "Lock File Failed, Tcp Server is already Running!");
        return -1;
    }

    if (SERVER_START_DAEMON != model) {
        return 0;
    }

    pid_t pid;
    if ((pid = fork()) != 0) {
        exit(0);
    }

    setsid();

    signal(SIGINT,  SIG_IGN);
    signal(SIGHUP,  SIG_IGN);
    signal(SIGQUIT, SIG_IGN);
    signal(SIGPIPE, SIG_IGN);
    signal(SIGTTOU, SIG_IGN);
    signal(SIGTTIN, SIG_IGN);
    signal(SIGCHLD, SIG_IGN);
    signal(SIGTERM, SIG_IGN);

    struct sigaction sig;

    sig.sa_handler = SIG_IGN;
    sig.sa_flags = 0;
    sigemptyset(&sig.sa_mask);
    sigaction(SIGHUP, &sig, NULL);

    if ((pid = fork()) != 0) {
        exit(0);
    }

    umask(0);
    setpgrp();
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