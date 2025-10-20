#pragma once
#include <stdbool.h>
#include <stddef.h>
#include "forge.h"  // brings http_req_t typedef
bool http_try_parse_request(const char *buf, size_t len, http_req_t *out);
