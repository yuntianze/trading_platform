#!/bin/bash

# 定义变量
SERVER_NAME="gateway_server"
SERVER_PATH="./bin"
PID_FILE="/var/run/${SERVER_NAME}.pid"
LOG_FILE="/var/log/${SERVER_NAME}.log"

# 检查服务是否已经在运行
if [ -f $PID_FILE ]; then
    echo "$SERVER_NAME is already running, PID: $(cat $PID_FILE)"
    exit 1
fi

# 启动服务
echo "Starting $SERVER_NAME..."
$SERVER_PATH/$SERVER_NAME > $LOG_FILE 2>&1 &

# 获取PID并保存
PID=$!
echo $PID > $PID_FILE

echo "$SERVER_NAME started with PID: $PID"