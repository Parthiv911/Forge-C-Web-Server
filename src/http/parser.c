#include "forge.h"
#include <strings.h>

bool http_try_parse_request(const char *buf, size_t len, http_req_t *out) {
  const char *end = NULL;
  for (size_t i = 3; i < len; ++i) {
    if (buf[i-3]=='\r' && buf[i-2]=='\n' && buf[i-1]=='\r' && buf[i]=='\n') {
      end = buf + i + 1; break;
    }
  }
  if (!end) return false; // need more bytes

  // request line
  out->method[0]=out->path[0]=out->version[0]='\0';
  if (sscanf(buf, "%7s %1023s %15s", out->method, out->path, out->version) != 3)
    return false;

  bool http11 = (strcmp(out->version, "HTTP/1.1") == 0);
  bool ka = http11;

  // scan headers (simple)
  const char *p = strstr(buf, "\r\n");
  while (p && p < end) {
    p += 2;
    if (p >= end) break;
    if (strncasecmp(p, "Connection:", 11) == 0) {
      const char *v = p + 11;
      while (*v==' '||*v=='\t') v++;
      if (strncasecmp(v, "close", 5) == 0) ka = false;
      if (strncasecmp(v, "keep-alive", 10) == 0) ka = true;
    }
  }
  out->keep_alive = ka;
  out->header_bytes = (size_t)(end - buf);
  return true;
}
