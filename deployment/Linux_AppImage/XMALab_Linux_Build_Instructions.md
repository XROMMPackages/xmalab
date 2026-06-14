# XMALab Linux Build & AppImage Instructions

**⚠️ glibc requirement**: Built on Fedora 40 (glibc 2.40). The AppImage runs on **Fedora 39+, Ubuntu 24.04+, Debian 13+, RHEL 10+ and equivalent**. Older distros will crash with a glibc version error. For older targets, build on Ubuntu 22.04 (glibc 2.35) instead.

This guide covers building XMALab for Linux using a **Fedora 40 Distrobox** container, and packaging it as a portable **AppImage**.

---

## Prerequisites

- **Podman** or **Docker**
- **Distrobox** (recommended) or direct Podman access
- **CMake** v3.25+ (installed inside container)
- **C++17 compiler** (GCC 14+ inside Fedora 40)

---

## 1. Create & Enter the Distrobox

```bash
distrobox create --name xmalab-fedora --image fedora:40
distrobox enter xmalab-fedora
```

## 2. Install Build Dependencies

Inside the distrobox, run:

```bash
sudo dnf install -y \
  cmake gcc gcc-c++ ninja-build git make \
  qt6-qtbase-devel qt6-qttools-devel qt6-qt5compat-devel \
  opencv-devel glew-devel mesa-libGLU-devel levmar-devel \
  quazip-qt6-devel squashfs-tools xorriso file desktop-file-utils
```

## 3. Build XMALab

```bash
cd /path/to/xmalab
cmake --preset linux-system
cmake --build --preset linux-system-release
```

The binary will be at: `build/linux-system/bin/Release/XMALab`

## 4. Verify the Binary

```bash
# Check for missing shared libraries
ldd build/linux-system/bin/Release/XMALab | grep "not found"

# Quick smoke test (offscreen mode - will print "Missing GL version" in headless env)
QT_QPA_PLATFORM=offscreen ./build/linux-system/bin/Release/XMALab
```

## 5. Create AppImage

### 5a. Download tools (inside distrobox, one-time setup)

```bash
# Download and extract linuxdeploy
wget https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-x86_64.AppImage \
  -O /tmp/linuxdeploy-x86_64.AppImage
chmod +x /tmp/linuxdeploy-x86_64.AppImage
/tmp/linuxdeploy-x86_64.AppImage --appimage-extract
mv /tmp/squashfs-root /tmp/linuxdeploy-extracted

# Download and extract linuxdeploy Qt plugin
wget https://github.com/linuxdeploy/linuxdeploy-plugin-qt/releases/download/continuous/linuxdeploy-plugin-qt-x86_64.AppImage \
  -O /tmp/linuxdeploy-plugin-qt-x86_64.AppImage
chmod +x /tmp/linuxdeploy-plugin-qt-x86_64.AppImage
/tmp/linuxdeploy-plugin-qt-x86_64.AppImage --appimage-extract
cp /tmp/squashfs-root/usr/bin/linuxdeploy-plugin-qt /tmp/linuxdeploy-extracted/plugins/
rm -rf /tmp/squashfs-root

# Download and extract appimagetool
wget https://github.com/AppImage/AppImageKit/releases/download/continuous/appimagetool-x86_64.AppImage \
  -O /tmp/appimagetool-x86_64.AppImage
chmod +x /tmp/appimagetool-x86_64.AppImage
/tmp/appimagetool-x86_64.AppImage --appimage-extract
mv /tmp/squashfs-root /tmp/appimagetool-extracted
```

### 5b. Run the AppImage build script

> **Note**: The script takes **5–10 minutes** — linuxdeploy processes every bundled library. Strip "ERROR" messages on stderr are harmless.

```bash
./deployment/Linux_AppImage/create-appimage.sh
```

Output: `XMALab-x86_64.AppImage`

### 5c. Test the AppImage

```bash
# From outside the distrobox (on the host):
chmod +x /path/to/XMALab-x86_64.AppImage
QT_QPA_PLATFORM=offscreen /path/to/XMALab-x86_64.AppImage

# Expected output: "Error: Missing GL version"
# (Normal for headless - will show GUI when run with a display)
```

---

## AppImage Structure

The AppImage bundles:
- XMALab binary and all shared libraries
- Qt6 platform plugins (xcb, wayland, eglfs, offscreen)
- OpenCV with FlexiBLAS (BLAS backend switching)
- Desktop file and icon for menu integration

---

## Known Issues

- **Samba/OpenCV**: OpenCV links `libsmbclient` on Fedora, which pulls in Samba private libraries. The build script aggressively removes all Samba libs. The binary still works; it just can't read video from SMB shares.
- **FlexiBLAS**: Uses the `netlib` backend by default for maximum compatibility. The `FLEXIBLAS_LIBRARY_PATH` and `FLEXIBLAS_CONFIG` env vars are set in the AppRun script.
- **Wayland vs XCB**: The AppImage bundles both xcb and wayland platform plugins. On Wayland sessions, Qt probes Wayland first; if the compositor is missing a feature, it falls back to xcb. Set `QT_QPA_PLATFORM=xcb` to force the xcb backend.

---

## File Locations (in Repo)

| Item | Path |
|------|------|
| Build preset | `CMakePresets.json` → `linux-system` |
| Binary | `build/linux-system/bin/Release/XMALab` |
| AppImage | `XMALab-x86_64.AppImage` (project root) |
| Desktop file | `deployment/Linux_AppImage/XMALab.desktop` |
| AppRun script | `deployment/Linux_AppImage/AppRun` |
| Build script | `deployment/Linux_AppImage/create-appimage.sh` |
| Icon | `deployment/Linux_AppImage/XMALab.png` |
