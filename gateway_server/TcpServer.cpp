#include "TcpServer.h"
#include "TcpConnectMgr.h"
#include <string>

// 日志文件名
const char* LOGFILE_INFO = "../log/tcpsvr_info.log";
const char* LOGFILE_ERROR = "../log/tcpsvr_error.log";

using namespace std;
using namespace google;

CTcpConnectMgr* g_tcp_connmgr;

void CTcpServer::sigusr1_handle(int iSigVal)
{
    g_tcp_connmgr->SetRunFlag(reloadcfg);
    signal(SIGUSR1, sigusr1_handle);
}

void CTcpServer::sigusr2_handle(int iSigVal)
{
    g_tcp_connmgr->SetRunFlag(tcpexit);
    signal(SIGUSR2, sigusr2_handle);
}

int CTcpServer::Init(ENMServerStartModel eModel)
{
    if(InitDaemon(eModel) != 0)
    {
        return -1;
    }

    signal(SIGUSR1, CTcpServer::sigusr1_handle);
    signal(SIGUSR2, CTcpServer::sigusr2_handle);
    
    return 0;
}

int CTcpServer::InitDaemon(ENMServerStartModel eModel)
{
    string cmd = "touch ";
    cmd += TCPSVR_PIPE_FILE;
    system(cmd.c_str());
    int lock_fd = open("./tcplock.lock", O_RDWR|O_CREAT, 0640);
    if(lock_fd < 0 )
    {
        LOG(ERROR) << "Open Lock File Failed, Tcp Server Init Failed!";
        return -1;
    }
    
    int ret = flock(lock_fd, LOCK_EX | LOCK_NB);
    if(ret < 0 )
    {
        LOG(ERROR) << "Lock File Failed, Tcp Server is already Running!";
        return -1;
    }

    if(SERVER_START_DAEMON != eModel)
    {
        /*如果不是以后台方式运行，返回成功*/
        return 0;
    }

    pid_t pid;
    if((pid = fork()) != 0)
    {
        exit(0);
    }

    setsid();

    signal( SIGINT,  SIG_IGN);
    signal( SIGHUP,  SIG_IGN);
    signal( SIGQUIT, SIG_IGN);
    signal( SIGPIPE, SIG_IGN);
    signal( SIGTTOU, SIG_IGN);
    signal( SIGTTIN, SIG_IGN);
    signal( SIGCHLD, SIG_IGN);
    signal( SIGTERM, SIG_IGN);

    struct sigaction sig;

    sig.sa_handler = SIG_IGN;
    sig.sa_flags = 0;
    sigemptyset( &sig.sa_mask);
    sigaction( SIGHUP,&sig,NULL);

    if((pid = fork()) != 0)
    {
        exit(0);
    }

    umask( 0);
    setpgrp();
    return 0;
}


int main(int argc, char **argv)
{
    google::InitGoogleLogging(argv[0]);
    google::SetLogDestination(google::INFO, LOGFILE_INFO);
    google::SetLogDestination(google::ERROR, LOGFILE_ERROR);    
//    google::SetStderrLogging(google::ERROR); // 错误日志同时输出到console
//    google::FlushLogFiles(ERROR);

    ENMServerStartModel eModel = SERVER_START_NODAEMON;
    int ret = CTcpServer::Instance().Init(eModel);
    if(ret != 0)
    {
        LOG(ERROR) << "start tcpsvr error!";
        return -1;
    }

    g_tcp_connmgr  = CTcpConnectMgr::CreateInstance();
    if(NULL == g_tcp_connmgr)
    {
        LOG(ERROR) << "init tcp connect mgr error!";
        return -1;
    }

    if(g_tcp_connmgr->Init() != 0)
    {
        return -1;
    }

    LOG(INFO) << "start tcpsvr ok!";
    Trace("start tcpsvr ok!");   
    
    g_tcp_connmgr->Run();

    google::ShutdownGoogleLogging();
    
    return 0;
}
 
