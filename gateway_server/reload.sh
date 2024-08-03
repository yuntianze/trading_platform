#!/bin/bash

# Define variables
SERVER_NAME="gateway_server"
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

# Send SIGUSR1 to reload configuration
echo "Reloading $SERVER_NAME configuration..."
kill -SIGUSR1 $PID

echo "Sent reload signal to $SERVER_NAME."