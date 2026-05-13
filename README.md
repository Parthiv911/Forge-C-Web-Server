# Forge-C Web Server

Forge is a minimal **NGINX-style** HTTP/1.1 static web server in **C**. It handles concurrent keep-alive connections using **prefork** worker processes, an **epoll**-based event loop, and zero-copy file streaming via **`sendfile()`**.

## Architecture and Features
- **Master + prefork workers:** a master process forks multiple workers.
- **I/O model:** each worker runs a non-blocking **epoll** event loop and registers non-blocking client sockets.
- **Serving static files:** on requests, files are opened, verified, and zero-copy streamed with **`sendfile()`** (page cache → socket).
- **Graceful shutdown:** on **SIGINT/SIGTERM**, a signal handler flips an atomic flag; workers stop accepting new connections, drain in-flight requests, and exit; the master reaps workers and closes the listener.
- **SIGPIPE-safe writes**: the server ignores SIGPIPE so failed sends return EPIPE instead of killing the worker; we detect it and close the connection.

## Benchmarking
Forge is benchmarked with `wrk`, plotting **RPS** and **latency** against response payload file size in a **closed-loop** setup (constant number of concurrent connections) for 15s each with 4.

| file     | size_bytes | rps      | latency_avg_ms | latency_p99_ms |
|----------|------------|----------|----------------|----------------|
| 1k.bin   | 1,024      | 6,204.01 | 40.98          | 42.30          |
| 2k.bin   | 2,048      | 6,207.79 | 40.98          | 42.34          |
| 4k.bin   | 4,096      | 6,208.92 | 40.95          | 42.23          |
| 8k.bin   | 8,192      | 6,203.08 | 40.99          | 42.32          |
| 16k.bin  | 16,384     | 6,204.24 | 40.97          | 42.32          |
| 32k.bin  | 32,768     | 6,196.89 | 41.05          | 42.50          |
| 64k.bin  | 65,536     | 53,046.07| 2.63           | 5.38           |
| 128k.bin | 131,072    | 38,295.71| 3.64           | 7.06           |
| 256k.bin | 262,144    | 22,882.66| 5.80           | 11.21          |
| 512k.bin | 524,288    | 12,757.29| 10.40          | 20.15          |
| 1m.bin   | 1,048,576  | 6,587.37 | 20.28          | 39.32          |


## Quickstart
```bash
# Build
make

# Create a simple page to serve
mkdir -p public
printf '<h1>forge up</h1>\n' > public/index.html

# Run (4 workers, port 8080, serve ./public)
./forge -w 4 -l 0.0.0.0:8080 -r ./public

# In another terminal, test
curl -i http://127.0.0.1:8080/
```
