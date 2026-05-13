#include "forge.h"
#include "http/parser.h"
#include "http/static.h"

atomic_bool g_terminate = false; // defined here, used by others

static void conn_close(forge_conn_t *c, int ep) {
  if (c->fd >= 0) {
    epoll_ctl(ep, EPOLL_CTL_DEL, c->fd, NULL);
    close(c->fd);
  }
  free(c);
}

static void accept_loop(int ep, int lfd) {
  for (;;) {
    struct sockaddr_in cli; socklen_t sl = sizeof(cli);
    int cfd = accept4(lfd, (struct sockaddr *)&cli, &sl, SOCK_NONBLOCK);
    if (cfd < 0) {
      if (errno == EAGAIN || errno == EWOULDBLOCK) break;
      if (errno == EINTR) continue;
      break;
    }
    forge_conn_t *c = calloc(1, sizeof(*c));
    c->fd = cfd; c->rlen = 0; c->keep_alive = true;

    struct epoll_event ev = {0};
    ev.events = EPOLLIN | EPOLLRDHUP | EPOLLHUP | EPOLLERR;
    ev.data.ptr = c;
    if (epoll_ctl(ep, EPOLL_CTL_ADD, cfd, &ev) < 0) { close(cfd); free(c); }
  }
}

static void handle_read(forge_cfg_t *cfg, forge_conn_t *c, int ep) {
  for (;;) {
    ssize_t n = read(c->fd, c->rbuf + c->rlen, RECV_BUF - c->rlen - 1);
    if (n > 0) {
      c->rlen += (size_t)n;
      if (c->rlen >= RECV_BUF - 1) { c->keep_alive=false; break; }

      http_req_t req;
      if (!http_try_parse_request(c->rbuf, c->rlen, &req)) {
        // need more bytes
        break;
      }

      if (strcmp(req.method, "GET") != 0) { http_send_400(c->fd); c->keep_alive = false; break; }

      c->keep_alive = req.keep_alive;
      // serve
      if (http_send_static_file(c->fd, cfg->doc_root, req.path, c->keep_alive) < 0) {
        c->keep_alive = false;
      }

      // reset buffer for next request (keep-alive)
      size_t remain = c->rlen - req.header_bytes;
      if (remain > 0) memmove(c->rbuf, c->rbuf + req.header_bytes, remain);
      c->rlen = 0; // simple path
      break;
    } else if (n == 0) {
      c->keep_alive = false; break;
    } else {
      if (errno == EAGAIN || errno == EWOULDBLOCK) break;
      c->keep_alive = false; break;
    }
  }

  if (!c->keep_alive) conn_close(c, ep);
}

void forge_run_worker(forge_cfg_t *cfg, int widx) {
  int ep = epoll_create1(EPOLL_CLOEXEC);
  if (ep < 0) forge_die("epoll_create1: %s", strerror(errno));

  struct epoll_event ev = {0};
  ev.events = EPOLLIN;
  ev.data.ptr = NULL;
  if (epoll_ctl(ep, EPOLL_CTL_ADD, cfg->listen_fd, &ev) < 0) forge_die("epoll_ctl ADD listen: %s", strerror(errno));

  forge_log("[worker %d] started pid=%d", widx, getpid());

  while (!atomic_load(&g_terminate)) {
    struct epoll_event events[MAX_EVENTS];
    int n = epoll_wait(ep, events, MAX_EVENTS, 1000);
    if (n < 0) {
      if (errno == EINTR) continue;
      break;
    }
    for (int i = 0; i < n; i++) {
      if (events[i].data.ptr == NULL) { accept_loop(ep, cfg->listen_fd); continue; }
      forge_conn_t *c = (forge_conn_t *)events[i].data.ptr;
      if (events[i].events & (EPOLLHUP | EPOLLERR | EPOLLRDHUP)) { conn_close(c, ep); continue; }
      if (events[i].events & EPOLLIN) { handle_read(cfg, c, ep); }
    }
  }

  close(ep);
  forge_log("[worker %d] exit", widx);
}
