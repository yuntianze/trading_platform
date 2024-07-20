#include "tcp_client.h"
#include <algorithm>
#include "tcp_connect_mgr.h"
#include "tcp_code.h"
#include "shm_mgr.h"
#include "logger.h"
#include "role.pb.h"


// 日志文件名
const char* LOGFILE = "../log/tcpclient.log";
// 当前共享内存指钆
char* TcpClient::current_shmptr_;

TcpClient* TcpClient::create_instance(void) {
    int shm_key = CLIENT_SHM_KEY;
    int shm_size = sizeof(TcpClient);
    int assign_size = shm_size;
    current_shmptr_ = static_cast<char*>(ShmMgr::instance().create_shm(shm_key, shm_size, assign_size));

    TcpClient* obj = new TcpClient();
    return obj;
}

void* TcpClient::operator new(size_t size) {
    (void)size;  // Explicitly marking size as unused
    return static_cast<void*>(current_shmptr_);
}

void TcpClient::operator delete(void* mem) {
    assert(mem != NULL);
    return;
}

int TcpClient::init(const char* ip, USHORT port) {
    memset(&client_conn_, 0, sizeof(client_conn_));
    memset(&client_opt_, 0, sizeof(client_opt_));
    memset(&client_events_, 0, sizeof(client_events_));

    client_epoll_fd_ = 0;
    maxfd_ = 0;
    cur_conn_num_ = 0;

    send_pkg_count_ = 0;
    recv_pkg_count_ = 0;
    laststat_time_ = 0;

    return create_socket(ip, port);
}

void TcpClient::run(void) {
    for (; ;) {
        prepare_io_event();
        tick_handle();
    }
}

int TcpClient::create_socket(const char* ip, USHORT port) {
    client_epoll_fd_ = epoll_create(MAX_CLIENT_CONN);
    if (client_epoll_fd_ < 0) {
        Logger::log(ERROR, "create client epoll fd error, fd={0:d}, errno={0:d}", client_epoll_fd_, errno);
        return -1;
    }

    for (int i = 0; i < MAX_CLIENT_CONN; ++i) {
        client_conn_[i] = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (client_conn_[i] < 0) {
            Logger::log(ERROR, "socket failed, fd={0:d}, ip={1:s}, port={0:d}", client_conn_[i], ip, port);
            return -1;
        }

        if (client_conn_[i] > maxfd_) {
            maxfd_ = client_conn_[i];
        }

        if (maxfd_ > MAX_CLIENT_CONN) {
            Logger::log(ERROR, "beyond max fd, maxfd={0:d}", maxfd_);
            break;
        }

        TcpConnectMgr::set_nonblock(client_conn_[i]);
        TcpConnectMgr::set_socket_opt(client_conn_[i]);

        struct sockaddr_in addr;
        TcpConnectMgr::set_address(ip, port, &addr);

        errno = 0;
        int ret = connect(client_conn_[i], (struct sockaddr*)(&addr), sizeof(addr));
        if (ret != 0 && errno != EINPROGRESS) {
            Logger::log(ERROR, "connect server error, ret={0:d}, ip={1:s}, port={0:d}, errno={0:d}, errstr={1:s}",
                    ret, ip, port, errno, strerror(errno));
            return -1;
        }

        // 在当前连接中注册epoll事件和连接fd
        struct epoll_event ev;
        ev.events = EPOLLIN|EPOLLOUT|EPOLLET;
        ev.data.fd = client_conn_[i];

        ret = epoll_ctl(client_epoll_fd_, EPOLL_CTL_ADD, client_conn_[i], &ev);
        if (ret < 0) {
            Logger::log(ERROR, "mod epoll event error, ret={0:d}, errno={0:d}, errstr={1:s}",
                    ret, errno, strerror(errno));
            return -1;
        }

        ++cur_conn_num_;
        Logger::log(INFO, "init client socket ok, fd={0:d}, connnum={0:d}", client_conn_[i], cur_conn_num_);
    }
    return 0;
}


void TcpClient::prepare_io_event(void) {
    int fd_num = epoll_wait(client_epoll_fd_, client_events_, MAX_CLIENT_CONN, 50);
    for (int i = 0; i < fd_num; ++i) {
        int fd = client_events_[i].data.fd;
        client_opt_[fd].fd = fd;

        // 若有可写fd,表明连接已经正常建立,可以发送数据了
        if (client_events_[i].events & EPOLLOUT) {
            if (fd <= 0 || fd > MAX_CLIENT_CONN) {
                continue;
            }
            client_opt_[fd].send_flag = true;
        }

        if (client_events_[i].events & EPOLLIN) {
            if (fd <= 0 || fd > MAX_CLIENT_CONN) {
                continue;
            }

            // 可以接收数据了
            client_opt_[fd].recv_flag = true;
        }
    }
}

void TcpClient::tick_handle(void) {
    send_data_to_server();

    int count = 2;
    for (int i = 0; i < count; ++i) {
        recv_data_from_server();
    }

    time_t curtime = time(NULL);
    if (curtime >= laststat_time_) {
        Logger::log(INFO, "send pkg={0:d}, recv pkg={0:d}", send_pkg_count_/STAT_TIME, recv_pkg_count_/STAT_TIME);
        send_pkg_count_ = 0;
        recv_pkg_count_ = 0;
        laststat_time_ = curtime + STAT_TIME;
    }

    usleep(1000*100);
}

void TcpClient::send_data_to_server(void) {
    for (int i = 0; i < MAX_CLIENT_CONN; ++i) {
        if (client_opt_[i].send_flag) {
            int fd = client_opt_[i].fd;
            assert(client_opt_[fd].fd == fd);

            int uin = (i+1)*10000;
            client_opt_[fd].uin = uin;
            int ret = send_account_login_req(fd, uin);
            if (0 == ret) {
                ++send_pkg_count_;
                Logger::log(INFO, "send pkg ok, fd={0:d}, uin={0:d}", fd, uin);
            } else {
                Logger::log(ERROR, "send account req error, ret={0:d}", ret);
            }
        }
    }
}

void TcpClient::recv_data_from_server(void) {
    for (int i = 0; i < MAX_CLIENT_CONN; ++i) {
        if (client_opt_[i].recv_flag) {
            int fd = client_opt_[i].fd;
            if (0 == fd) {
                continue;
            }

            ClientOptInfo& cur_conn = client_opt_[fd];
            assert(fd == cur_conn.fd);

            Logger::log(INFO, "recv data from server, fd={0:d}, uin={0:d}", fd, cur_conn.uin);

            int offset = cur_conn.recv_bytes;
            int len = sizeof(cur_conn.recv_buf) - offset;
            int ret = TcpConnectMgr::tcp_recv_data(fd, cur_conn.recv_buf + offset, len);
            if (ERROR_CLIENT_CLOSE == ret) {
                Logger::log(ERROR, "recv server data error, ret={0:d}", ret);
                cur_conn.recv_bytes = 0;
                // close(fd);
                continue;
            } else if (ERROR_READ_BUFFEMPTY == ret && 0 == len) {
                continue;
            }

            if (len <= MIN_CSPKG_LEN || len >= MAX_CSPKG_LEN) {
                Logger::log(ERROR, "recv invalid server pkg, fd={0:d}, len={0:d}, ret={0:d}", fd, len, ret);
                cur_conn.recv_bytes = 0;
                continue;
            }

            cur_conn.recv_bytes += len;  // 累加当前socket已接收的数据
            int cur_pkg_len = TcpCode::convert_int32(cur_conn.recv_buf);  // 当前请求包原始长度

            // 只有当前已接受的数据大于原始请求包长度时,才作处理,否则继续接受数据
            while (cur_conn.recv_bytes >= cur_pkg_len) {
                // 检查本次接受的数据是否还有剩余数据还没有处理
                int recv_restdata_len = cur_conn.recv_bytes - cur_pkg_len;
                if (recv_restdata_len > 0) {
                    memmove(cur_conn.recv_buf, cur_conn.recv_buf + cur_pkg_len, recv_restdata_len);
                    cur_conn.recv_bytes -= cur_pkg_len;
                    cur_pkg_len = TcpCode::convert_int32(cur_conn.recv_buf);  // 下一个请求包原始长度

                    Logger::log(INFO, "there are rest data to been process after, restlen={0:d}, fd={0:d}next pkg len={0:d}",
                            recv_restdata_len, fd, cur_pkg_len);
                } else {
                    recv_account_login_res(fd, cur_conn.recv_buf, cur_pkg_len);
                    cur_conn.recv_bytes = 0;
                    ++recv_pkg_count_;
                }
            }
        }
    }
}

void TcpClient::print_msg(const std::string& buf) {
    Logger::log(INFO, "print msg info, buf={0:s}", buf);
    for (size_t i = 0; i < buf.size(); ++i) {
        Logger::log(INFO, "{0:d}: {1:c}", i, buf[i]);
    }
}

int TcpClient::send_account_login_req(int fd, UINT uin) {
    cspkg::AccountLoginReq acc_login_req;
    acc_login_req.set_account(uin);
    std::string session = R"(Dream what you want to dream; go where you want to go; be what you want to be; \
        because you have only one life and one chance to do all the things you want to do.)";

    acc_login_req.set_session_key(session);

    std::string pkg = TcpCode::encode(acc_login_req);
    if (pkg.size() == 0) {
        return -1;
    }

    int pkg_head = 0;
    std::copy(pkg.begin(), pkg.begin() + sizeof(pkg_head),  reinterpret_cast<char*>(&pkg_head));
    int len = ::ntohl(pkg_head);
    assert(len == static_cast<int>(pkg.size()));

    std::string buf = pkg.substr(PKGHEAD_FIELD_SIZE);
    assert(len == static_cast<int>(buf.size())+PKGHEAD_FIELD_SIZE);

    Logger::log(INFO, "encode pkg ok, len={0:d}, fd={0:d}, uin={0:d}", len, fd, uin);

    return TcpConnectMgr::tcp_send_data(fd, pkg.c_str(), pkg.size());
}


int TcpClient::recv_account_login_res(int fd, const char* pkg, int len) {
    std::string buf;
    buf.assign(pkg, len);
    google::protobuf::Message* msg = TcpCode::decode(buf);
    assert(NULL != msg);
    cspkg::AccountLoginRes* acc_login_res = dynamic_cast<cspkg::AccountLoginRes*>(msg);
    assert(acc_login_res != NULL);
    assert(acc_login_res->account() == static_cast<uint32_t>(client_opt_[fd].uin));

    Logger::log(INFO, "recv accout login res, fd={0:d}, uin={0:d}, result={0:d}",
            fd, acc_login_res->account(), acc_login_res->result());

    return 0;
}

void TcpClient::destroy(void) {
    for (int i = 0; i < MAX_CLIENT_CONN; ++i) {
        close(client_conn_[i]);
    }
}


int main(int argc, char** argv) {
    (void)argc;  // Explicitly marking argc as unused
    (void)argv;  // Explicitly marking argv as unused

    Logger::init(LOGFILE);  // 初始化日志文件

    TcpClient* client = TcpClient::create_instance();
    if (NULL == client) {
        printf("create client inst error!\n");
        return -1;
    }

    printf("create client inst ok!\n");

    int ret = client->init(CONNECT_IP, CONNECT_PORT);
    if (ret != 0) {
        Logger::log(ERROR, "init client error");
        return -1;
    }

    client->run();

    return 0;
}


