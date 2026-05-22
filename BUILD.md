# XMALab Build Instructions

XMALab uses a modern CMake Presets setup to ensure consistent and reproducible builds across all supported platforms. 
This process automatically fetches and builds necessary dependencies (using `vcpkg`), configures the build tree, and compiles the software.

## Prerequisites
- **Git**
- **CMake** (v3.25 or newer)
- **vcpkg** (Must be installed and the `VCPKG_ROOT` environment variable must be set to its installation path)
- **C++17 Compiler** (MSVC for Windows, Clang for macOS, GCC for Linux)

---

## Standard Build Process

### 1. Clone the Repository
Always start with a clean clone of the repository to ensure no leftover artifacts interfere with the build.
```bash
git clone https://github.com/XROMMPackages/xmalab.git
cd xmalab
```

### 2. Configure the Build Environment
Run the CMake configuration step using the preset that matches your operating system. This step will resolve dependencies and generate the build files in `build/<preset-name>`.

**Windows:**
```bash
cmake --preset windows
```
**macOS (Apple Silicon / ARM64):**
```bash
cmake --preset macos
```
**macOS (Intel / x86_64):**
```bash
cmake --preset macos-intel
```
**Linux:**
```bash
cmake --preset linux
```

### 3. Compile the Application
Once configuration is complete, compile the release version of the application using the corresponding build preset.

**Windows:**
```bash
cmake --build --preset windows-release
```
**macOS (Apple Silicon / ARM64):**
```bash
cmake --build --preset macos-release
```
**macOS (Intel / x86_64):**
```bash
cmake --build --preset macos-intel-release
```
**Linux:**
```bash
cmake --build --preset linux-release
```

---

## Troubleshooting & Cleaning
If you encounter build errors, it is highly recommended to wipe the `build/` directory completely and start over.
You can use `git clean` to completely reset the repository to a freshly-cloned state (Warning: This deletes ALL untracked files).
```bash
git clean -fdx
```
