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
- **MarkerTracking3D (experimental "volumetric" tracker, 2D + triangulation)**: An alternative to the per-camera 2D tracker that uses the calibration to keep all cameras consistent. It is enabled by the "3D tracking" checkbox in the tracking UI and requires a calibrated trial and a 3D point at the source frame (a marker with no 3D point is skipped). Per tracked marker and frame it does the following:
  1. **Per camera, NCC matching.** A template is cut around the marker at the source frame and masked to a disc (marker radius + 2 px) so a touching neighbour cannot bias the match. The 2D position is extrapolated linearly from the previous two frames (falling back to the projected 3D point) and a normalised cross-correlation map is computed over a search window of ±max(30 px, 3× marker size) around it. Cameras where the marker is undefined at the source frame are skipped.
  2. **Map normalisation and priors.** Raw NCC on grey X-ray background is nearly flat (~0.98 everywhere), so each camera's map is stretched to [0, 1] before anything else, as the 2D tracker does. A wide Gaussian prior around the prediction is applied, and locations claimed by the trial's other markers (their point at the target frame if defined, else their own prediction) are down-weighted so a neighbour's blob does not attract the match. Claims that coincide with this marker's own prediction or its own source position are ignored, since suppressing them would only blind the camera.
  3. **Peaks and 3D candidates.** Up to two peaks per camera are kept, separated by at least one marker radius. Every cross-camera pair of peaks is triangulated. A pair is only a candidate if the 3D point reprojects back onto both peaks within 3 px; pairs that fail are different features and do not compete. Candidates are scored by the summed reprojected NCC value over all cameras, blended with a distance-from-prediction prior whose sigma equals the search radius and whose weight is the marker's tracking penalty (the default of 50 gives equal weight).
  4. **Refinement.** The best candidate is hill-climbed in 3D with a step derived from the calibration so that it moves ~0.5 px in the most sensitive camera. All world-unit quantities (step, prior sigma, velocity gate) are derived from pixels this way, so the tracker behaves the same for millimetre or centimetre calibrations.
  5. **Fallback.** If no consistent candidate exists (fewer than two cameras, no peaks, or only inconsistent pairs), each camera takes its own best NCC peak, which is what the 2D tracker would produce, rather than projecting an unsupported 3D point.
  6. **Snap.** Each camera's 2D result is refined by `MarkerDetection::detectionPoint` with the marker's detection method, and the detected size is recorded, as after 2D tracking. The snap is refused if the detected blob is more than 1.5× the marker's most recent size in that camera, which indicates two markers thresholded into one blob; the NCC position is then kept. The reference is the recent size rather than the trial mean so that markers that grow (magnification) or rotate (non-spherical markers) keep snapping.

  Limitations: if two markers already sit on the same blob in a camera at the source frame, nothing in that frame distinguishes them and the tracker keeps the shared assignment ("wrong but stable") rather than guessing; correct the seed frame by hand. Strongly elongated markers may have snaps refused in frames where their enclosing radius jumps by more than 1.5× within a frame.

  **Diagnostics.** Set the environment variable `XMALAB_TRACK3D_DEBUG=<directory>` before launching XMALab to dump, per tracked frame/marker/camera, the template, masked template, search ROI, and raw and weighted NCC maps as PNGs, plus `track3d_log.txt` with predictions, peaks, every candidate with its reprojection residual and score, the chosen 3D point and each snap decision. `XMALAB_TRACK3D_DISABLE=<comma list>` turns individual features off for A/B testing on the same build: `mask`, `claims`, `guard`, `nms`, `size`. Both are off unless set. When reporting a tracking problem, please include the log.

## Build Infrastructure & Maintenance
- **Standardized Build Process**: Cleared out outdated legacy build directories and completely standardized the cross-platform CMake build process using `CMakePresets.json`. 
- **Build Documentation**: Created a comprehensive `BUILD.md` file at the repository root detailing the exact steps to compile XMALab from scratch on Windows using vcpkg.
- **Versioning**: Incremented software versioning to `3.0.0` across `CMakeLists.txt` and installer scripts (`XMALabInstaller.nsi`). Currently `3.0.0-beta.4`.
- **macOS bundle metadata**: Added `MACOSX_BUNDLE_BUNDLE_NAME`, `BUNDLE_VERSION`, and `GUI_IDENTIFIER` to `CMakeLists.txt` so the app appears with proper name/version in the macOS menu bar and About dialog.
- **ESC-tracking crash fix**: Pressing ESC during marker tracking left `disableDraw = true` permanently and frame state inconsistent, causing a crash on next mouse click. Fixed by adding `setDisableDraw(false)` and frame state refresh to `WizardDigitizationFrame::stopTracking()`. Also removed duplicate ESC handler from `Shortcuts::eventFilter` (QShortcut already handles it) and added `disableDraw` guard to `GLCameraView::mousePressEvent`.
- **AppImage libxkbcommon fix**: Bundled `libxkbcommon` 1.6.0 from Fedora 40 could not parse the host's newer XKB keymap data (`xkeyboard-config`), crashing on any keyboard event. Fixed by excluding libxkbcommon from the AppImage so the host's version is used at runtime.
- **Camera calibration import/export precision fix**: Fixed precision loss when importing/exporting camera calibration data (`ExternalCalibrationFrame.cpp`). Changed `float`→`double` and `%f`→`%lf` in `fscanf` calls to preserve 64-bit precision. Updated all `precision(12)`→`precision(17)` across 10 files for lossless round-tripping of 64-bit doubles on export.


## Changes in beta.4 (volumetric tracker)
All changes are confined to `src/processing/MarkerTracking3D.cpp/.h`; the 2D tracker is untouched. Bugs fixed in the beta.3 tracker:
- Search ROI was off-centre by one search radius (`Image::getSubImage` takes a half-size).
- The "no 3D point" test checked for (0,0,0) but undefined points are (-1000,-1000,-1000); now uses `status3D`.
- Cameras with the marker undefined at the source frame got a garbage template and an uninitialised prediction; now skipped.
- `extractPeaks` ignored its minimum-distance argument, so the second peak was usually a sub-peak of the same blob.
- No marker size was recorded for 3D-tracked frames.
- Hill-climb step, distance prior and velocity gate were fixed world-unit constants (0.5 and 5 units); on a centimetre calibration the step was ~16 px and the prior had no effect. All are now derived from pixels via the calibration.
- The raw NCC map was used without normalisation, so any prior applied to it dominated the ~0.5% contrast between marker and background.

New behaviour: disc-masked template, suppression of other markers' claimed locations, ray-consistency gate on candidate pairs with 2D fallback, merged-blob snap guard, per-marker penalty controlling the 3D prior weight, and the diagnostics described above.

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
