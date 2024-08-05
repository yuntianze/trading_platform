#!/bin/bash

# Define variables
SERVER_NAME="order_server"
PID_FILE="/var/run/${SERVER_NAME}.pid"

# Check if PID file exists
if [ ! -f $PID_FILE ]; then
    echo "$SERVER_NAME is not running."
    exit 1
fi

# Read PID
PID=$(cat $PID_FILE)

# Check if process exists
if ! kill -0 $PID 2>/dev/null; then
    echo "$SERVER_NAME is not running, but PID file exists. Cleaning up."
    rm $PID_FILE
    exit 1
fi

# Stop the server using SIGUSR2
echo "Stopping $SERVER_NAME..."
kill -SIGUSR2 $PID

# Wait for process to end
for i in {1..30}; do
    if ! kill -0 $PID 2>/dev/null; then
        echo "$SERVER_NAME stopped."
        rm $PID_FILE
        exit 0
    fi
    sleep 1
done

# If process didn't stop within 30 seconds, force terminate
echo "$SERVER_NAME did not stop gracefully. Forcing stop..."
kill -9 $PID
rm $PID_FILE

echo "$SERVER_NAME forcefully stopped."