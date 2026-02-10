# librtc

A modern C++20 wrapper around WebRTC, designed to provide a clean, asynchronous API using coroutines and standard C++ idioms.

## Features

- **Standard C++20**: Built with modern C++ standards in mind.
- **Coroutines**: First-class support for C++20 coroutines for asynchronous operations.
- **Clean API**: Hides the complexity of the native WebRTC C++ API.
- **Error Handling**: Uses a custom `expected`-like result type for robust error handling (as `std::expected` is not supported in Clang 21).
- **Events**: Uses a type-safe event system instead of third-party signal/slot libraries.

## Prerequisites

To build `librtc`, you need:

- **C++ Compiler**: A compiler supporting C++20 (Clang 21).
- **CMake**: Version 3.20 or later.
- **WebRTC**: A pre-compiled WebRTC library (libwebrtc).
- **Boost**: Specifically `Boost.System` and headers.
- **Ninja** (optional): Recommended build generator.
- **Tested Platform**: Ubuntu 24.04.

### Installing Clang 21
To install Clang 21 and LLVM 21 on Ubuntu 24.04 (Noble), use the official LLVM APT repository:

1. **Add the LLVM GPG key:**
   ```bash
   wget -qO- https://apt.llvm.org/llvm-snapshot.gpg.key | sudo tee /etc/apt/keyrings/llvm.asc
   ```

2. **Add the LLVM repository:**
   ```bash
   echo "deb [signed-by=/etc/apt/keyrings/llvm.asc] http://apt.llvm.org/noble/ llvm-toolchain-noble-21 main" | sudo tee /etc/apt/sources.list.d/llvm.list
   ```

3. **Update package list and install:**
   ```bash
   sudo apt update
   sudo apt install clang-21 lldb-21 lld-21 clangd-21
   ```

4. **Verify Installation:**
   ```bash
   clang-21 --version
   ```

## Building

### 1. Build WebRTC Package
Before building `librtc`, you must build and export the WebRTC Conan package:

```bash
git clone https://github.com/ChebotarDmytro/webrtc_conan
cd webrtc_conan
# Follow the instructions in webrtc_conan/README.md to build and create the conan package
```

### 2. Clone the repository
```bash
git clone https://github.com/ChebotarDmytro/librtc.git
cd librtc
```

### 3. Install Dependencies
```bash
conan install . --build=missing
```

### 4. Build using CMake Presets
We use CMake Presets to simplify the build process.

**Release with Debug Info (Recommended):**
```bash
cmake --preset dev-rel
cmake --build --preset dev-rel
```

**Debug Build:**
```bash
cmake --preset dev-debug
cmake --build --preset dev-debug
```

## Running Examples

After building, you can find the example executable in the build directory.

```bash
./build/hello_world
```

The `hello_world` example demonstrates two peers (Alice and Bob) connecting to each other locally, exchanging ICE candidates, and sending a message over a DataChannel.

## Project Structure

```
librtc/
├── include/
│   └── librtc/           # Public headers
│       ├── errors/       # Error definitions
│       ├── utils/        # Utilities (AsyncBridge, Event, Expected)
│       └── ...
├── src/                  # implementation details
├── examples/             # Example usages (hello_world)
├── CMakeLists.txt        # Build configuration
└── README.md             # This file
```

## Basic Usage

Detailed examples can be found in the `examples/` directory.

The [hello_world_test.cpp](examples/hello_world_test.cpp) example demonstrates:
- Creating PeerConnections
- Setting up DataChannels
- Exchange of ICE candidates and SDP
- Sending and receiving messages

## License

MIT
