# Release Notes (v3.0.0 In-Progress)

This document tracks all backend and user-facing changes made during the `c++upgrade` modernization branch. It will be used to compile the final release notes when merged into the main release.

## User-Facing UI/UX Changes
- **Lock UI Panes**: Added a toggle option to the `View` menu allowing users to lock all dockable widgets in place to prevent accidental undocking/movement.
- **Improved Detection Tooltips**: Added detailed, explanatory tooltips to the "Detection Method" dropdowns across the application (`Detection Settings`, `Marker Settings`, and `Global Settings`). Users can now hover over options like "default Xray marker" or "Blobdetection" to understand exactly what algorithm is used under the hood. Clarified that the "Input" dropdown is strictly a visualization preview.
- **Responsive Points Pane**: The bottom buttons (set markers, set rigid bodies, import/export) in the `Points` dock widget now dynamically stack vertically if the pane is resized to be too narrow, preventing the buttons from being cut off.
- **Consistent Dark Mode Styling**: Implemented a global stylesheet application (`MainWindow::applyTheme`) to fix native Windows widget styling bugs and ensure a clean, consistent dark mode experience.
- **Improved Tracking UI**: Cleaned up the tracking UI to remove confusing and broken epipolar constraint models, and added an explicit checkbox to toggle the experimental 3D volumetric tracking buttons.
- **Interpolation UX**: Added an explicit "Set Interpolation" button with clear tooltips and a safety warning to guide the user to shift-select a range on the plot before attempting to assign an interpolation method to missing frames.

## Backend & Tracking Algorithms
- **Pointer Safety Enhancements**: Migrated unsafe UI pointer-chaining to robust, modernized functional wrappers (`State::getActiveTrialData()` and `Trial::withActiveMarker()`). These use lambdas to ensure bounds checking and prevent null pointer dereferences.
- **Crash Fixes**: Resolved a crash relating to `RigidBody` interpolation by utilizing the new pointer safety patterns.
- **Video Decoding Resilience**: Added an interception layer to OpenCV's `VideoCapture` (`AviVideo.cpp`) to prevent the video decoder from entering a permanent unrecoverable error state when seeking to improperly encoded frames (such as EOF overestimations common in AVIs).
- **Sub-Pixel Refinement**: Integrated `MarkerDetection::detectionPoint` into `MarkerTracking3D` to ensure volumetric tracking correctly snaps to the precise sub-pixel centroid of the marker, preventing tracking drift and jumping between neighboring beads.

## Build Infrastructure & Maintenance
- **Standardized Build Process**: Cleared out outdated legacy build directories and completely standardized the cross-platform CMake build process using `CMakePresets.json`. 
- **Build Documentation**: Created a comprehensive `BUILD.md` file at the repository root detailing the exact steps to compile XMALab from scratch on Windows using vcpkg.
- **Versioning**: Incremented software versioning to `3.0.0` across `CMakeLists.txt` and installer scripts (`XMALabInstaller.nsi`).
- **macOS bundle metadata**: Added `MACOSX_BUNDLE_BUNDLE_NAME`, `BUNDLE_VERSION`, and `GUI_IDENTIFIER` to `CMakeLists.txt` so the app appears with proper name/version in the macOS menu bar and About dialog.
- **ESC-tracking crash fix**: Pressing ESC during marker tracking left `disableDraw = true` permanently and frame state inconsistent, causing a crash on next mouse click. Fixed by adding `setDisableDraw(false)` and frame state refresh to `WizardDigitizationFrame::stopTracking()`. Also removed duplicate ESC handler from `Shortcuts::eventFilter` (QShortcut already handles it) and added `disableDraw` guard to `GLCameraView::mousePressEvent`.
- **AppImage libxkbcommon fix**: Bundled `libxkbcommon` 1.6.0 from Fedora 40 could not parse the host's newer XKB keymap data (`xkeyboard-config`), crashing on any keyboard event. Fixed by excluding libxkbcommon from the AppImage so the host's version is used at runtime.


## Currently Broken
- ~~**MacOS (maybe all OS) in 'force close' macos menu, XMALab doesn't have a title** — fixed by adding `MACOSX_BUNDLE_BUNDLE_NAME` and related properties to CMakeLists.txt.~~
- ~~**Linux Crashing a lot**: Crashes if you close the 3D world view — fixed by accepting the close event instead of ignoring it, guarding division by zero in paintGL, and moving quadric allocation to initializeGL.~~ 
- **Linux and Windows still _much_ slower at tracking, and scrubbing than MacOS** May not be fixable.
- ~~**MacOS Theming (maybe all OS)** - light theme on MacOS does not match 'follow system theme' when MacOS is in light mode. Fixed by replacing hardcoded grey palette with `QPalette()` for light theme, making it identical to system theme.~~
- ~~**Settings button crash**: clicking settings button next to marker crashes — fixed by adding bounds checks in `MarkerTreeWidgetButton`.~~
- ~~**Set interpolation crash**: clicking set interpolation crashes — fixed by bounds-safe `Marker::setInterpolation/getInterpolation` and safe access in `PlotWindow::setInterpolation`.~~


## Todos for Next Phase
1. **Automate/Calibrate Marker Detection**: Investigate sampling a user-identified "good" frame to automatically estimate and set ideal `Threshold` and `Penalty` detection settings based on pixel profile.
2. **Implement Autosave**: Build a robust background autosave feature to prevent data loss. create .xma1 while working.  save in background every 10 minutes (configurable in settings). On clean save and exit, save to .xma and remove .xma1.  
3. **mark change to file with asterix in the title bar**: let people know if a file has been changed.
4. **Save image slider state (bias etc)**


gitbook wiki blender animation stuff
