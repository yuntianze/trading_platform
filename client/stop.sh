#!/bin/bash

# 定义变量
CLIENT_NAME="tcp_client"
PID_FILE="/var/run/${CLIENT_NAME}.pid"

# 检查PID文件是否存在
if [ ! -f $PID_FILE ]; then
    echo "$CLIENT_NAME is not running."
    exit 1
fi

# 读取PID
PID=$(cat $PID_FILE)

# 检查进程是否存在
if ! kill -0 $PID 2>/dev/null; then
    echo "$CLIENT_NAME is not running, but PID file exists. Cleaning up."
    rm $PID_FILE
    exit 1
fi

# 停止客户端
echo "Stopping $CLIENT_NAME..."
kill $PID

# 等待进程结束
for i in {1..10}; do
    if ! kill -0 $PID 2>/dev/null; then
        echo "$CLIENT_NAME stopped."
        rm $PID_FILE
        exit 0
    fi
    sleep 1
done

# 如果进程没有在10秒内停止，强制终止
echo "$CLIENT_NAME did not stop gracefully. Forcing stop..."
kill -9 $PID
rm $PID_FILE

echo "$CLIENT_NAME forcefully stopped."