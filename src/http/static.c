#include "forge.h"
#include "http/static.h"

static const char *guess_type(const char *p) {
  const char *dot = strrchr(p, '.');
  if (!dot) return "application/octet-stream";
  if (!strcmp(dot, ".html")||!strcmp(dot,".htm")) return "text/html";
  if (!strcmp(dot, ".txt"))  return "text/plain";
  if (!strcmp(dot, ".css"))  return "text/css";
  if (!strcmp(dot, ".js"))   return "application/javascript";
  if (!strcmp(dot, ".png"))  return "image/png";
  if (!strcmp(dot, ".jpg") || !strcmp(dot, ".jpeg")) return "image/jpeg";
  if (!strcmp(dot, ".gif"))  return "image/gif";
  if (!strcmp(dot, ".bin"))  return "application/octet-stream";
  return "application/octet-stream";
}

bool http_path_has_dotdot(const char *p) { return strstr(p, "..") != NULL; }

void http_send_404(int fd, bool keep_alive) {
  char hdr[HDR_BUF];
  int n = snprintf(hdr, sizeof(hdr),
    "HTTP/1.1 404 Not Found\r\n"
    "Content-Length: 13\r\n"
    "Content-Type: text/plain\r\n"
    "Connection: %s\r\n"
    "\r\n"
    "404 Not Found", keep_alive ? "keep-alive" : "close");
  (void)write(fd, hdr, (size_t)n);
}

void http_send_400(int fd) {
  const char *h =
    "HTTP/1.1 400 Bad Request\r\n"
    "Content-Length: 11\r\n"
    "Content-Type: text/plain\r\n"
    "Connection: close\r\n"
    "\r\n"
    "bad request";
  (void)write(fd, h, strlen(h));
}

int http_send_static_file(int fd, const char *doc_root, const char *url_path, bool keep_alive) {
  if (http_path_has_dotdot(url_path)) { http_send_404(fd, keep_alive); return -1; }

  char fs[2048];
  if (strcmp(url_path, "/") == 0) {
    snprintf(fs, sizeof(fs), "%s/index.html", doc_root);
  } else {
    snprintf(fs, sizeof(fs), "%s/%s", doc_root, (url_path[0]=='/') ? url_path+1 : url_path);
  }

  int f = open(fs, O_RDONLY);
  if (f < 0) { http_send_404(fd, keep_alive); return -1; }

  struct stat st;
  if (fstat(f, &st) < 0 || !S_ISREG(st.st_mode)) {
    close(f); http_send_404(fd, keep_alive); return -1;
  }

  const char *ctype = guess_type(fs);
  char hdr[HDR_BUF];
  int hn = snprintf(hdr, sizeof(hdr),
      "HTTP/1.1 200 OK\r\n"
      "Content-Length: %lld\r\n"
      "Content-Type: %s\r\n"
      "Connection: %s\r\n"
      "Last-Modified: Thu, 01 Jan 1970 00:00:00 GMT\r\n"
      "\r\n",
      (long long)st.st_size, ctype, keep_alive ? "keep-alive" : "close");
  if (write(fd, hdr, (size_t)hn) < hn) { close(f); return -1; }

  off_t off = 0;
  ssize_t remain = st.st_size;
  while (remain > 0) {
    ssize_t n = sendfile(fd, f, &off, (size_t)remain);
    if (n < 0) {
      if (errno == EAGAIN || errno == EWOULDBLOCK) continue;
      close(f); return -1;
    }
    remain -= n;
  }
  close(f);
  return 0;
}
