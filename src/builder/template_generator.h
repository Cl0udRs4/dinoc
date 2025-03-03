/**
 * @file template_generator.h
 * @brief Template generator for builder
 */

#ifndef DINOC_TEMPLATE_GENERATOR_H
#define DINOC_TEMPLATE_GENERATOR_H

#include "builder.h"
#include "client_template.h"
#include "../include/common.h"
#include <stdint.h>
#include <stdbool.h>

// Template generator functions
status_t template_generator_init(void);
status_t template_generator_shutdown(void);
status_t template_generator_generate(const builder_config_t* builder_config, const char* output_file);

#endif /* DINOC_TEMPLATE_GENERATOR_H */
