#!/bin/bash

# Simple test script for HTTP API

# Configuration
API_HOST="localhost"
API_PORT="8080"
API_URL="http://$API_HOST:$API_PORT/api"

# Function to make API requests
function api_request() {
    local method=$1
    local endpoint=$2
    local data=$3
    
    echo "Making $method request to $API_URL/$endpoint"
    
    if [ -z "$data" ]; then
        curl -v -X "$method" "$API_URL/$endpoint"
    else
        curl -v -X "$method" -H "Content-Type: application/json" -d "$data" "$API_URL/$endpoint"
    fi
    
    echo -e "\n"
}

# Test creating a TCP listener
echo "Creating TCP listener..."
api_request "POST" "listeners" '{
    "protocol": "tcp",
    "bind_address": "0.0.0.0",
    "port": 9001,
    "timeout_ms": 30000,
    "auto_start": true
}'

# Test creating a UDP listener
echo "Creating UDP listener..."
api_request "POST" "listeners" '{
    "protocol": "udp",
    "bind_address": "0.0.0.0",
    "port": 9002,
    "timeout_ms": 30000,
    "auto_start": true
}'

# Test getting all listeners
echo "Getting all listeners..."
api_request "GET" "listeners" ""

# Test health endpoint
echo "Testing health endpoint..."
api_request "GET" "health" ""
