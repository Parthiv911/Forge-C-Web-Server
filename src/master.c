#include "forge.h"

extern atomic_bool g_terminate;

static void sig_handler(int sig) {
  if (sig == SIGINT || sig == SIGTERM) {
    atomic_store(&g_terminate, true);
  }
}

void forge_install_signals(void) {
  struct sigaction sa = {.sa_handler = sig_handler};
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = 0;
  sigaction(SIGINT,  &sa, NULL);
  sigaction(SIGTERM, &sa, NULL);
  signal(SIGPIPE, SIG_IGN);
}
void forge_master_run(forge_cfg_t *cfg) {
  int workers = cfg->workers;
  int (*pipes)[2] = calloc((size_t)workers, sizeof(int[2]));
  if (!pipes) forge_die("calloc pipes failed");

  for (int i = 0; i < workers; i++) {
    if (socketpair(AF_UNIX, SOCK_DGRAM, 0, pipes[i]) < 0)
      forge_die("socketpair: %s", strerror(errno));

    forge_set_nonblock(pipes[i][0]);
    forge_set_nonblock(pipes[i][1]);

    pid_t pid = fork();
    if (pid < 0) forge_die("fork: %s", strerror(errno));

    if (pid == 0) {
      // child worker
      close(pipes[i][0]);          // close master side
      close(cfg->listen_fd);       // worker does not accept

      cfg->control_fd = pipes[i][1];
      cfg->listen_fd = -1;

      forge_run_worker(cfg, i);
      _exit(0);
    }

    // master
    close(pipes[i][1]);            // close worker side
  }

  int ep = epoll_create1(EPOLL_CLOEXEC);
  if (ep < 0) forge_die("epoll_create1 master: %s", strerror(errno));

  struct epoll_event ev = {0};
  ev.events = EPOLLIN;
  ev.data.fd = cfg->listen_fd;

  if (epoll_ctl(ep, EPOLL_CTL_ADD, cfg->listen_fd, &ev) < 0)
    forge_die("master epoll_ctl listen_fd: %s", strerror(errno));

  int next = 0;

  while (!atomic_load(&g_terminate)) {
    struct epoll_event events[16];

    int n = epoll_wait(ep, events, 16, 1000);
    if (n < 0) {
      if (errno == EINTR) continue;
      break;
    }

    for (int i = 0; i < n; i++) {
      if (events[i].data.fd != cfg->listen_fd) continue;

      for (;;) {
        struct sockaddr_in cli;
        socklen_t sl = sizeof(cli);

        int cfd = accept4(cfg->listen_fd,
                          (struct sockaddr *)&cli,
                          &sl,
                          SOCK_NONBLOCK);

        if (cfd < 0) {
          if (errno == EAGAIN || errno == EWOULDBLOCK) break;
          if (errno == EINTR) continue;
          break;
        }

        int target = next;
        next = (next + 1) % workers;

        if (forge_send_fd(pipes[target][0], cfd) < 0) {
          close(cfd);
        } else {
          close(cfd); // worker now has its own fd reference
        }
      }
    }

    int status;
    while (waitpid(-1, &status, WNOHANG) > 0) {
      // handle worker death
    }
  }

  kill(0, SIGTERM);

  for (int i = 0; i < workers; i++)
    close(pipes[i][0]);

  close(ep);
  close(cfg->listen_fd);
  free(pipes);
}

// void forge_master_run(forge_cfg_t *cfg) {
//   // Prefork workers
//   for (int i = 0; i < cfg->workers; i++) {
//     pid_t pid = fork();
//     if (pid < 0) forge_die("fork: %s", strerror(errno));
//     if (pid == 0) { // worker
//       forge_run_worker(cfg, i);
//       _exit(0);
//     }
//   }

//   // Master lifecycle: reap, respond to signals
//   int status;
//   while (!atomic_load(&g_terminate)) {
//     pid_t p = waitpid(-1, &status, WNOHANG);
//     if (p <= 0) usleep(200000);
//   }

//   // Signal all workers to stop and close listener
//   kill(0, SIGTERM);
//   // close(cfg->listen_fd);
// }
