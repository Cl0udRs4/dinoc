/**
 * @file task_api.c
 * @brief Task management API endpoints
 */

#include "../include/api.h"
#include "../include/common.h"
#include "../include/task.h"
#include "../common/uuid.h"
#include "../common/base64.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <jansson.h>

// Forward declarations
static json_t* task_to_json(const task_t* task);
static json_t* tasks_to_json(const task_t** tasks, size_t count);

/**
 * @brief Register task management API handlers
 */
status_t register_task_api_handlers(void) {
    // Register task management endpoints
    status_t status = http_server_register_handler("/api/tasks", "GET", api_tasks_get);
    if (status != STATUS_SUCCESS) {
        return status;
    }
    
    status = http_server_register_handler("/api/tasks", "POST", api_tasks_post);
    if (status != STATUS_SUCCESS) {
        return status;
    }
    
    status = http_server_register_handler("/api/tasks/", "GET", api_task_get);
    if (status != STATUS_SUCCESS) {
        return status;
    }
    
    status = http_server_register_handler("/api/tasks/", "PUT", api_task_state_put);
    if (status != STATUS_SUCCESS) {
        return status;
    }
    
    status = http_server_register_handler("/api/tasks/", "POST", api_task_result_post);
    if (status != STATUS_SUCCESS) {
        return status;
    }
    
    status = http_server_register_handler("/api/clients/", "GET", api_client_tasks_get);
    if (status != STATUS_SUCCESS) {
        return status;
    }
    
    return STATUS_SUCCESS;
}

/**
 * @brief Get all tasks API handler
 */
status_t api_tasks_get(struct MHD_Connection* connection,
                     const char* url, const char* method,
                     const char* upload_data, size_t upload_data_size) {
    // TODO: Implement get all tasks
    // For now, return empty array
    
    // Create JSON response
    json_t* json = json_array();
    if (json == NULL) {
        return STATUS_ERROR_MEMORY;
    }
    
    // Send response
    status_t status = http_server_send_json_response(connection, 200, json);
    
    // Free JSON
    json_decref(json);
    
    return status;
}

/**
 * @brief Create task API handler
 */
status_t api_tasks_post(struct MHD_Connection* connection,
                      const char* url, const char* method,
                      const char* upload_data, size_t upload_data_size) {
    // Parse JSON request
    json_t* json = NULL;
    status_t status = http_server_parse_json_request(upload_data, upload_data_size, &json);
    
    if (status != STATUS_SUCCESS) {
        return http_server_send_response(connection, 400, "text/plain", "Invalid JSON");
    }
    
    // Extract client ID
    json_t* client_id_json = json_object_get(json, "client_id");
    if (!json_is_string(client_id_json)) {
        json_decref(json);
        return http_server_send_response(connection, 400, "text/plain", "Missing client_id");
    }
    
    const char* client_id_str = json_string_value(client_id_json);
    uuid_t client_id;
    
    status = uuid_from_string(client_id_str, client_id);
    if (status != STATUS_SUCCESS) {
        json_decref(json);
        return http_server_send_response(connection, 400, "text/plain", "Invalid client_id");
    }
    
    // Extract task type
    json_t* type_json = json_object_get(json, "type");
    if (!json_is_integer(type_json)) {
        json_decref(json);
        return http_server_send_response(connection, 400, "text/plain", "Missing type");
    }
    
    task_type_t type = (task_type_t)json_integer_value(type_json);
    
    // Extract data
    json_t* data_json = json_object_get(json, "data");
    uint8_t* data = NULL;
    size_t data_len = 0;
    
    if (json_is_string(data_json)) {
        const char* data_str = json_string_value(data_json);
        size_t str_len = strlen(data_str);
        data = (uint8_t*)malloc(str_len); // Allocate enough space for decoded data
        if (data != NULL) {
            data_len = base64_decode(data_str, str_len, data, str_len);
        }
    }
    
    // Extract timeout
    json_t* timeout_json = json_object_get(json, "timeout");
    uint32_t timeout = 0;
    
    if (json_is_integer(timeout_json)) {
        timeout = (uint32_t)json_integer_value(timeout_json);
    }
    
    // Create task
    task_t* task = NULL;
    status = task_create(&client_id, type, data, data_len, timeout, &task);
    
    // Free data
    if (data != NULL) {
        free(data);
    }
    
    // Check status
    if (status != STATUS_SUCCESS) {
        json_decref(json);
        return http_server_send_response(connection, 500, "text/plain", "Failed to create task");
    }
    
    // Create JSON response
    json_t* response_json = task_to_json(task);
    if (response_json == NULL) {
        json_decref(json);
        return http_server_send_response(connection, 500, "text/plain", "Failed to create response");
    }
    
    // Send response
    status = http_server_send_json_response(connection, 201, response_json);
    
    // Free JSON
    json_decref(json);
    json_decref(response_json);
    
    return status;
}

/**
 * @brief Get task by ID API handler
 */
status_t api_task_get(struct MHD_Connection* connection,
                    const char* url, const char* method,
                    const char* upload_data, size_t upload_data_size) {
    // Extract task ID from URL
    uuid_t task_id;
    status_t status = http_server_extract_uuid_from_url(url, "/api/tasks/", task_id);
    
    if (status != STATUS_SUCCESS) {
        return http_server_send_response(connection, 400, "text/plain", "Invalid task ID");
    }
    
    // Find task
    task_t* task = task_find(&task_id);
    if (task == NULL) {
        return http_server_send_response(connection, 404, "text/plain", "Task not found");
    }
    
    // Create JSON response
    json_t* json = task_to_json(task);
    if (json == NULL) {
        return http_server_send_response(connection, 500, "text/plain", "Failed to create response");
    }
    
    // Send response
    status = http_server_send_json_response(connection, 200, json);
    
    // Free JSON
    json_decref(json);
    
    return status;
}

/**
 * @brief Update task state API handler
 */
status_t api_task_state_put(struct MHD_Connection* connection,
                          const char* url, const char* method,
                          const char* upload_data, size_t upload_data_size) {
    // Extract task ID from URL
    uuid_t task_id;
    status_t status = http_server_extract_uuid_from_url(url, "/api/tasks/", task_id);
    
    if (status != STATUS_SUCCESS) {
        return http_server_send_response(connection, 400, "text/plain", "Invalid task ID");
    }
    
    // Find task
    task_t* task = task_find(&task_id);
    if (task == NULL) {
        return http_server_send_response(connection, 404, "text/plain", "Task not found");
    }
    
    // Parse JSON request
    json_t* json = NULL;
    status = http_server_parse_json_request(upload_data, upload_data_size, &json);
    
    if (status != STATUS_SUCCESS) {
        return http_server_send_response(connection, 400, "text/plain", "Invalid JSON");
    }
    
    // Extract state
    json_t* state_json = json_object_get(json, "state");
    if (!json_is_integer(state_json)) {
        json_decref(json);
        return http_server_send_response(connection, 400, "text/plain", "Missing state");
    }
    
    task_state_t state = (task_state_t)json_integer_value(state_json);
    
    // Update task state
    status = task_update_state(task, state);
    
    // Check status
    if (status != STATUS_SUCCESS) {
        json_decref(json);
        return http_server_send_response(connection, 500, "text/plain", "Failed to update task state");
    }
    
    // Create JSON response
    json_t* response_json = task_to_json(task);
    if (response_json == NULL) {
        json_decref(json);
        return http_server_send_response(connection, 500, "text/plain", "Failed to create response");
    }
    
    // Send response
    status = http_server_send_json_response(connection, 200, response_json);
    
    // Free JSON
    json_decref(json);
    json_decref(response_json);
    
    return status;
}

/**
 * @brief Set task result API handler
 */
status_t api_task_result_post(struct MHD_Connection* connection,
                            const char* url, const char* method,
                            const char* upload_data, size_t upload_data_size) {
    // Extract task ID from URL
    uuid_t task_id;
    status_t status = http_server_extract_uuid_from_url(url, "/api/tasks/", task_id);
    
    if (status != STATUS_SUCCESS) {
        return http_server_send_response(connection, 400, "text/plain", "Invalid task ID");
    }
    
    // Find task
    task_t* task = task_find(&task_id);
    if (task == NULL) {
        return http_server_send_response(connection, 404, "text/plain", "Task not found");
    }
    
    // Parse JSON request
    json_t* json = NULL;
    status = http_server_parse_json_request(upload_data, upload_data_size, &json);
    
    if (status != STATUS_SUCCESS) {
        return http_server_send_response(connection, 400, "text/plain", "Invalid JSON");
    }
    
    // Extract result
    json_t* result_json = json_object_get(json, "result");
    if (!json_is_string(result_json)) {
        json_decref(json);
        return http_server_send_response(connection, 400, "text/plain", "Missing result");
    }
    
    const char* result_str = json_string_value(result_json);
    size_t result_len = 0;
    size_t str_len = strlen(result_str);
    uint8_t* result = (uint8_t*)malloc(str_len); // Allocate enough space for decoded data
    if (result != NULL) {
        result_len = base64_decode(result_str, str_len, result, str_len);
    }
    
    if (result == NULL) {
        json_decref(json);
        return http_server_send_response(connection, 400, "text/plain", "Invalid result");
    }
    
    // Extract error
    json_t* error_json = json_object_get(json, "error");
    const char* error = NULL;
    
    if (json_is_string(error_json)) {
        error = json_string_value(error_json);
    }
    
    // Set task result
    if (error != NULL) {
        status = task_set_error(task, error);
    } else {
        status = task_set_result(task, result, result_len);
    }
    
    // Free result
    free(result);
    
    // Check status
    if (status != STATUS_SUCCESS) {
        json_decref(json);
        return http_server_send_response(connection, 500, "text/plain", "Failed to set task result");
    }
    
    // Create JSON response
    json_t* response_json = task_to_json(task);
    if (response_json == NULL) {
        json_decref(json);
        return http_server_send_response(connection, 500, "text/plain", "Failed to create response");
    }
    
    // Send response
    status = http_server_send_json_response(connection, 200, response_json);
    
    // Free JSON
    json_decref(json);
    json_decref(response_json);
    
    return status;
}

/**
 * @brief Get tasks for client API handler
 */
status_t api_client_tasks_get(struct MHD_Connection* connection,
                            const char* url, const char* method,
                            const char* upload_data, size_t upload_data_size) {
    // Extract client ID from URL
    uuid_t client_id;
    status_t status = http_server_extract_uuid_from_url(url, "/api/clients/", client_id);
    
    if (status != STATUS_SUCCESS) {
        return http_server_send_response(connection, 400, "text/plain", "Invalid client ID");
    }
    
    // Get tasks for client
    task_t** tasks = NULL;
    size_t task_count = 0;
    
    status = task_get_for_client(&client_id, &tasks, &task_count);
    if (status != STATUS_SUCCESS) {
        return http_server_send_response(connection, 500, "text/plain", "Failed to get tasks");
    }
    
    // Create JSON response
    json_t* json = tasks_to_json(tasks, task_count);
    if (json == NULL) {
        free(tasks);
        return http_server_send_response(connection, 500, "text/plain", "Failed to create response");
    }
    
    // Send response
    status = http_server_send_json_response(connection, 200, json);
    
    // Free JSON and tasks
    json_decref(json);
    free(tasks);
    
    return status;
}

/**
 * @brief Convert task to JSON
 */
static json_t* task_to_json(const task_t* task) {
    if (task == NULL) {
        return NULL;
    }
    
    // Create JSON object
    json_t* json = json_object();
    if (json == NULL) {
        return NULL;
    }
    
    // Add task ID
    char task_id_str[37];
    uuid_to_string(task->id, task_id_str, sizeof(task_id_str));
    json_object_set_new(json, "id", json_string(task_id_str));
    
    // Add client ID
    char client_id_str[37];
    uuid_to_string(task->client_id, client_id_str, sizeof(client_id_str));
    json_object_set_new(json, "client_id", json_string(client_id_str));
    
    // Add task type
    json_object_set_new(json, "type", json_integer(task->type));
    
    // Add task state
    json_object_set_new(json, "state", json_integer(task->state));
    
    // Add timeout
    json_object_set_new(json, "timeout", json_integer(task->timeout));
    
    // Add timestamps
    json_object_set_new(json, "created_time", json_integer(task->created_time));
    
    if (task->sent_time > 0) {
        json_object_set_new(json, "sent_time", json_integer(task->sent_time));
    }
    
    if (task->start_time > 0) {
        json_object_set_new(json, "start_time", json_integer(task->start_time));
    }
    
    if (task->end_time > 0) {
        json_object_set_new(json, "end_time", json_integer(task->end_time));
    }
    
    // Add data
    if (task->data != NULL && task->data_len > 0) {
        // Calculate required buffer size for base64 encoding (4*n/3 + padding)
        size_t output_len = ((task->data_len + 2) / 3) * 4 + 1;
        char* data_base64 = (char*)malloc(output_len);
        if (data_base64 != NULL) {
            size_t encoded_len = base64_encode(task->data, task->data_len, data_base64, output_len);
            if (encoded_len > 0) {
                data_base64[encoded_len] = '\0'; // Ensure null termination
                json_object_set_new(json, "data", json_string(data_base64));
            }
            free(data_base64);
        }
    }
    
    // Add result
    if (task->result != NULL && task->result_len > 0) {
        // Calculate required buffer size for base64 encoding (4*n/3 + padding)
        size_t output_len = ((task->result_len + 2) / 3) * 4 + 1;
        char* result_base64 = (char*)malloc(output_len);
        if (result_base64 != NULL) {
            size_t encoded_len = base64_encode(task->result, task->result_len, result_base64, output_len);
            if (encoded_len > 0) {
                result_base64[encoded_len] = '\0'; // Ensure null termination
                json_object_set_new(json, "result", json_string(result_base64));
            }
            free(result_base64);
        }
    }
    
    // Add error message
    if (task->error_message != NULL) {
        json_object_set_new(json, "error", json_string(task->error_message));
    }
    
    return json;
}

/**
 * @brief Convert tasks array to JSON
 */
static json_t* tasks_to_json(const task_t** tasks, size_t count) {
    if (tasks == NULL) {
        return NULL;
    }
    
    // Create JSON array
    json_t* json = json_array();
    if (json == NULL) {
        return NULL;
    }
    
    // Add tasks
    for (size_t i = 0; i < count; i++) {
        json_t* task_json = task_to_json(tasks[i]);
        if (task_json != NULL) {
            json_array_append_new(json, task_json);
        }
    }
    
    return json;
}
