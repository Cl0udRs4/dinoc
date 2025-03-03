#!/bin/bash

# Simple test script for HTTP API using curl

# Configuration
API_HOST="localhost"
API_PORT="8080"
API_URL="http://$API_HOST:$API_PORT/api"

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[0;33m'
NC='\033[0m' # No Color

# Function to print colored messages
function print_message() {
    local color=$1
    local message=$2
    echo -e "${color}${message}${NC}"
}

# Function to make API requests
function api_request() {
    local method=$1
    local endpoint=$2
    local data=$3
    
    print_message "$YELLOW" "Making $method request to $API_URL/$endpoint"
    
    if [ -z "$data" ]; then
        curl -s -X "$method" "$API_URL/$endpoint"
    else
        curl -s -X "$method" -H "Content-Type: application/json" -d "$data" "$API_URL/$endpoint"
    fi
    
    echo -e "\n"
}

# Test health endpoint
print_message "$YELLOW" "Testing health endpoint..."
api_request "GET" "health" ""

# Test creating a TCP listener
print_message "$YELLOW" "Creating TCP listener..."
TCP_RESPONSE=$(api_request "POST" "listeners" '{
    "protocol": "tcp",
    "bind_address": "0.0.0.0",
    "port": 9001,
    "timeout_ms": 30000,
    "auto_start": true
}')

# Extract listener ID from response
TCP_ID=$(echo "$TCP_RESPONSE" | grep -o '"id":"[^"]*"' | cut -d'"' -f4)

if [ -n "$TCP_ID" ]; then
    print_message "$GREEN" "TCP listener created with ID: $TCP_ID"
else
    print_message "$RED" "Failed to create TCP listener"
    exit 1
fi

# Test getting TCP listener status
print_message "$YELLOW" "Getting TCP listener status..."
api_request "GET" "listeners/$TCP_ID" ""

# Test stopping TCP listener
print_message "$YELLOW" "Stopping TCP listener..."
api_request "POST" "listeners/$TCP_ID/stop" ""

# Test getting TCP listener status again
print_message "$YELLOW" "Getting TCP listener status after stopping..."
api_request "GET" "listeners/$TCP_ID" ""

# Test starting TCP listener
print_message "$YELLOW" "Starting TCP listener..."
api_request "POST" "listeners/$TCP_ID/start" ""

# Test getting TCP listener status again
print_message "$YELLOW" "Getting TCP listener status after starting..."
api_request "GET" "listeners/$TCP_ID" ""

# Test creating a UDP listener
print_message "$YELLOW" "Creating UDP listener..."
UDP_RESPONSE=$(api_request "POST" "listeners" '{
    "protocol": "udp",
    "bind_address": "0.0.0.0",
    "port": 9002,
    "timeout_ms": 30000,
    "auto_start": true
}')

# Extract listener ID from response
UDP_ID=$(echo "$UDP_RESPONSE" | grep -o '"id":"[^"]*"' | cut -d'"' -f4)

if [ -n "$UDP_ID" ]; then
    print_message "$GREEN" "UDP listener created with ID: $UDP_ID"
else
    print_message "$RED" "Failed to create UDP listener"
fi

# Test getting all listeners
print_message "$YELLOW" "Getting all listeners..."
api_request "GET" "listeners" ""

# Test deleting TCP listener
print_message "$YELLOW" "Deleting TCP listener..."
api_request "DELETE" "listeners/$TCP_ID" ""

# Test deleting UDP listener
if [ -n "$UDP_ID" ]; then
    print_message "$YELLOW" "Deleting UDP listener..."
    api_request "DELETE" "listeners/$UDP_ID" ""
fi

# Test getting all listeners again
print_message "$YELLOW" "Getting all listeners after deletion..."
api_request "GET" "listeners" ""

print_message "$GREEN" "API tests completed"
