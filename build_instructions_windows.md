# Windows Build and Deployment Instructions

This guide covers building and packaging XMALab on Windows using **Visual Studio 2022**, **CMake**, and the **vcpkg** package manager.

---

## 1. Prerequisites

Before starting, install the following tools:
- **Visual Studio 2022**: Install the community, professional, or enterprise edition. Make sure you select the **Desktop development with C++** workload during installation.
- **CMake**: Download and install CMake (v3.25 or newer) from [cmake.org](https://cmake.org/download/). Ensure it is added to your system PATH.
- **Git**: Download and install Git from [git-scm.com](https://git-scm.com/).
- **NSIS (Nullsoft Scriptable Install System)**: Download and install NSIS from [nsis.sourceforge.io](https://nsis.sourceforge.io/Main_Page) (required for creating the installer).

---

## 2. Set Up Dependencies via vcpkg

XMALab relies on Microsoft's **vcpkg** toolchain to automatically resolve libraries on Windows.

1. **Clone and Bootstrap vcpkg**:
   Open a terminal (e.g., Command Prompt or PowerShell) and run:
   ```cmd
   git clone https://github.com/microsoft/vcpkg.git C:\vcpkg
   cd C:\vcpkg
   .\bootstrap-vcpkg.bat
   ```

2. **Set the Environment Variable**:
   Set `VCPKG_ROOT` as a system environment variable pointing to `C:\vcpkg` (or your custom install directory).
   - In PowerShell, you can set it temporarily or permanently:
     ```powershell
     [Environment]::SetEnvironmentVariable("VCPKG_ROOT", "C:\vcpkg", "User")
     ```
   - *Note*: Restart your terminal/IDE after setting system environment variables for them to take effect.

3. **Install Required Libraries**:
   Run the following command to download and compile all dependencies for the standard 64-bit Windows triplet:
   ```cmd
   C:\vcpkg\vcpkg install opencv4 quazip glew levmar
   ```
   *(Note: This might take some time as it compiles OpenCV and its prerequisites).*

---

## 3. Clone and Build XMALab

1. **Clone the Repository**:
   ```cmd
   git clone https://github.com/XROMMPackages/xmalab.git
   cd xmalab
   ```

2. **Configure the Build via CMake Presets**:
   XMALab uses standard CMake Presets. This step configures the build using Visual Studio and hooks up the vcpkg toolchain automatically:
   ```cmd
   cmake --preset windows
   ```

3. **Compile the Release Version**:
   Run the compilation step using the release build preset:
   ```cmd
   cmake --build --preset windows-release
   ```
   - This will build the executable under `build/windows/bin/Release/XMALab.exe`.
   - **Automatic Deployment**: The build process automatically runs `windeployqt` as a post-build step, copying all required Qt DLLs, plugins, and platforms directly into the output directory next to `XMALab.exe`.

---

## 4. Packaging the Windows Installer (NSIS)

1. Open **NSIS** on your computer.
2. Select **Compile NSI scripts**.
3. Load the script located at `deployment/Windows_NSIS/XMALabInstaller.nsi`.
4. Compile the script. This will bundle the compiled binary, assets, and dependencies into a standalone installer (`XMALab_Setup.exe`).
