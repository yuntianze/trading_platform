#include "tcp_server.h"
#include <string>
#include "tcp_connect_mgr.h"
#include "logger.h"

const char* LOGFILE = "../log/tcpsvr.log";

using std::string;

TcpConnectMgr* g_tcp_connmgr;

void TcpServer::sigusr1_handle(int sigval) {
    (void)sigval;  // Explicitly marking sigval as unused
    g_tcp_connmgr->set_run_flag(RELOAD_CFG);
    signal(SIGUSR1, sigusr1_handle);
}

void TcpServer::sigusr2_handle(int sigval) {
    (void)sigval;  // Explicitly marking sigval as unused
    g_tcp_connmgr->set_run_flag(TCP_EXIT);
    signal(SIGUSR2, sigusr2_handle);
}

int TcpServer::init(ServerStartModel model) {
    if (init_daemon(model) != 0) {
        return -1;
    }

    signal(SIGUSR1, TcpServer::sigusr1_handle);
    signal(SIGUSR2, TcpServer::sigusr2_handle);

    return 0;
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


int main(int argc, char **argv) {
    (void)argc;  // Explicitly marking argc as unused
    (void)argv;  // Explicitly marking argv as unused

    Logger::init(LOGFILE);  // 初始化日志文件

    ServerStartModel model = SERVER_START_NODAEMON;
    int ret = TcpServer::instance().init(model);
    if (ret != 0) {
        Logger::log(ERROR, "start tcpsvr error!");
        return -1;
    }

    g_tcp_connmgr  = TcpConnectMgr::create_instance();
    if (NULL == g_tcp_connmgr) {
        Logger::log(ERROR, "create tcp connect mgr error!");
        return -1;
    }

    if (g_tcp_connmgr->init() != 0) {
        return -1;
    }

    Logger::log(INFO, "start tcpsvr ok!");
    printf("start tcpsvr ok!\n");

    g_tcp_connmgr->run();

    return 0;
}
