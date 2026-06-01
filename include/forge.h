#pragma once
#define _GNU_SOURCE
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <signal.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/sendfile.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#define MAX_EVENTS  4096
#define RECV_BUF    8192
#define HDR_BUF     2048

typedef struct {
  const char *listen_hostport; // "0.0.0.0:8080"
  const char *doc_root;        // "./public"
  int workers;                 // e.g., 4
  int listen_fd;               // created in master before fork
  int control_fd;
} forge_cfg_t;

typedef struct {
  int  fd;
  char rbuf[RECV_BUF];
  size_t rlen;
  bool keep_alive;
} forge_conn_t;

extern atomic_bool g_terminate;

/* net.c */
int  forge_create_listener(const char *hostport);
int  forge_set_nonblock(int fd);
int forge_send_fd(int sock, int fd_to_send);
int forge_recv_fd(int sock);

/* master.c */
void forge_install_signals(void);
void forge_master_run(forge_cfg_t *cfg);

/* worker.c */
void forge_run_worker(forge_cfg_t *cfg, int widx);

/* util/log.c */
void forge_log(const char *fmt, ...);
void forge_die(const char *fmt, ...);

/* http/parser.h */
typedef struct {
  char method[8];
  char path[1024];
  char version[16];
  bool keep_alive;
  size_t header_bytes; // end of headers offset (points after \r\n\r\n)
} http_req_t;

bool http_try_parse_request(const char *buf, size_t len, http_req_t *out);

/* http/static.h */
void http_send_400(int fd);
void http_send_404(int fd, bool keep_alive);
int  http_send_static_file(int fd, const char *doc_root, const char *url_path, bool keep_alive);
bool http_path_has_dotdot(const char *p);
