# XMALab Build Instructions

XMALab uses a modern CMake Presets setup to ensure consistent and reproducible builds across all supported platforms. 

## Prerequisites
- **Git**
- **CMake** (v3.25 or newer)
- **C++17 Compiler** (MSVC for Windows, Clang for macOS, GCC for Linux)

### Dependencies
XMALab requires the following libraries:
- **Qt5 or Qt6** (Core, Gui, Widgets, OpenGL)
- **OpenCV**
- **GLEW**
- **QuaZip**
- **Levmar**
- **zlib**

**For Windows Users:**
The provided `CMakePresets.json` relies on **vcpkg** to automatically resolve dependencies on Windows. You must have vcpkg installed and the `VCPKG_ROOT` environment variable set to its installation path.

**For macOS Users:**
The macOS build utilizes local dependency directories instead of vcpkg. Our CMake configuration automatically resolves paths for Homebrew (`/opt/homebrew`), Qt installs (`~/Qt`), and source builds located in `~/Documents` (e.g., `~/Documents/opencvbuild`, `~/Documents/quazip-1.5`, `~/Documents/levmar-2.6`). Ensure your local dependencies are placed in these standard paths.

**For Linux Users:**
Dependencies are typically installed via your system's package manager (e.g., `apt`) or built directly from source. The CMake Presets provided for Linux assume you have installed these libraries globally or configured CMake to find them.

---

## Standard Build Process

### 1. Clone the Repository
Always start with a clean clone of the repository.
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
**Linux (vcpkg):**
```bash
cmake --preset linux
```
**Linux (system packages):**
```bash
cmake --preset linux-system
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
**Linux (vcpkg):**
```bash
cmake --build --preset linux-release
```
**Linux (system packages):**
```bash
cmake --build --preset linux-system-release
```

*Note on Automatic Deployment:* 
- **Windows**: The build process automatically runs `windeployqt` as a post-build step.
- **macOS**: The build process automatically invokes `macdeployqt` to package dependencies, followed by a deep ad-hoc code signature (`codesign --force --deep --sign -`) to prevent crashes on launch.

---

## 4. Packaging the Installers

### Windows Installer (.exe)
1. Open **NSIS** on your computer.
2. Select **Compile NSI scripts**.
3. Load the script located at `deployment/Windows_NSIS/XMALabInstaller.nsi`.
4. Compile the script. This will bundle the compiled binary, assets, and dependencies into a standalone installer (`XMALab_Setup.exe`).

### macOS Installer (.dmg)
We use `appdmg` to package the app bundle into a user-friendly installer image:
1. Ensure Node.js and `appdmg` are installed (`brew install node && npm install -g appdmg`).
2. Edit the template JSON to set the correct version number:
   ```bash
   cd deployment/MAC
   cp xmalab_template.json xmalab.json
   sed -i '' 's/VERSION/3.0.0/g' xmalab.json
   ```
2. Run `appdmg` to generate the package. You must copy the compiled `.app` to the `deployment/MAC` folder first:
   ```bash
   cp -R ../../build/macos/bin/XMALab.app .
   appdmg xmalab.json ../../build/macos/bin/XMALab_3.0.0_macOS.dmg
   ```

### Linux AppImage (.AppImage)

The project uses a **Fedora 40 Distrobox** container to build the AppImage. This ensures a consistent glibc baseline (2.38) for maximum compatibility across Linux distributions.

#### 1. Create & Enter the Distrobox

```bash
distrobox create --name xmalab-fedora --image fedora:40
distrobox enter xmalab-fedora
```

#### 2. Install Build Dependencies

```bash
sudo dnf install -y \
  cmake gcc gcc-c++ ninja-build git make \
  qt6-qtbase-devel qt6-qttools-devel qt6-qt5compat-devel \
  opencv-devel glew-devel mesa-libGLU-devel levmar-devel \
  quazip-qt6-devel squashfs-tools xorriso file desktop-file-utils
```

#### 3. Configure & Build

```bash
cmake --preset linux-system
cmake --build --preset linux-system-release
```

Output: `build/linux-system/bin/Release/XMALab`

#### 4. Download AppImage Packaging Tools

```bash
# linuxdeploy
wget https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-x86_64.AppImage \
  -O /tmp/linuxdeploy-x86_64.AppImage
chmod +x /tmp/linuxdeploy-x86_64.AppImage
/tmp/linuxdeploy-x86_64.AppImage --appimage-extract
mv /tmp/squashfs-root /tmp/linuxdeploy-extracted

# linuxdeploy Qt plugin
wget https://github.com/linuxdeploy/linuxdeploy-plugin-qt/releases/download/continuous/linuxdeploy-plugin-qt-x86_64.AppImage \
  -O /tmp/linuxdeploy-plugin-qt-x86_64.AppImage
chmod +x /tmp/linuxdeploy-plugin-qt-x86_64.AppImage
/tmp/linuxdeploy-plugin-qt-x86_64.AppImage --appimage-extract
cp /tmp/squashfs-root/usr/bin/linuxdeploy-plugin-qt /tmp/linuxdeploy-extracted/plugins/
rm -rf /tmp/squashfs-root

# appimagetool
wget https://github.com/AppImage/AppImageKit/releases/download/continuous/appimagetool-x86_64.AppImage \
  -O /tmp/appimagetool-x86_64.AppImage
chmod +x /tmp/appimagetool-x86_64.AppImage
/tmp/appimagetool-x86_64.AppImage --appimage-extract
mv /tmp/squashfs-root /tmp/appimagetool-extracted
```

#### 5. Build the AppImage

Run the automated packaging script:

```bash
./deployment/Linux_AppImage/create-appimage.sh
```

This produces `XMALab-x86_64.AppImage` in the project root.

#### 6. FUSE3 Runtime (for modern distros without FUSE2)

The AppImage's embedded runtime requires `libfuse.so.2` (FUSE2). On modern distributions (Fedora 41+, Bluefin, etc.) that ship only FUSE3, you must rebuild the AppImage with a FUSE3-compatible runtime.

Inside the `xmalab-fedora` distrobox:

```bash
# Build libfuse3 statically from source
cd /tmp
wget https://github.com/libfuse/libfuse/releases/download/fuse-3.15.0/fuse-3.15.0.tar.xz
tar xf fuse-3.15.0.tar.xz
cd fuse-3.15.0
patch -p1 < /tmp/type2-runtime/patches/libfuse/mount.c.diff
mkdir build && cd build
meson setup --prefix=/usr/local ..
meson configure --default-library static
sudo ninja install

# Build squashfuse statically against libfuse3
cd /tmp
wget https://github.com/vasi/squashfuse/archive/0.5.2.tar.gz -O squashfuse-0.5.2.tar.gz
tar xf squashfuse-0.5.2.tar.gz
cd squashfuse-0.5.2
export CFLAGS="-ffunction-sections -fdata-sections -Os -DFUSE_USE_VERSION=30"
export LDFLAGS="-static"
./autogen.sh
./configure --prefix=/usr/local
make -j$(nproc)
sudo make install
sudo /usr/bin/install -c -m 644 ./*.h /usr/local/include/squashfuse

# Build the runtime
cd /tmp/type2-runtime/src/runtime
cat > Makefile.static << "EOF"
GIT_COMMIT := $(shell cat version)
CC = clang
CFLAGS = -std=gnu99 -Os -D_FILE_OFFSET_BITS=64 -DGIT_COMMIT=\"$(GIT_COMMIT)\" -T data_sections.ld -ffunction-sections -fdata-sections -Wl,--gc-sections -static -Wall -no-pie -fno-PIE
LIBS = -lsquashfuse -lsquashfuse_ll -lzstd -lz -lfuse3
all: runtime
runtime: runtime.c
	$(CC) -I/usr/local/include/squashfuse -I/usr/local/include/fuse3 $(CFLAGS) $^ $(LIBS) -o $@
EOF
make -f Makefile.static

# Strip and add AppImage magic bytes
objcopy --only-keep-debug runtime runtime.debug
strip --strip-debug --strip-unneeded runtime
echo -ne 'AI\x02' | dd of=runtime bs=1 count=3 seek=8 conv=notrunc
```

Then rebuild the AppImage using the new runtime:

```bash
export ARCH=x86_64
rm -f XMALab.AppDir && ./deployment/Linux_AppImage/create-appimage.sh

# Rebuild with FUSE3 runtime
/tmp/appimagetool-extracted/usr/bin/appimagetool \
    --runtime-file /tmp/type2-runtime/src/runtime/runtime \
    XMALab.AppDir \
    XMALab-x86_64.AppImage
```

---

## Troubleshooting & Cleaning
If you encounter build errors, it is highly recommended to wipe the `build/` directory completely and start over.
You can use `git clean` to completely reset the repository to a freshly-cloned state (Warning: This deletes ALL untracked files).
```bash
git clean -fdx
```
