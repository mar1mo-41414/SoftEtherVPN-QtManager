# SoftEtherVPN-QtManager

[日本語 README →](README.md)

SoftEther VPN's official management GUI, "VPN Server Manager," is Windows-only. This
project reimplements an equivalent management GUI for Mac / Linux from scratch in
Qt (C++), talking to SoftEther VPN Server over its public JSON-RPC API.

## Status

🚧 Under development (Phase 0: project scaffolding complete). Not yet usable.
See [docs/ROADMAP.md](docs/ROADMAP.md) for progress and [HISTORY.md](HISTORY.md) for
the background and design decisions.

## Intended usage

Connect to any SoftEther VPN Server (Linux/Mac/Windows, as long as the JSON-RPC API is
enabled) by host, port, and admin password, and manage Virtual Hubs, users, groups,
sessions, etc. with a workflow similar to the Windows VPN Server Manager.

## Build

```bash
brew install qt cmake   # if not already installed
cmake -S . -B build
cmake --build build
```

(A usable application is expected from Phase 2 onward.)

## Requirements

- Qt 6
- CMake 3.20+
- A C++20-capable compiler (macOS: Xcode Command Line Tools / Linux: gcc or clang)
