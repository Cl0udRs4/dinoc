#!/bin/bash

# Test script for HTTP API

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
    
    if [ -z "$data" ]; then
        curl -s -X "$method" "$API_URL/$endpoint"
    else
        curl -s -X "$method" -H "Content-Type: application/json" -d "$data" "$API_URL/$endpoint"
    fi
}

# Test creating a TCP listener
function test_create_tcp_listener() {
    print_message "$YELLOW" "Creating TCP listener..."
    
    local data='{
        "protocol": "tcp",
        "bind_address": "0.0.0.0",
        "port": 9001,
        "timeout_ms": 30000,
        "auto_start": true
    }'
    
    local response=$(api_request "POST" "listeners" "$data")
    echo "$response"
    
    # Extract listener ID from response
    local listener_id=$(echo "$response" | grep -o '"id":"[^"]*"' | cut -d'"' -f4)
    
    if [ -n "$listener_id" ]; then
        print_message "$GREEN" "TCP listener created with ID: $listener_id"
        echo "$listener_id"
    else
        print_message "$RED" "Failed to create TCP listener"
        echo ""
    fi
}

# Test getting listener status
function test_get_listener_status() {
    local listener_id=$1
    
    print_message "$YELLOW" "Getting status for listener $listener_id..."
    
    local response=$(api_request "GET" "listeners/$listener_id" "")
    echo "$response"
    
    # Check if response contains state
    if echo "$response" | grep -q '"state"'; then
        print_message "$GREEN" "Successfully got listener status"
    else
        print_message "$RED" "Failed to get listener status"
    fi
}

# Test stopping a listener
function test_stop_listener() {
    local listener_id=$1
    
    print_message "$YELLOW" "Stopping listener $listener_id..."
    
    local response=$(api_request "POST" "listeners/$listener_id/stop" "")
    echo "$response"
    
    # Check if response indicates success
    if echo "$response" | grep -q '"success"'; then
        print_message "$GREEN" "Successfully stopped listener"
    else
        print_message "$RED" "Failed to stop listener"
    fi
}

# Test starting a listener
function test_start_listener() {
    local listener_id=$1
    
    print_message "$YELLOW" "Starting listener $listener_id..."
    
    local response=$(api_request "POST" "listeners/$listener_id/start" "")
    echo "$response"
    
    # Check if response indicates success
    if echo "$response" | grep -q '"success"'; then
        print_message "$GREEN" "Successfully started listener"
    else
        print_message "$RED" "Failed to start listener"
    fi
}

# Test deleting a listener
function test_delete_listener() {
    local listener_id=$1
    
    print_message "$YELLOW" "Deleting listener $listener_id..."
    
    local response=$(api_request "DELETE" "listeners/$listener_id" "")
    echo "$response"
    
    # Check if response indicates success
    if echo "$response" | grep -q '"success"'; then
        print_message "$GREEN" "Successfully deleted listener"
    else
        print_message "$RED" "Failed to delete listener"
    fi
}

# Main test sequence
function run_tests() {
    print_message "$YELLOW" "Starting API tests..."
    
    # Create TCP listener
    local listener_id=$(test_create_tcp_listener)
    
    if [ -z "$listener_id" ]; then
        print_message "$RED" "Cannot continue tests without a listener ID"
        return 1
    fi
    
    # Get listener status
    test_get_listener_status "$listener_id"
    
    # Stop listener
    test_stop_listener "$listener_id"
    
    # Get listener status again
    test_get_listener_status "$listener_id"
    
    # Start listener
    test_start_listener "$listener_id"
    
    # Get listener status again
    test_get_listener_status "$listener_id"
    
    # Delete listener
    test_delete_listener "$listener_id"
    
    print_message "$GREEN" "API tests completed"
}

# Check if API server is running
function check_api_server() {
    if curl -s -o /dev/null -w "%{http_code}" "$API_URL/health" | grep -q "200"; then
        print_message "$GREEN" "API server is running"
        return 0
    else
        print_message "$RED" "API server is not running at $API_URL"
        return 1
    fi
}

# Main
if check_api_server; then
    run_tests
else
    print_message "$YELLOW" "Please start the API server and try again"
    exit 1
fi
