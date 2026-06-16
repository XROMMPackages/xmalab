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
The project includes a pre-configured AppDir at `XMALab.AppDir/`. To package the compiled binary into a portable AppImage:

1. Copy the compiled binary into the AppDir:
   ```bash
   cp build/linux-system/bin/Release/XMALab XMALab.AppDir/usr/bin/XMALab
   ```

2. Ensure the AppRun script is executable:
   ```bash
   chmod +x XMALab.AppDir/AppRun
   ```

3. Download and extract `appimagetool` (FUSE is typically unavailable in build containers, so extract the AppImage):
   ```bash
   wget 'https://github.com/AppImage/AppImageKit/releases/download/continuous/appimagetool-x86_64.AppImage' -O /tmp/appimagetool
   chmod +x /tmp/appimagetool
   /tmp/appimagetool --appimage-extract
   ```

4. Generate the AppImage:
   ```bash
   /tmp/squashfs-root/AppRun XMALab.AppDir
   ```

   This creates `XMALab-x86_64.AppImage` in the project root.

---

## Troubleshooting & Cleaning
If you encounter build errors, it is highly recommended to wipe the `build/` directory completely and start over.
You can use `git clean` to completely reset the repository to a freshly-cloned state (Warning: This deletes ALL untracked files).
```bash
git clean -fdx
```
