# SoftEtherVPN-QtManager

[日本語 README →](README.md)

SoftEther VPN's official management GUI, "VPN Server Manager," is Windows-only. This
project reimplements an equivalent management GUI for Mac / Linux from scratch in
Qt (C++), talking to SoftEther VPN Server over its public JSON-RPC API. Wording and
screen layout follow the official GUI as closely as possible.

## Status

🚧 Under development. The main screens work, but it's not yet production-ready.
See [docs/ROADMAP.md](docs/ROADMAP.md) for progress,
[docs/ARCHITECTURE.md](docs/ARCHITECTURE.md) for the architecture and implementation
notes, and [HISTORY.md](HISTORY.md) for the background and design decisions.

## What it can do

Connect to any SoftEther VPN Server (Linux/Mac/Windows, as long as the JSON-RPC API is
enabled) by host, port, and admin password, and manage it with a workflow similar to
the Windows VPN Server Manager:

- Connection settings, server-wide administration (listeners, encryption settings,
  clustering, DDNS, VPN Azure, IPsec/L2TP, EtherIP/L2TPv3, OpenVPN/SSTP, etc.)
- Virtual Hub administration (users, groups, security policies, access lists, cascade
  connections, SecureNAT, local bridges, Layer 3 switches, etc.)
- Session / MAC / IP table views, log file download, etc.

## Build

```bash
brew install qt cmake   # if not already installed (use your distro's package manager on Linux)
cmake -S . -B build
cmake --build build
```

The built app is `build/SoftEtherVPN-QtManager` (a `.app` bundle on macOS).

For building distributable packages (macOS `.app`/zip, Linux `.deb`/AppImage), see
[docs/PACKAGING.md](docs/PACKAGING.md).

To preview the screens without a real server, use the bundled mock server
([tools/README.md](tools/README.md)).

## Requirements

- Qt 6
- CMake 3.20+
- A C++20-capable compiler (macOS: Xcode Command Line Tools / Linux: gcc or clang)

## License

[Apache License 2.0](LICENSE). This project is an independent reimplementation,
unaffiliated with and not endorsed by the SoftEther VPN project. Material under
`docs/upstream-reference/` is derived from the SoftEther VPN project (Apache License
2.0); see [NOTICE](NOTICE) for details.
