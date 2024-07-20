#include "tcp_connect_mgr.h"
#include <algorithm>
#include "shm_mgr.h"
#include "tcp_code.h"
#include "logger.h"


char* TcpConnectMgr::current_shmptr_ = NULL;


TcpConnectMgr::TcpConnectMgr() :
    run_flag_(RUN_INIT),
    epoll_fd_(INVALID_SOCKET),
    listen_fd_(INVALID_SOCKET),
    maxfds_(0),
    epoll_timeout_(0),
    cur_conn_num_(0),
    send_pkg_count_(0),
    recv_pkg_count_(0),
    laststat_time_(0) {
}

TcpConnectMgr::~TcpConnectMgr() {
    if (listen_fd_ != INVALID_SOCKET) {
        close(listen_fd_);
    }

    if (epoll_fd_ != INVALID_SOCKET) {
        close(epoll_fd_);
    }

    Logger::log(INFO, "tcpsvr exit ok!");
}

TcpConnectMgr* TcpConnectMgr::create_instance(void) {
    int shm_key = SOCKET_SHM_KEY;
    int shm_size = count_size();
    int assign_size = shm_size;
    current_shmptr_ = static_cast<char*>(ShmMgr::instance().create_shm(shm_key, shm_size, assign_size));

    TcpConnectMgr* obj = new TcpConnectMgr();
    return obj;
}

int TcpConnectMgr::count_size(void) {
    int size = sizeof(TcpConnectMgr);
    return size;
}

void* TcpConnectMgr::operator new(size_t size) {
    return static_cast<void*>(current_shmptr_ + size);
}

void TcpConnectMgr::operator delete(void* mem) {
    assert(mem != NULL);
}

int TcpConnectMgr::init(void) {
    errno = 0;
    send_pkg_count_ = 0;
    recv_pkg_count_ = 0;
    laststat_time_ = 0;

    // 初始化服务器监听socket
    if (init_listen_socket(CONNECT_IP, CONNECT_PORT) != 0) {
        return -1;
    }

    // 初始化socket列表数据
    for (int i = 0; i < MAX_SOCKET_NUM; ++i) {
        memset(&client_sockconn_list_[i], 0, sizeof(SocketConnInfo));
        client_sockconn_list_[i].socket_fd = INVALID_SOCKET;
    }

    maxfds_ = listen_fd_+1;
    epoll_timeout_ = 20;
    cur_conn_num_ = 0;

    Logger::log(INFO, "init tcpsvr ok!");

    return 0;
}

void TcpConnectMgr::run(void) {
    while (1) {
        if (TCP_EXIT == run_flag_) {
            Logger::log(INFO, "tcpsvrd exit!");
            return;
        } else if (RELOAD_CFG == run_flag_) {
            Logger::log(INFO, "reload tcpsvrd config file ok!");
            run_flag_ = RUN_INIT;
        }

        // 读取客户端请求
        get_client_message();

        // 读取tcpsvr中待发送的数据
        check_wait_send_data();

        // 检测通讯超时的socket
        check_timeout();
    }
}

int TcpConnectMgr::init_listen_socket(const char* ip, USHORT port) {
    listen_fd_ = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listen_fd_ < 0) {
        Logger::log(ERROR, "socket failed, fd={0:d}, ip={1:s}, port={2:d}", listen_fd_, ip, port);
        return -1;
    }

    if (set_nonblock(listen_fd_) != 0) {
        Logger::log(ERROR, "set nonblock failed, fd={0:d}", listen_fd_);
        return -1;
    }

    if (set_socket_opt(listen_fd_) != 0) {
        Logger::log(ERROR, "set socket opt failed, fd={0:d}", listen_fd_);
        return -1;
    }
    struct sockaddr_in addr;
    set_address(ip, port, &addr);

    int ret = bind(listen_fd_, (struct sockaddr*)&addr, sizeof(addr));
    if (ret != 0) {
        Logger::log(ERROR, "bind socket error, ret={0:d}", ret);
        return -1;
    }

    ret = listen(listen_fd_, LISTEN_BACKLOG);
    if (ret != 0) {
        Logger::log(ERROR, "listen socket error, ret={0:d}", ret);
        return -1;
    }

    Logger::log(INFO, "create server socket ok, ip={0:s}, port={0:d}, fd={0:d}", ip, port, listen_fd_);

    // 创建服务器epoll句柄
    epoll_fd_ = epoll_create(MAX_SOCKET_NUM);
    if (epoll_fd_ < 0) {
        Logger::log(ERROR, "create server epoll fd error, fd={0:d}, errno={0:d}", epoll_fd_, errno);
        return -1;
    }

    // 将服务器侦听socket加入epoll事件中
    struct epoll_event ev;
    ev.events = EPOLLIN|EPOLLET|EPOLLERR|EPOLLHUP;
    ev.data.fd = listen_fd_;

    ret = epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, listen_fd_, &ev);
    if (ret < 0) {
        Logger::log(ERROR, "mod epoll event error, fd={0:d}, ret={0:d}, err={0:d}, errstr={0:s}",
                listen_fd_, ret, errno, strerror(errno));
        return -1;
    }

    Logger::log(INFO, "add listen socket to epoll ok, fd={0:d}", listen_fd_);

    return 0;
}

int TcpConnectMgr::set_nonblock(int fd) {
    int flags = 1;
    int ret = ioctl(fd, FIONBIO, &flags);
    if (ret != 0) {
        Logger::log(ERROR, "ioctl opt error, ret={0:d}", ret);
        return -1;
    }

    flags = fcntl(fd, F_GETFL);
    flags |= O_NONBLOCK;
    ret = fcntl(fd, F_SETFL, flags);
    if (ret < 0) {
        Logger::log(ERROR, "fcntl opt error, ret={0:d}", ret);
        return -1;
    }

    return 0;
}

int TcpConnectMgr::set_socket_opt(int fd) {
    // 设置套接字重用
    int reuse_addr_ok = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &reuse_addr_ok, sizeof(reuse_addr_ok));

    // 设置接收&发送buffer
    int recv_buf = SOCK_RECV_BUFFER;
    setsockopt(fd, SOL_SOCKET, SO_RCVBUF, (const char*)&recv_buf, sizeof(recv_buf));
    int send_buf = SOCK_SEND_BUFFER;
    setsockopt(fd, SOL_SOCKET, SO_SNDBUF, (const char*)&send_buf, sizeof(send_buf));

    int flags = 1;
    struct linger ling = {0, 0};
    setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, &flags, sizeof(flags));
    setsockopt(fd, SOL_SOCKET, SO_LINGER, &ling, sizeof(ling));

    return 0;
}

void TcpConnectMgr::set_address(const char* ip, USHORT port, struct sockaddr_in* addr) {
    bzero(addr, sizeof(*addr));
    addr->sin_family = AF_INET;
    addr->sin_addr.s_addr = inet_addr(ip);
    addr->sin_port = htons(port);
}

void TcpConnectMgr::clear_socket_info(int fd, SocketErrors type) {
    if (ERROR_CLIENT_CLOSE == type) {
        close(fd);
        dec_sock_conn();
    }
}

void TcpConnectMgr::get_client_message(void) {
    struct sockaddr_in client_addr;
    bzero(&client_addr, sizeof(client_addr));
    struct epoll_event ev;

    int fd_num = epoll_wait(epoll_fd_, events_, MAX_SOCKET_NUM, epoll_timeout_);
    for (int i = 0; i < fd_num; ++i) {
        if (events_[i].data.fd <= 0) {
            Logger::log(ERROR, "invalid socket fd, fd={0:d}", events_[i].data.fd);
            continue;
        }

        if (events_[i].events & EPOLLERR) {
            Logger::log(ERROR, "socket generate error event");
            clear_socket_info(events_[i].data.fd, ERROR_CLIENT_CLOSE);
            continue;
        }

        // 如果监测到一个SOCKET用户连接到了绑定的SOCKET端口，则建立新的连接
        if (events_[i].data.fd == listen_fd_) {
            while (1) {
                socklen_t client_len = sizeof(sockaddr_in);
                int conn_fd = accept(listen_fd_, (struct sockaddr*)&client_addr, &client_len);
                if (conn_fd <= 0) {
                    if (EINTR == errno) {
                        Logger::log(INFO, "accept conn continue, fd={0:d}", conn_fd);
                        continue;
                    }

                    if (EAGAIN == errno) {
                        // 当前尚未有新连接上来
                        Logger::log(INFO, "current no client connect accept, fd={0:d}, err={0:d}, errstring={0:s}",
                                conn_fd, errno, strerror(errno));
                        break;
                    }

                    // 其他情况出错则表示客户端连接上来以后又立即关闭了
                    Logger::log(ERROR, "accept socket conn error, fd={0:d}, err={0:d}, errstring={0:s}",
                            conn_fd, errno, strerror(errno));
                    break;
                } else {
                    inc_sock_conn();
                    // 检查连接数是否过载
                    if (cur_conn_num_ >= MAX_SOCKET_NUM || conn_fd >= MAX_SOCKET_NUM) {
                        Logger::log(ERROR, "accept a invalid conn, fd={0:d}", conn_fd);
                        close(conn_fd);
                        dec_sock_conn();
                        continue;
                    }

                    if (conn_fd > maxfds_) {
                        maxfds_ = conn_fd;
                    }

                    // 添加新的client连接至epoll事件集合中
                    set_nonblock(conn_fd);
                    ev.data.fd = conn_fd;
                    ev.events = EPOLLIN|EPOLLET;
                    int ret = epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, conn_fd, &ev);
                    if (ret < 0) {
                        Logger::log(ERROR, "epoll_ctl add client conn error, ret={0:d}, err={0:d}, errstring={0:s}",
                                ret, errno, strerror(errno));
                        continue;
                    }

                    // 分配新连接存储空间,以socket为索引
                    client_sockconn_list_[conn_fd].socket_fd = conn_fd;
                    client_sockconn_list_[conn_fd].client_ip = client_addr.sin_addr.s_addr;
                    time(&(client_sockconn_list_[conn_fd].create_Time));

                    client_sockconn_list_[conn_fd].recv_bytes = 0;
                    client_sockconn_list_[conn_fd].recv_data_time = 0;
                    client_sockconn_list_[conn_fd].uin = 0;

                    Logger::log(INFO, "accept a client socket ok, client={0:s}, fd={0:d}, connnum={0:d}",
                            inet_ntoa(client_addr.sin_addr), conn_fd, cur_conn_num_);
                }
            }
        } else if (events_[i].events & EPOLLIN) {
            int sockfd = events_[i].data.fd;
            if (sockfd < 0) {
                Logger::log(ERROR, "invalid socket, fd={0:d}", sockfd);
                continue;
            }

            Logger::log(INFO, "process client reading, fd={0:d}", sockfd);

            // 从socket读取client数据,并发送至tcpsvr-->gamesvr的消息队列中
            recv_client_data(sockfd);
        }
    }
}

void TcpConnectMgr::check_wait_send_data(void) {
    for (int i = 0; i < MAX_SEND_PKGNUM; ++i) {
        // int size = 0;
        // int ret = mq_sc_->Pop(send_client_buf_, size);
        // if(ret != 0)
        // {
        //     return;
        // }

        char* recv_data = &send_client_buf_[0];
        CSPkgHead* head = reinterpret_cast<CSPkgHead*>(recv_data);
        int fd = ntohl(head->fd);
        if (fd > MAX_SOCKET_NUM) {
            Logger::log(ERROR, "recv invalid client fd, fd={0:d}", fd);
            return;
        }

        ULONG client_ip = ntohl(head->client_ip);
        assert(client_sockconn_list_[fd].socket_fd == fd);
        assert(client_sockconn_list_[fd].client_ip == client_ip);

        const char* body = recv_data + sizeof(CSPkgHead);
        int pkg_body_size = TcpCode::convert_int32(body);

        int ret = tcp_send_data(fd, body, pkg_body_size);
        if (ret != 0) {
            Logger::log(ERROR, "send to client data error, ret={0:d}, fd={0:d}, size={0:d}", ret, fd, pkg_body_size);
        } else {
            Logger::log(INFO, "send to client data ok, ret={0:d}, fd={0:d}, size={0:d}", ret, fd, pkg_body_size);
            ++send_pkg_count_;
        }
    }
}

void TcpConnectMgr::check_timeout(void) {
    /***加入统计信息***/
    time_t curtime = time(NULL);
    if (curtime >= laststat_time_) {
        send_pkg_count_ /= STAT_TIME;
        recv_pkg_count_ /= STAT_TIME;

        Logger::log(INFO, "process pkg info, sendpkg={0:d}, recvpkg={0:d}", send_pkg_count_, recv_pkg_count_);

        send_pkg_count_ = 0;
        recv_pkg_count_ = 0;
        laststat_time_ = curtime + STAT_TIME;
    }
}

int TcpConnectMgr::recv_client_data(int fd) {
    SocketConnInfo& cur_conn = client_sockconn_list_[fd];
    assert(fd == cur_conn.socket_fd);

    // 检查socket合法性
    if (fd != cur_conn.socket_fd) {
        Logger::log(ERROR, "invalid socket, fd={0:d}, cur_fd={0:d}", fd, cur_conn.socket_fd);
        return -1;
    }

    /* 由于采用的是ET触发,故每次read数据时,均把当前socket tcp buf中的数据读取完毕,
        即最大限度地读取数据*/    

    int offset = cur_conn.recv_bytes;
    int len = sizeof(cur_conn.recv_buf) - offset;
    int ret = tcp_recv_data(fd, cur_conn.recv_buf + offset, len);
    if (ret == ERROR_CLIENT_CLOSE) {
        Logger::log(INFO, "recv client data error, ret={0:d}", ret);
        // clear_socket_info(fd, Err_ClientClose);
        cur_conn.recv_bytes = 0;
        return -1;
    } else if (ERROR_READ_BUFFEMPTY == ret && 0 == len) {
        return 0;
    }

    if (len <= MIN_CSPKG_LEN || len > MAX_CSPKG_LEN) {
        Logger::log(ERROR, "recv invalid client pkg, fd={0:d}, len={0:d}", fd, len);
        return -1;
    }

    cur_conn.recv_bytes += len;  // 累加当前socket已接收的数据
    time(&cur_conn.recv_data_time);  // 记录接收数据时间
    int cur_pkg_len = TcpCode::convert_int32(cur_conn.recv_buf);  // 当前请求包原始长度
    if (cur_pkg_len <= 0) {
        Logger::log(ERROR, "recv client data error, fd={0:d}, offset={0:d}, cur_recv_len={0:d}, totle_recv_len={0:d}, cur_pkg_len={0:d}",
                fd, offset, len, cur_conn.recv_bytes, cur_pkg_len);
        return -1;
    }

    Logger::log(INFO, "recv client data ok, fd={0:d}, offset={0:d}, cur_recv_len={0:d}, totle_recv_len={0:d}, cur_pkg_len={0:d}",
            fd, offset, len, cur_conn.recv_bytes, cur_pkg_len);

    // 只有当前已接受的数据大于原始请求包长度时,才作处理,否则继续接受数据
    while (cur_conn.recv_bytes >= cur_pkg_len) {
        static char msg_buf[CSPKG_OPT_BUFFSIZE];
        char* msg_ptr = msg_buf;
        CSPkgHead* pkg_head = reinterpret_cast<CSPkgHead*>(msg_ptr);
        pkg_head->pkg_size = htonl(cur_pkg_len + sizeof(CSPkgHead));
        pkg_head->client_ip = htonl(cur_conn.client_ip);
        pkg_head->fd = htonl(fd);

        msg_ptr += sizeof(CSPkgHead);
        memcpy(msg_ptr, cur_conn.recv_buf, cur_pkg_len);

        // 加入了tcpsvr-->gamesvr的消息队列中
        // mq_cs_->Push(msg_buf, cur_pkg_len + sizeof(CSPkgHead));

        // 检查本次接受的数据是否还有剩余数据还没有处理
        int recv_restdata_len = cur_conn.recv_bytes - cur_pkg_len;
        if (recv_restdata_len > 0) {
            memmove(cur_conn.recv_buf, cur_conn.recv_buf + cur_pkg_len, recv_restdata_len);
            cur_conn.recv_bytes -= cur_pkg_len;
            cur_pkg_len = TcpCode::convert_int32(cur_conn.recv_buf);  // 下一个请求包原始长度

            Logger::log(INFO, "there are rest data to been process after, restlen={0:d}, fd={0:d}next pkg len={0:d}",
                    recv_restdata_len, fd, cur_pkg_len);
        } else {
            cur_conn.recv_bytes = 0;
            Logger::log(INFO, "current client pkg process over, fd={0:d}", fd);
            ++recv_pkg_count_;
        }
    }

    return 0;
}

int TcpConnectMgr::tcp_send_data(int fd, const char* databuf, int len) {
    int left = len;
    while (left > 0) {
        int ret = write(fd, databuf + len - left, left);
        if (ret <= 0) {
            // 忽略EINTR 信号
            if (errno == EINTR) {
                Logger::log(INFO, "recv EINTR signal while send data, fd={0:d}", fd);
                continue;
            }

             // 当socket是非阻塞时,如返回EAGAIN,表示写缓冲队列已满
            if (errno == EAGAIN) {
                Logger::log(INFO, "tcp send buffer is full, fd={0:d}, left={0:d}, len={0:d}", fd, left, len);
                // 下次继续写
                return ERROR_WRITE_BUFFOVER;
            }

            // 连接出错,需要关闭连接
            Logger::log(ERROR, "tcp connect error, ret={0:d}, errno={0:d}, errstr={0:s}, fd={0:d}",
                    ret, errno, strerror(errno), fd);
            return ERROR_CLIENT_CLOSE;
        }

        left -= ret;
        Logger::log(INFO, "send data ok, fd={0:d}, left={0:d}, len={0:d}", fd, left, len);
    }

    Logger::log(INFO, "send data over, fd={0:d}", fd);
    return ERROR_OK;
}

int TcpConnectMgr::tcp_recv_data(int fd, char* databuf, int& len) {
    int read_len = 0;
    int left = len;
    while (left > 0) {
        int ret = read(fd, databuf + len - left, std::min(left, SOCK_RECV_BUFFER));
        if (ret <= 0) {
            // 忽略EINTR 信号
            if (errno == EINTR) {
                Logger::log(INFO, "recv EINTR signal while recv data, fd={0:d}", fd);
                continue;
            }

            if (0 == ret) {
                // 表示client已关闭连接
                len = 0;
                Logger::log(INFO, "the client connect has closed, ret={0:d}, errno={0:d}, errstr={0:s}, fd={0:d}",
                        ret, errno, strerror(errno), fd);
                return ERROR_CLIENT_CLOSE;
            }

            // 当socket是非阻塞时,如返回EAGAIN,表示读缓冲队列为空
            if (errno == EAGAIN) {
                len = read_len;
                Logger::log(INFO, "tcp recv buffer is empty, ret={0:d}, fd={0:d}, left={0:d}, len={0:d}, readlen={0:d}",
                        ret, fd, left, len, read_len);
                // 下次继续读
                return ERROR_READ_BUFFEMPTY;
            }

            // 其它出错
            len = 0;
            Logger::log(ERROR, "tcp connect error, ret={0:d}, errno={0:d}, errstr={0:s}, fd={0:d}",
                    ret, errno, strerror(errno), fd);
            return ERROR_CLIENT_CLOSE;
        }

        left -= ret;
        read_len += ret;
        Logger::log(INFO, "recv data ok, fd={0:d}, left={0:d}, len={0:d}", fd, left, len);
    }

    Logger::log(INFO, "recv data over, fd={0:d}", fd);

    len = read_len;
    return ERROR_OK;
}


