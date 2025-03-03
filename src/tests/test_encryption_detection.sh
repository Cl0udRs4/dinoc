#!/bin/bash

# Test script for encryption detection

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

# Test with AES encryption
print_message "$YELLOW" "Testing with AES encryption..."
./test_client 127.0.0.1 9001 1

# Wait a bit
sleep 2

# Test with ChaCha20 encryption
print_message "$YELLOW" "Testing with ChaCha20 encryption..."
./test_client 127.0.0.1 9001 2

print_message "$GREEN" "Encryption detection tests completed"
