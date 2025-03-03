/**
 * @file utils.c
 * @brief Utility functions implementation for C2 server
 */

#include "utils.h"
#include "logger.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/utsname.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <zlib.h>
#include <math.h>

// CRC32 table
static uint32_t crc32_table[256];
static bool crc32_table_initialized = false;

/**
 * @brief Initialize CRC32 table
 */
static void init_crc32_table(void) {
    if (crc32_table_initialized) {
        return;
    }
    
    for (uint32_t i = 0; i < 256; i++) {
        uint32_t c = i;
        for (int j = 0; j < 8; j++) {
            c = (c & 1) ? (0xEDB88320 ^ (c >> 1)) : (c >> 1);
        }
        crc32_table[i] = c;
    }
    
    crc32_table_initialized = true;
}

/**
 * @brief Calculate CRC32 checksum
 */
uint32_t utils_crc32(const uint8_t* data, size_t len) {
    if (data == NULL) {
        return 0;
    }
    
    init_crc32_table();
    
    uint32_t crc = 0xFFFFFFFF;
    
    for (size_t i = 0; i < len; i++) {
        crc = (crc >> 8) ^ crc32_table[(crc & 0xFF) ^ data[i]];
    }
    
    return crc ^ 0xFFFFFFFF;
}

/**
 * @brief Calculate Fletcher-16 checksum
 */
uint16_t utils_fletcher16(const uint8_t* data, size_t len) {
    if (data == NULL) {
        return 0;
    }
    
    uint16_t sum1 = 0;
    uint16_t sum2 = 0;
    
    for (size_t i = 0; i < len; i++) {
        sum1 = (sum1 + data[i]) % 255;
        sum2 = (sum2 + sum1) % 255;
    }
    
    return (sum2 << 8) | sum1;
}

/**
 * @brief Calculate entropy of data
 */
double utils_entropy(const uint8_t* data, size_t len) {
    if (data == NULL || len == 0) {
        return 0.0;
    }
    
    // Count byte frequencies
    size_t counts[256] = {0};
    
    for (size_t i = 0; i < len; i++) {
        counts[data[i]]++;
    }
    
    // Calculate entropy
    double entropy = 0.0;
    
    for (int i = 0; i < 256; i++) {
        if (counts[i] > 0) {
            double p = (double)counts[i] / len;
            entropy -= p * log2(p);
        }
    }
    
    return entropy;
}

/**
 * @brief Compress data using zlib
 */
status_t utils_compress(const uint8_t* data, size_t data_len, uint8_t** compressed_data, size_t* compressed_len) {
    if (data == NULL || compressed_data == NULL || compressed_len == NULL) {
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    // Allocate memory for compressed data (worst case: input size + 0.1% + 12 bytes)
    size_t max_compressed_len = data_len + (data_len / 1000) + 12;
    *compressed_data = malloc(max_compressed_len);
    
    if (*compressed_data == NULL) {
        return STATUS_ERROR_MEMORY;
    }
    
    // Compress data
    z_stream stream;
    memset(&stream, 0, sizeof(stream));
    
    if (deflateInit(&stream, Z_DEFAULT_COMPRESSION) != Z_OK) {
        free(*compressed_data);
        *compressed_data = NULL;
        return STATUS_ERROR_COMPRESSION;
    }
    
    stream.next_in = (Bytef*)data;
    stream.avail_in = data_len;
    stream.next_out = (Bytef*)*compressed_data;
    stream.avail_out = max_compressed_len;
    
    if (deflate(&stream, Z_FINISH) != Z_STREAM_END) {
        deflateEnd(&stream);
        free(*compressed_data);
        *compressed_data = NULL;
        return STATUS_ERROR_COMPRESSION;
    }
    
    *compressed_len = stream.total_out;
    
    deflateEnd(&stream);
    
    return STATUS_SUCCESS;
}

/**
 * @brief Decompress data using zlib
 */
status_t utils_decompress(const uint8_t* compressed_data, size_t compressed_len, uint8_t** data, size_t* data_len) {
    if (compressed_data == NULL || data == NULL || data_len == NULL) {
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    // Allocate memory for decompressed data (initial guess: 4x compressed size)
    size_t max_data_len = compressed_len * 4;
    *data = malloc(max_data_len);
    
    if (*data == NULL) {
        return STATUS_ERROR_MEMORY;
    }
    
    // Decompress data
    z_stream stream;
    memset(&stream, 0, sizeof(stream));
    
    if (inflateInit(&stream) != Z_OK) {
        free(*data);
        *data = NULL;
        return STATUS_ERROR_COMPRESSION;
    }
    
    stream.next_in = (Bytef*)compressed_data;
    stream.avail_in = compressed_len;
    stream.next_out = (Bytef*)*data;
    stream.avail_out = max_data_len;
    
    int ret = inflate(&stream, Z_FINISH);
    
    if (ret != Z_STREAM_END) {
        // If buffer is too small, try again with a larger buffer
        if (ret == Z_BUF_ERROR) {
            inflateEnd(&stream);
            
            // Reallocate with a larger buffer (8x compressed size)
            max_data_len = compressed_len * 8;
            *data = realloc(*data, max_data_len);
            
            if (*data == NULL) {
                return STATUS_ERROR_MEMORY;
            }
            
            memset(&stream, 0, sizeof(stream));
            
            if (inflateInit(&stream) != Z_OK) {
                free(*data);
                *data = NULL;
                return STATUS_ERROR_COMPRESSION;
            }
            
            stream.next_in = (Bytef*)compressed_data;
            stream.avail_in = compressed_len;
            stream.next_out = (Bytef*)*data;
            stream.avail_out = max_data_len;
            
            ret = inflate(&stream, Z_FINISH);
            
            if (ret != Z_STREAM_END) {
                inflateEnd(&stream);
                free(*data);
                *data = NULL;
                return STATUS_ERROR_COMPRESSION;
            }
        } else {
            inflateEnd(&stream);
            free(*data);
            *data = NULL;
            return STATUS_ERROR_COMPRESSION;
        }
    }
    
    *data_len = stream.total_out;
    
    inflateEnd(&stream);
    
    return STATUS_SUCCESS;
}

/**
 * @brief Generate random bytes
 */
status_t utils_random_bytes(uint8_t* buffer, size_t len) {
    if (buffer == NULL) {
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    int fd = open("/dev/urandom", O_RDONLY);
    if (fd == -1) {
        // Fallback to time-based random if /dev/urandom is not available
        srand(time(NULL) ^ getpid());
        
        for (size_t i = 0; i < len; i++) {
            buffer[i] = (uint8_t)rand();
        }
        
        return STATUS_SUCCESS;
    }
    
    size_t bytes_read = 0;
    while (bytes_read < len) {
        ssize_t ret = read(fd, buffer + bytes_read, len - bytes_read);
        
        if (ret <= 0) {
            close(fd);
            
            // Fallback to time-based random if read fails
            srand(time(NULL) ^ getpid());
            
            for (size_t i = bytes_read; i < len; i++) {
                buffer[i] = (uint8_t)rand();
            }
            
            return STATUS_SUCCESS;
        }
        
        bytes_read += ret;
    }
    
    close(fd);
    
    return STATUS_SUCCESS;
}

/**
 * @brief Get current timestamp in milliseconds
 */
uint64_t utils_get_timestamp(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    
    return (uint64_t)tv.tv_sec * 1000 + (uint64_t)tv.tv_usec / 1000;
}

/**
 * @brief Get hostname
 */
status_t utils_get_hostname(char* hostname, size_t size) {
    if (hostname == NULL) {
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    if (gethostname(hostname, size) != 0) {
        strncpy(hostname, "unknown", size);
        hostname[size - 1] = '\0';
        return STATUS_ERROR;
    }
    
    hostname[size - 1] = '\0';
    
    return STATUS_SUCCESS;
}

/**
 * @brief Get IP address
 */
status_t utils_get_ip_address(char* ip_address, size_t size) {
    if (ip_address == NULL) {
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    struct ifaddrs* ifaddr = NULL;
    if (getifaddrs(&ifaddr) == -1) {
        strncpy(ip_address, "unknown", size);
        ip_address[size - 1] = '\0';
        return STATUS_ERROR;
    }
    
    // Find the first non-loopback IPv4 address
    for (struct ifaddrs* ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr == NULL) {
            continue;
        }
        
        if (ifa->ifa_addr->sa_family == AF_INET) {
            struct sockaddr_in* addr = (struct sockaddr_in*)ifa->ifa_addr;
            
            // Skip loopback addresses
            if (ntohl(addr->sin_addr.s_addr) == 0x7F000001) {
                continue;
            }
            
            if (inet_ntop(AF_INET, &addr->sin_addr, ip_address, size) != NULL) {
                freeifaddrs(ifaddr);
                return STATUS_SUCCESS;
            }
        }
    }
    
    // If no suitable address found, try to get the hostname and resolve it
    char hostname[256];
    if (gethostname(hostname, sizeof(hostname)) == 0) {
        struct hostent* he = gethostbyname(hostname);
        
        if (he != NULL && he->h_addr_list[0] != NULL) {
            struct in_addr addr;
            memcpy(&addr, he->h_addr_list[0], sizeof(struct in_addr));
            
            if (inet_ntop(AF_INET, &addr, ip_address, size) != NULL) {
                freeifaddrs(ifaddr);
                return STATUS_SUCCESS;
            }
        }
    }
    
    // If all else fails, return "unknown"
    strncpy(ip_address, "unknown", size);
    ip_address[size - 1] = '\0';
    
    freeifaddrs(ifaddr);
    
    return STATUS_ERROR;
}

/**
 * @brief Get OS information
 */
status_t utils_get_os_info(char* os_info, size_t size) {
    if (os_info == NULL) {
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    struct utsname uts;
    if (uname(&uts) == -1) {
        strncpy(os_info, "unknown", size);
        os_info[size - 1] = '\0';
        return STATUS_ERROR;
    }
    
    snprintf(os_info, size, "%s %s %s %s", uts.sysname, uts.release, uts.version, uts.machine);
    os_info[size - 1] = '\0';
    
    return STATUS_SUCCESS;
}

// Base64 encoding table
static const char base64_table[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

/**
 * @brief Base64 encode
 */
status_t utils_base64_encode(const uint8_t* data, size_t data_len, char** encoded, size_t* encoded_len) {
    if (data == NULL || encoded == NULL || encoded_len == NULL) {
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    // Calculate encoded length (4 bytes for every 3 bytes, rounded up)
    *encoded_len = ((data_len + 2) / 3) * 4;
    
    // Allocate memory for encoded data
    *encoded = malloc(*encoded_len + 1); // +1 for null terminator
    
    if (*encoded == NULL) {
        return STATUS_ERROR_MEMORY;
    }
    
    // Encode data
    size_t i, j;
    for (i = 0, j = 0; i < data_len; i += 3, j += 4) {
        uint32_t octet_a = i < data_len ? data[i] : 0;
        uint32_t octet_b = i + 1 < data_len ? data[i + 1] : 0;
        uint32_t octet_c = i + 2 < data_len ? data[i + 2] : 0;
        
        uint32_t triple = (octet_a << 16) + (octet_b << 8) + octet_c;
        
        (*encoded)[j] = base64_table[(triple >> 18) & 0x3F];
        (*encoded)[j + 1] = base64_table[(triple >> 12) & 0x3F];
        (*encoded)[j + 2] = base64_table[(triple >> 6) & 0x3F];
        (*encoded)[j + 3] = base64_table[triple & 0x3F];
    }
    
    // Add padding
    if (data_len % 3 == 1) {
        (*encoded)[*encoded_len - 2] = '=';
        (*encoded)[*encoded_len - 1] = '=';
    } else if (data_len % 3 == 2) {
        (*encoded)[*encoded_len - 1] = '=';
    }
    
    // Add null terminator
    (*encoded)[*encoded_len] = '\0';
    
    return STATUS_SUCCESS;
}

/**
 * @brief Base64 decode
 */
status_t utils_base64_decode(const char* encoded, size_t encoded_len, uint8_t** data, size_t* data_len) {
    if (encoded == NULL || data == NULL || data_len == NULL) {
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    // If encoded_len is 0, use strlen
    if (encoded_len == 0) {
        encoded_len = strlen(encoded);
    }
    
    // Check if encoded length is valid
    if (encoded_len % 4 != 0) {
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    // Calculate decoded length (3 bytes for every 4 bytes, adjusted for padding)
    *data_len = (encoded_len / 4) * 3;
    
    if (encoded_len >= 1 && encoded[encoded_len - 1] == '=') {
        (*data_len)--;
    }
    
    if (encoded_len >= 2 && encoded[encoded_len - 2] == '=') {
        (*data_len)--;
    }
    
    // Allocate memory for decoded data
    *data = malloc(*data_len);
    
    if (*data == NULL) {
        return STATUS_ERROR_MEMORY;
    }
    
    // Create reverse lookup table
    uint8_t reverse_table[256];
    memset(reverse_table, 0xFF, sizeof(reverse_table));
    
    for (int i = 0; i < 64; i++) {
        reverse_table[(uint8_t)base64_table[i]] = i;
    }
    
    // Decode data
    size_t i, j;
    for (i = 0, j = 0; i < encoded_len; i += 4, j += 3) {
        uint8_t sextet_a = reverse_table[(uint8_t)encoded[i]];
        uint8_t sextet_b = reverse_table[(uint8_t)encoded[i + 1]];
        uint8_t sextet_c = reverse_table[(uint8_t)encoded[i + 2]];
        uint8_t sextet_d = reverse_table[(uint8_t)encoded[i + 3]];
        
        // Check if any character is invalid
        if (sextet_a == 0xFF || sextet_b == 0xFF ||
            (sextet_c == 0xFF && encoded[i + 2] != '=') ||
            (sextet_d == 0xFF && encoded[i + 3] != '=')) {
            free(*data);
            *data = NULL;
            return STATUS_ERROR_INVALID_PARAM;
        }
        
        uint32_t triple = (sextet_a << 18) + (sextet_b << 12) +
                          (sextet_c << 6) + sextet_d;
        
        if (j < *data_len) {
            (*data)[j] = (triple >> 16) & 0xFF;
        }
        
        if (j + 1 < *data_len) {
            (*data)[j + 1] = (triple >> 8) & 0xFF;
        }
        
        if (j + 2 < *data_len) {
            (*data)[j + 2] = triple & 0xFF;
        }
    }
    
    return STATUS_SUCCESS;
}

/**
 * @brief Hex encode
 */
status_t utils_hex_encode(const uint8_t* data, size_t data_len, char** encoded, size_t* encoded_len) {
    if (data == NULL || encoded == NULL || encoded_len == NULL) {
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    // Calculate encoded length (2 characters for each byte)
    *encoded_len = data_len * 2;
    
    // Allocate memory for encoded data
    *encoded = malloc(*encoded_len + 1); // +1 for null terminator
    
    if (*encoded == NULL) {
        return STATUS_ERROR_MEMORY;
    }
    
    // Encode data
    for (size_t i = 0; i < data_len; i++) {
        sprintf(*encoded + (i * 2), "%02x", data[i]);
    }
    
    // Add null terminator
    (*encoded)[*encoded_len] = '\0';
    
    return STATUS_SUCCESS;
}

/**
 * @brief Hex decode
 */
status_t utils_hex_decode(const char* encoded, size_t encoded_len, uint8_t** data, size_t* data_len) {
    if (encoded == NULL || data == NULL || data_len == NULL) {
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    // If encoded_len is 0, use strlen
    if (encoded_len == 0) {
        encoded_len = strlen(encoded);
    }
    
    // Check if encoded length is valid
    if (encoded_len % 2 != 0) {
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    // Calculate decoded length (1 byte for every 2 characters)
    *data_len = encoded_len / 2;
    
    // Allocate memory for decoded data
    *data = malloc(*data_len);
    
    if (*data == NULL) {
        return STATUS_ERROR_MEMORY;
    }
    
    // Decode data
    for (size_t i = 0; i < *data_len; i++) {
        char hex[3] = {encoded[i * 2], encoded[i * 2 + 1], '\0'};
        
        // Check if characters are valid hex
        for (int j = 0; j < 2; j++) {
            if (!((hex[j] >= '0' && hex[j] <= '9') ||
                  (hex[j] >= 'a' && hex[j] <= 'f') ||
                  (hex[j] >= 'A' && hex[j] <= 'F'))) {
                free(*data);
                *data = NULL;
                return STATUS_ERROR_INVALID_PARAM;
            }
        }
        
        (*data)[i] = (uint8_t)strtol(hex, NULL, 16);
    }
    
    return STATUS_SUCCESS;
}
