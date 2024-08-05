#!/bin/bash

# Define variables
SERVER_NAME="order_server"
SERVER_PATH="./bin"
PID_FILE="/var/run/${SERVER_NAME}.pid"
LOG_FILE="/var/log/${SERVER_NAME}.log"

# Check if the process is already running
if [ -f $PID_FILE ]; then
    PID=$(cat $PID_FILE)
    # Use ps to check if the process with the PID exists
    if ps -p $PID > /dev/null; then 
        echo "$SERVER_NAME is already running, PID: $PID"
        exit 1
    else
        echo "PID file exists, but process not running. Removing PID file."
        rm $PID_FILE  # Remove stale PID file
    fi
fi

# Start the server
echo "Starting $SERVER_NAME..."
$SERVER_PATH/$SERVER_NAME > $LOG_FILE 2>&1 &

# Get PID and save it
PID=$!
echo $PID > $PID_FILE

echo "$SERVER_NAME started with PID: $PID"
