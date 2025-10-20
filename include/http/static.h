#pragma once
#include <stdbool.h>
int  http_send_static_file(int fd, const char *doc_root, const char *url_path, bool keep_alive);
void http_send_404(int fd, bool keep_alive);
void http_send_400(int fd);
bool http_path_has_dotdot(const char *p);