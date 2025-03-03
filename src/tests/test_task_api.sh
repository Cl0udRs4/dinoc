#!/bin/bash

# Test script for task management API

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

# Generate a random UUID
function generate_uuid() {
    python -c "import uuid; print(str(uuid.uuid4()))"
}

# Test creating a task
print_message "$YELLOW" "Creating a task..."
CLIENT_ID=$(generate_uuid)
TASK_DATA=$(cat <<EOF
{
    "client_id": "$CLIENT_ID",
    "type": 0,
    "data": "ZWNobyAiaGVsbG8gd29ybGQi",
    "timeout": 30
}
EOF
)

TASK_RESPONSE=$(api_request "POST" "tasks" "$TASK_DATA")
TASK_ID=$(echo $TASK_RESPONSE | python -c "import sys, json; print(json.load(sys.stdin)['id'])")

if [ -z "$TASK_ID" ]; then
    print_message "$RED" "Failed to create task"
    exit 1
fi

print_message "$GREEN" "Task created with ID: $TASK_ID"

# Test getting a task by ID
print_message "$YELLOW" "Getting task by ID..."
api_request "GET" "tasks/$TASK_ID" ""

# Test updating task state
print_message "$YELLOW" "Updating task state to RUNNING..."
api_request "PUT" "tasks/$TASK_ID" '{"state": 2}'

# Test setting task result
print_message "$YELLOW" "Setting task result..."
api_request "POST" "tasks/$TASK_ID" '{"result": "aGVsbG8gd29ybGQ="}'

# Test getting tasks for client
print_message "$YELLOW" "Getting tasks for client..."
api_request "GET" "clients/$CLIENT_ID" ""

# Test getting all tasks
print_message "$YELLOW" "Getting all tasks..."
api_request "GET" "tasks" ""

print_message "$GREEN" "Task API tests completed"
