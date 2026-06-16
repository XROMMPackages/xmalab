# macOS Build and Deployment Instructions

This guide covers building, compiling, and packaging XMALab on macOS (supporting both Apple Silicon / ARM64 and Intel / x86_64 architectures) using local precompiled libraries.

---

## 1. Prerequisites

Before starting, install the following tools:
- **Xcode Command Line Tools**: Install using terminal:
  ```bash
  xcode-select --install
  ```
- **CMake**: Install CMake (v3.25 or newer) via [cmake.org](https://cmake.org/download/) or using Homebrew:
  ```bash
  brew install cmake
  ```
- **Homebrew**: Install from [brew.sh](https://brew.sh/) to manage system libraries.
- **Node.js & appdmg** (for DMG creation):
  ```bash
  brew install node
  npm install -g appdmg
  ```

---

## 2. Local Dependencies Setup

For macOS builds, dependencies are precompiled and placed in standard user folders. CMake has been updated to scan these directories automatically.

Ensure your machine's precompiled libraries are structured in the following directories (paths starting with `~/` refer to your user home `/Users/<username>/`):

1. **GLEW** (Static library): Installed via Homebrew:
   ```bash
   brew install glew
   ```
2. **Qt6**: Installed via Qt Online Installer to:
   `~/Qt/6.9.3/macos` (or your specific version folder)
3. **OpenCV**: Compiled from source and located in:
   `~/Documents/opencvbuild`
4. **QuaZip**: Built from source and located in:
   `~/Documents/quazip-1.5` (the library file `libquazip1-qt6.dylib` must reside under the build tree in `~/Documents/quazip-1.5/build/quazip/`)
5. **Levmar**: Compiled from source in:
   `~/Documents/levmar-2.6` (creates the static library `liblevmar.a` directly at the root of the folder)

---

## 3. Clone and Build XMALab

1. **Clone the Repository**:
   ```bash
   git clone https://github.com/XROMMPackages/xmalab.git
   cd xmalab
   ```

2. **Configure the Build via CMake**:
   Our updated CMake configuration automatically scans `~/Documents`, `~/Qt`, and Homebrew paths, resolving all dependencies. Run the configure step in a new build folder (e.g., `build`):
   ```bash
   cmake -B build -S .
   ```
   *(Note: No manual variable adjustments in cmake-gui are required. The Accelerate framework is automatically detected and linked for Levmar).*

3. **Compile the Application**:
   Compile the release bundle using parallel cores:
   ```bash
   cmake --build build --config Release -j$(sysctl -n hw.ncpu)
   ```
   - This compiles the app bundle to `build/bin/XMALab.app`.
   - **Automatic Deployment and Code Signing**: The build process automatically invokes `macdeployqt` to package dependencies, followed by a deep ad-hoc code signature:
     ```bash
     codesign --force --deep --sign - build/bin/XMALab.app
     ```
     This prevents the application from immediately crashing on launch (specifically resolving signature invalidation/exc_bad_access errors on Apple Silicon).

---

## 4. Packaging the macOS Installer (.dmg)

We use `appdmg` to package the app bundle into a user-friendly installer image:

1. Edit the template JSON to set the correct version number:
   ```bash
   cd deployment/MAC
   cp xmalab_template.json xmalab.json
   # Replace VERSION with your release version (e.g. 3.0.0)
   sed -i '' 's/VERSION/3.0.0/g' xmalab.json
   ```

2. Run `appdmg` to generate the package:
   ```bash
   appdmg xmalab.json ../../build/bin/XMALab_3.0.0_macOS.dmg
   ```
   The final distributable `.dmg` package will be saved in `build/bin/`.
