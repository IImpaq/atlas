<h1 align="center">Atlas 🚀</h1>

A blazingly fast, modern package manager for macOS and Linux written in C++

## ⚡️ Features

- Lightning-fast package installation and updates
- Native system integration
- Minimal memory footprint
- Cross-platform compatibility

## 🚀 Installation

```bash
# Note: Installation instructions are subject to change and will break 
git clone https://github.com/IImpaq/atlas.git
cd atlas
make install
```

## 🎯 Quick Start

```bash
# Install a package
atlas install nginx

# Remove a package
atlas remove nginx

# Sync with remote
atlas fetch

# Update all packages
atlas update

# Search for packages
atlas search database
```

## 🔧 Configuration

Atlas will be be configurable via ```~/.config/atlas.toml```:

```toml
[core]
threads = 4
cache_dir = "~/.cache/atlas/"

[network]
timeout = 30
retries = 3
```

## 🏗 Building from Source

Requirements:
- C++20 compatible compiler
- CMake 3.15+
- JsonCPP
- libcurl

```bash
mkdir build && cd build
cmake ..
make -j$(nproc)
```

## 📈 Performance

| Operation | Time |
|-----------|------|
| Install   | x.xs |
| Update    | x.xs |
| Search    | x.xs |

## 🤝 Contributing

We welcome contributions! Please see our [Contributing Guide](CONTRIBUTING.md).

## 📜 License

MIT License - see the [LICENSE](LICENSE) file for details.

## ⚠️ Disclaimer

This project is highly unstable, work in progress and may & will things. Use at your own risk.
 
