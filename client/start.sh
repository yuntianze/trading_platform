#!/bin/bash

# 定义变量
CLIENT_NAME="tcp_client"
CLIENT_PATH="../build"  # 请替换为实际的路径
PID_FILE="/var/run/${CLIENT_NAME}.pid"
LOG_FILE="/var/log/${CLIENT_NAME}.log"

# 检查客户端是否已经在运行
if [ -f $PID_FILE ]; then
    echo "$CLIENT_NAME is already running, PID: $(cat $PID_FILE)"
    exit 1
fi

# 启动客户端
echo "Starting $CLIENT_NAME..."
$CLIENT_PATH/$CLIENT_NAME > $LOG_FILE 2>&1 &

# 获取PID并保存
PID=$!
echo $PID > $PID_FILE

echo "$CLIENT_NAME started with PID: $PID"