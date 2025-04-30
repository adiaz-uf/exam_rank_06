# 🗨️ Simple TCP Chat Server in C

This project is a multi-client chat server written in C that uses TCP sockets and the `select()` system call to handle multiple connections without using threads.

## 📋 Description

The server listens for incoming connections on `127.0.0.1` and the port provided as a command-line argument. 
Each connected client is assigned a unique ID and can send messages, which are broadcast to all other connected clients. 
Messages are processed line by line (using `\n` as a delimiter).

### ✅ Features

- Supports multiple simultaneous clients using `select()`.
- Messages are broadcast to all clients except the sender.
- Notifies other clients when someone connects or disconnects.
- Dynamically manages per-client input buffers.
- Robust error handling and resource cleanup.

---

## 🛠️ Compilation
```bash
gcc -Wall -Wextra -Werror -o chat_server chat_server.c
```

## 🚀 Running the Server
```bash 
./chat_server <port>
 ```
💬 Connecting to the Server
You can connect using nc (netcat) or telnet:
```bash 
nc 127.0.0.1 8080
 ```
