#!/bin/bash

#表示如果套接字由本端要求关闭，这个参数决定了它保持在FIN-WAIT-2状态的时间
echo 30 > /proc/sys/net/ipv4/tcp_fin_timeout

#表示当keepalive起用的时候，TCP发送keepalive消息的频度。缺省是2小时，改为20分钟。
echo 1200 > /proc/sys/net/ipv4/tcp_keepalive_time

#路由缓存刷新频率， 当一个路由失败后多长时间跳到另一个默认是300
echo 30 > /proc/sys/net/ipv4/route/gc_timeout

#表示用于向外连接的端口范围。缺省情况下很小：32768到61000，改为1024到65000
echo 1024 65000 > /proc/sys/net/ipv4/ip_local_port_range

#表示开启重用。允许将TIME_WAIT sockets重新用于新的TCP连接，默认为0，表示关闭
echo 1 > /proc/sys/net/ipv4/tcp_tw_reuse

#表示开启TCP连接中TIME_WAIT sockets的快速回收，默认为0，表示关闭
echo 1 > /proc/sys/net/ipv4/tcp_tw_recycle

#表示开启SYN Cookies。当出现SYN等待队列溢出时，启用cookies来处理，可防范少量SYN攻击，默认为0，表示关闭
echo 1 > /proc/sys/net/ipv4/tcp_syncookies

#表示SYN队列的长度，默认1024，加大队列长队为819200，可以容纳更多等待连接的网络连接数
echo 819200 > /proc/sys/net/ipv4/tcp_max_syn_backlog

#进入包的最大设备队列.默认是300,对重负载服务器而言,该值太低,可调整至一个较大的值
echo 819200 > /proc/sys/net/core/netdev_max_backlog

#listen()的默认参数,挂起请求的最大数量.默认是128.对繁忙的服务器,增加该值有助于网络性能.可调整至一个较大值
echo 819200 > /proc/sys/net/core/somaxconn

#tcp_mem[0]:低于此值,TCP没有内存压力; tcp_mem[1]:在此值下,进入内存压力阶段; tcp_mem[2]:高于此值,TCP拒绝分配socket
echo 94500000 915000000 927000000 > /proc/sys/net/ipv4/tcp_mem

#表示系统同时保持TIME_WAIT套接字的最大数量，如果超过这个数字，TIME_WAIT套接字将立刻被清除并打印警告信息。默认为180000
echo 5000 > /proc/sys/net/ipv4/tcp_max_tw_buckets

#对于一个新建连接，内核要发送多少个 SYN 连接请求才决定放弃。不应该大于255，默认值是5，对应于180秒左右
echo 1 > /proc/sys/net/ipv4/tcp_syn_retries
echo 1 > /proc/sys/net/ipv4/tcp_synack_retries

#修改系统最大打开文件数量和堆栈大小
echo 700000 > /proc/sys/fs/file-max
echo ulimit -n 700000 >> ~/.profile
echo ulimit -s 16384 >> ~/.profile
source ~/.profile

echo "init ok"

