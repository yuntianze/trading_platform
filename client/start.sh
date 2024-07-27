#!/bin/bash

# Define variables
CLIENT_NAME="tcp_client"
CLIENT_PATH="../build"  # Replace with the actual path
PID_FILE="/var/run/${CLIENT_NAME}.pid"
LOG_FILE="/var/log/${CLIENT_NAME}.log"

# Check if the client is already running
if [ -f $PID_FILE ]; then
    PID=$(cat $PID_FILE)
    # Use ps to check if the process with the PID exists
    if ps -p $PID > /dev/null; then
        echo "$CLIENT_NAME is already running, PID: $PID"
        exit 1
    else
        echo "PID file exists, but process not running. Removing PID file."
        rm $PID_FILE  # Remove stale PID file
    fi
fi

# Start the client
echo "Starting $CLIENT_NAME..."
$CLIENT_PATH/$CLIENT_NAME > $LOG_FILE 2>&1 &

# Get PID and save it
PID=$!
echo $PID > $PID_FILE

echo "$CLIENT_NAME started with PID: $PID"
