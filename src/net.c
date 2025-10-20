#include "forge.h"

int forge_set_nonblock(int fd) {
  int fl = fcntl(fd, F_GETFL, 0);
  if (fl < 0) return -1;
  return fcntl(fd, F_SETFL, fl | O_NONBLOCK);
}

int forge_create_listener(const char *hostport) {
  char host[128]; strncpy(host, hostport, sizeof(host));
  char *colon = strrchr(host, ':');
  if (!colon) forge_die("bad -l host:port");
  *colon = '\0';
  int port = atoi(colon + 1);

  int fd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
  if (fd < 0) forge_die("socket: %s", strerror(errno));
  int yes = 1;
  setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));
#ifdef SO_REUSEPORT
  setsockopt(fd, SOL_SOCKET, SO_REUSEPORT, &yes, sizeof(yes));
#endif

  struct sockaddr_in addr = {0};
  addr.sin_family = AF_INET;
  addr.sin_port = htons((uint16_t)port);
  if (inet_pton(AF_INET, host, &addr.sin_addr) != 1) forge_die("inet_pton failed");

  if (bind(fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) forge_die("bind: %s", strerror(errno));
  if (listen(fd, 1024) < 0) forge_die("listen: %s", strerror(errno));
  return fd;
}
