#!/bin/bash
# Simple helper to listen to TCP-exposed QEMU serial
PORT=4444
echo "Listening on TCP port $PORT for serial output..."
nc -l -p $PORT
