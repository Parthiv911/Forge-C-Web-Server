#include "forge.h"

int main(int argc, char **argv) {
  forge_install_signals();

  forge_cfg_t cfg = {
    .listen_hostport = "0.0.0.0:8080",
    .doc_root        = "./public",
    .workers         = 4,
    .listen_fd       = -1,
  };

  int opt;
  while ((opt = getopt(argc, argv, "w:l:r:")) != -1) {
    switch (opt) {
      case 'w': cfg.workers = atoi(optarg); break;
      case 'l': cfg.listen_hostport = optarg; break;
      case 'r': cfg.doc_root = optarg; break;
      default:
        fprintf(stderr, "Usage: %s -w <workers> -l <host:port> -r <docroot>\n", argv[0]);
        return 1;
    }
  }
  cfg.listen_fd = forge_create_listener(cfg.listen_hostport);
  cfg.control_fd = -1;
  forge_master_run(&cfg);
  return 0;
}
