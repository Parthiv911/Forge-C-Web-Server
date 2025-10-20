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
  // Prefork workers
  for (int i = 0; i < cfg->workers; i++) {
    pid_t pid = fork();
    if (pid < 0) forge_die("fork: %s", strerror(errno));
    if (pid == 0) { // worker
      forge_run_worker(cfg, i);
      _exit(0);
    }
  }

  // Master lifecycle: reap, respond to signals
  int status;
  while (!atomic_load(&g_terminate)) {
    pid_t p = waitpid(-1, &status, WNOHANG);
    if (p <= 0) usleep(200000);
  }

  // Signal all workers to stop and close listener
  kill(0, SIGTERM);
  close(cfg->listen_fd);
}
