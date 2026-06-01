#include "forge.h"

int forge_set_nonblock(int fd) {
  int fl = fcntl(fd, F_GETFL, 0);
  if (fl < 0) return -1;
  return fcntl(fd, F_SETFL, fl | O_NONBLOCK);
}

int forge_create_listener(const char *hostport) {
  char host[128];
  snprintf(host, sizeof(host), "%s", hostport);
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


// int forge_set_nonblock(int fd) {
//   int flags = fcntl(fd, F_GETFL, 0);
//   if (flags < 0) return -1;
//   return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
// }

int forge_send_fd(int sock, int fd_to_send) {
  struct msghdr msg = {0};
  char buf[CMSG_SPACE(sizeof(int))];
  memset(buf, 0, sizeof(buf));

  char dummy = 'F';
  struct iovec io = {
    .iov_base = &dummy,
    .iov_len = sizeof(dummy)
  };

  msg.msg_iov = &io;
  msg.msg_iovlen = 1;
  msg.msg_control = buf;
  msg.msg_controllen = sizeof(buf);

  struct cmsghdr *cmsg = CMSG_FIRSTHDR(&msg);
  cmsg->cmsg_level = SOL_SOCKET;
  cmsg->cmsg_type = SCM_RIGHTS;
  cmsg->cmsg_len = CMSG_LEN(sizeof(int));

  memcpy(CMSG_DATA(cmsg), &fd_to_send, sizeof(int));
  msg.msg_controllen = cmsg->cmsg_len;

  ssize_t n = sendmsg(sock, &msg, 0);
  if (n < 0) return -1;
  return 0;
}

int forge_recv_fd(int sock) {
  struct msghdr msg = {0};
  char m_buffer[1];
  struct iovec io = {
    .iov_base = m_buffer,
    .iov_len = sizeof(m_buffer)
  };

  char c_buffer[CMSG_SPACE(sizeof(int))];
  memset(c_buffer, 0, sizeof(c_buffer));

  msg.msg_iov = &io;
  msg.msg_iovlen = 1;
  msg.msg_control = c_buffer;
  msg.msg_controllen = sizeof(c_buffer);

  ssize_t n = recvmsg(sock, &msg, 0);
  if (n <= 0) return -1;

  struct cmsghdr *cmsg = CMSG_FIRSTHDR(&msg);
  if (!cmsg) return -1;

  if (cmsg->cmsg_level != SOL_SOCKET || cmsg->cmsg_type != SCM_RIGHTS)
    return -1;

  int fd;
  memcpy(&fd, CMSG_DATA(cmsg), sizeof(int));
  return fd;
}