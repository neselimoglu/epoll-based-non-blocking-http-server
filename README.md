# Epoll-Based Non-Blocking Redis Clone

> ⚠️ **Note:** This project evolved from a simple HTTP server into a high-performance, in-memory Key-Value store (Redis clone). It is written entirely in C from scratch, emphasizing robust system architecture, manual memory management, and high concurrency.

This project is a high-performance, lightweight database engine. It is engineered to handle thousands of concurrent client connections efficiently by utilizing Linux's `epoll` I/O multiplexing mechanism alongside non-blocking sockets. It supports the **RESP (REdis Serialization Protocol)**, meaning standard tools like `redis-cli` and `redis-benchmark` can connect to it natively.

## 🚀 Key Features

- **Asynchronous I/O & Epoll Event Loop:** Uses `epoll` in Edge-Triggered (`EPOLLET`) mode with non-blocking sockets (`O_NONBLOCK`) to prevent process starvation and maximize RPS (Requests Per Second). Partial reads and partial writes are handled safely.
- **RESP & Telnet Support:** Compatible with the official Redis Serialization Protocol for integration with existing Redis tools, while keeping fallback support for raw inline Telnet commands.
- **Custom O(1) Hash Table:** Built-from-scratch storage using the `djb2` hashing algorithm, with linked lists for collision resolution. Designed to be memory-leak free.
- **TTL & Expiration Engine:** Supports key expiration (`EXPIRE`) with a dual-purge mechanism: passive expiration (on read) and an active background sweep running every second.
- **Binary Persistence (RDB):** Takes point-in-time snapshots of the database (`SAVE`). Data is written to disk in a compact binary format (`dump.rdb`) and automatically loaded into RAM on server startup.
- **Atomic Counters:** Built-in string-to-integer conversion and atomic math operations (`INCR`, `DECR`).
- **Zombie Connection Sweeping:** Built-in cleanup that scans for and disconnects idle clients (50-second timeout), protecting against Slowloris-style attacks.

## 📂 Project Architecture

| File | Responsibility |
|------|----------------|
| `server.c` | Core networking engine. Initializes the TCP socket, binds to port 8080, runs the main `epoll_wait` event loop to dispatch I/O events and trigger background sweeps. |
| `HashTable.c` / `HashTable.h` | The database core. Memory allocation (`malloc`/`free`), CRUD operations, TTL checks, and binary disk I/O (`fread`/`fwrite`). |
| `CommandParser.c` / `CommandParser.h` | Protocol router. Parses incoming RESP arrays and raw strings, maps them to hash table functions, and formats standard RESP responses (e.g. `+OK\r\n`). |
| `ClientsState.c` / `ClientsState.h` | Manages the `clients[]` state array for concurrent connections: active file descriptors, partial read/write buffers, and timestamps. |
| `Makefile` | Build script that compiles the server with the `-O3` optimization flag. |

## ⌨️ Supported Commands

| Command | Description |
|---------|-------------|
| `SET <key> <value>` | Store a key-value pair. |
| `GET <key>` | Retrieve the value of a key. |
| `DEL <key>` | Delete a key and free its memory. |
| `INCR <key>` / `DECR <key>` | Atomically increment or decrement an integer value. |
| `EXPIRE <key> <seconds>` | Set a time-to-live for a key. |
| `SAVE` | Save the current database state to disk (`dump.rdb`). |
| `PING` | Returns `PONG`. |

## 🛠️ Build and Run

This server requires a **Linux** environment, as it relies on the Linux-specific `epoll` system call.

### 1. Compile

Navigate to the project directory and run:

```bash
make
```

### 2. Start the server

```bash
./redis-clone
```

The server starts silently and listens for TCP connections on `127.0.0.1:8080`.

### 3. Connect and test

**Using Redis CLI (recommended):**

```bash
redis-cli -p 8080
```

```text
127.0.0.1:8080> PING
PONG
127.0.0.1:8080> SET username "Nail Eren"
OK
127.0.0.1:8080> GET username
"Nail Eren"
```

**Using Telnet:**

```bash
telnet localhost 8080
```

```text
Trying 127.0.0.1...
Connected to localhost.
SET language C
+OK
GET language
$1
C
```
