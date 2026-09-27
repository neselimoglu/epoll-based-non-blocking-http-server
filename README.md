# Epoll-Based Non-Blocking HTTP Server

> ⚠️ **Note:** This project is currently under active development. Features and architecture are subject to continuous improvements.

This project is a high-performance, lightweight HTTP web server written entirely in C. It was engineered to handle multiple concurrent client connections efficiently by utilizing Linux's `epoll` I/O multiplexing mechanism alongside non-blocking sockets.

## 🚀 Key Features

*   **Asynchronous I/O:** Fully non-blocking socket operations (`O_NONBLOCK`) to prevent process starvation during read/write cycles.
*   **Epoll Event Loop:** Utilizes `epoll` with Edge-Triggered (`EPOLLET`) mode for highly scalable and efficient event notification.
*   **Connection Persistence:** Robust support for HTTP `Keep-Alive` headers, allowing multiple requests over a single TCP connection without tearing down the socket.
*   **State Management:** Modular client state tracking (`ClientsState`), managing individual read/write buffers, parsed headers, and connection states.
*   **Timeout Management:** Built-in garbage collection that actively scans and disconnects idle or zombie clients (50-second timeout mechanism).
*   **Signal Handling:** Ignores `SIGPIPE` to prevent the server from crashing when clients abruptly disconnect during transmission.

## 📂 Project Architecture

*   **`main.c`**: Initializes the server, binds to port 8080, and runs the core `epoll_wait` event loop to dispatch incoming events.
*   **`HTTP.c` / `HTTP.h`**: Responsible for parsing incoming HTTP requests, extracting headers (e.g., `Content-Length`), and formatting standard HTTP responses (e.g., `200 OK`, `404 Not Found`, JSON payloads).
*   **`ClientsState.c` / `ClientsState.h`**: Manages the memory and state array (`clients[]`) for up to 1000 concurrent connections, tracking active file descriptors and timestamps.
*   **`Makefile`**: Automated build script for compiling the server objects.

## 🛠️ Build and Run Instructions

This server requires a Linux environment (or WSL) as it relies on the Linux-specific `epoll` system call.

**1. Compile the Server:**
Navigate to the project directory and run:
```bash
make
```

**2. Start the Server:**
Launch the compiled executable:
```bash
./epoll_server
```
*The server will start silently and listen for incoming TCP connections on `127.0.0.1:8080`.*

**3. Test the Endpoints:**
You can test the server using a web browser or `curl`:
```bash
# Test the main HTML page
curl -i http://localhost:8080/

# Test the JSON API endpoint
curl -i http://localhost:8080/api/users
```
