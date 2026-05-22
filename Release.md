# Release Notes (v3.0.0 In-Progress)

This document tracks all backend and user-facing changes made during the `c++upgrade` modernization branch. It will be used to compile the final release notes when merged into the main release.

## User-Facing UI/UX Changes
- **Lock UI Panes**: Added a toggle option to the `View` menu allowing users to lock all dockable widgets in place to prevent accidental undocking/movement.
- **Improved Detection Tooltips**: Added detailed, explanatory tooltips to the "Detection Method" dropdowns across the application (`Detection Settings`, `Marker Settings`, and `Global Settings`). Users can now hover over options like "default Xray marker" or "Blobdetection" to understand exactly what algorithm is used under the hood. Clarified that the "Input" dropdown is strictly a visualization preview.
- **Responsive Points Pane**: The bottom buttons (set markers, set rigid bodies, import/export) in the `Points` dock widget now dynamically stack vertically if the pane is resized to be too narrow, preventing the buttons from being cut off.
- **Consistent Dark Mode Styling**: Implemented a global stylesheet application (`MainWindow::applyTheme`) to fix native Windows widget styling bugs and ensure a clean, consistent dark mode experience.

## Backend & Modernization
- **Pointer Safety Enhancements**: Migrated unsafe UI pointer-chaining to robust, modernized functional wrappers (`State::getActiveTrialData()` and `Trial::withActiveMarker()`). These use lambdas to ensure bounds checking and prevent null pointer dereferences.
- **Crash Fixes**: Resolved a crash relating to `RigidBody` interpolation by utilizing the new pointer safety patterns.
- **Performance**: Parallelized `ButterworthLowPassFilter.cpp` to significantly improve filtering speed for large datasets.

## Build Infrastructure & Maintenance
- **Standardized Build Process**: Cleared out outdated legacy build directories and completely standardized the cross-platform CMake build process using `CMakePresets.json`. 
- **Build Documentation**: Created a comprehensive `BUILD.md` file at the repository root detailing the exact steps to compile XMALab from scratch on Windows using vcpkg.
- **Versioning**: Incremented software versioning to `3.0.0` across `CMakeLists.txt` and installer scripts (`XMALabInstaller.nsi`).
