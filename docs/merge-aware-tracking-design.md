# Merge-aware tracking: design note

Status: proposal for discussion, no code yet. Written 2026-10-14 after an offline analysis of the two reference projects. Replaces the beta.3 "volumetric" tracker (`src/processing/MarkerTracking3D.*`).

## 1. What we are trying to do

Markers on different bones pass in front of each other in one camera. While they overlap, 2D tracking in that camera follows a merged blob, and when the pair separates it often follows the wrong one. The other camera still sees both markers apart. The goal is to use that second camera to keep each marker's identity through the merge and to pick the right blob at the split, and to stop rather than write a wrong point when the evidence is not there.

Everything else about tracking stays as it is. This is not a 3D search.

## 2. What the data says

Measured from the hand-checked 2D points in the two projects, with a fundamental matrix fitted to the user's own correspondences (0.5 px median residual on the rat, 0.2 px on the pig). A merge is two marker centres within 6 px in one camera.

| | Rat (`Rob - xromm`, frames 0-299) | Pig tutorial (`XROMM example`, frames 49-77) |
|---|---|---|
| Marker radius | ~2.2 px | ~2.2 px |
| Pixels per calibration unit (cm) | 65 to 100 | 32 to 40 |
| Nearest other marker, median | 21 to 31 px | 40 px |
| Marker-frames with a neighbour within 10 px | 12 to 18 % | 0 to 4 % |
| Per-frame displacement, 90th percentile | 4 to 5 px | 2.5 px |
| Merge episodes | 70 (36 cam 1, 34 cam 2), median 2-3 frames, longest 57 | 1 |
| Merged in both cameras at once | never | never |

At the frame a merged pair separates, which cue picks the right blob?

| Cue | Rat cam 1 | Rat cam 2 |
|---|---|---|
| Epipolar line from the clean camera, margin over 2 px | 78 % | 50 % |
| Epipolar cue wrong by more than 1 px | 3 % | 6 % |
| Split runs along the epipolar line, no information | 8 % | 26 % |
| Constant-velocity extrapolation across the merge, 2D or 3D | 36 % | 47 % |
| Hold the pre-merge position | 14 % | 12 % |

Conclusions that drive the design:

1. The clean camera's epipolar line is the decider. The correct blob stays on it to within the calibration residual; the wrong blob sits off it by the across-line component of the separation.
2. Extrapolating motion across a merge is a coin flip and must not be used to decide. Pairs usually cross rather than bounce.
3. A quarter of camera-2 splits run along the epipolar line. In the rat the lines are vertical in the lateral view and jaw motion is vertical. The pig's single merge is also along the line: margin 0.1 px at the split, 2.6 px four frames later. These cases are resolved by waiting, not by guessing.
4. The reference data itself carries merged-centroid errors of 3 to 6 px at merge frames, and the pig tutorial assigns two markers the identical position for two frames. A point measured on a merged blob is not a measurement of either marker and must not be written as TRACKED.

## 3. Why the current volumetric tracker is replaced rather than fixed

- It searches raw `TM_CCORR_NORMED` maps (0.97 to 0.99 on empty background) under a Gaussian prior, ignoring the normalisation and the user's penalty that the 2D tracker uses, so faint blobs lose to background near the prediction.
- Its peak extractor ignores its minimum-distance argument, so there is effectively one candidate per camera and pairwise triangulation adds no discrimination. With two cameras every pair has a solution and the residual is never checked.
- Its hill-climb step, proximity prior and velocity gate are in calibration units. Both projects are in centimetres, so the step is 35 to 50 px on the rat and 16 to 20 px on the pig. It moves freely along the direction the two cameras cannot constrain, which is the depth slide seen on marker 8.
- It writes TRACKED for every camera regardless of per-camera status, runs no validity check, records no size, and cuts the next template from whatever it wrote.
- The September rework on `backup/tracker-work-beta4` tried normalisation, pixel-expressed gates and fail-closed inside this structure and did not beat 2D tracking.

## 4. Principles

- 2D tracking per camera remains the only source of measured points: correlation with the user's penalty, then the detector, then the existing validity check.
- 3D and the other cameras are used to predict, to detect merges, and to decide identity. They never generate a point written as TRACKED.
- Every threshold is in pixels and comes from a measured quantity: the marker's detected radius, or the calibration residual.
- Every written point carries a status that says how it was obtained. A held or provisional point is PREDICTED, which already exists in `markerStatus`.
- When the evidence is ambiguous the tracker stops the affected markers, as the 2D path does today on a failed validity check.

## 5. Per-frame pipeline

This runs inside the existing track-all / track-selected flow after each frame's 2D tracking, detection and validity check have completed.

**A. Merge detection, per camera.** Two signals, either is enough:
- two markers' detected centres are within `2r + 1` px, where `r` is each marker's mean detected radius;
- a marker's detected size exceeds `1.5 × meanSize` while another marker's track is within `3r`.

Markers caught by this form a merge group in that camera. Only the merged camera is affected; the other cameras continue tracking normally. Merges in all cameras at once were never observed and would simply stop the group.

**B. Holding through the merge.** For each marker in a group, in the merged camera:
- the written point becomes the reprojection of its 3D estimate: the clean camera's ray intersected with the merged blob's position along the epipolar line, which locates the marker to within a blob radius;
- status PREDICTED, not TRACKED, and no size recorded;
- the correlation template for the next frame is not re-cut from the merged blob; the last template taken from a validated single blob is kept.

Tracking of a PREDICTED point continues as normal on the next frame, because the track-all filter already re-tracks anything with status at or below TRACKED.

**C. Split and assignment.** When the detector reports distinct blobs near the group again, build a cost matrix of markers against blobs. The cost of giving marker m to blob b is the distance in undistorted image space from b to the epipolar line of m's point in each clean camera, summed over clean cameras. Take the lowest-cost permutation and record the margin to the second-lowest.

- Margin above `T_epi`: commit. Each marker gets its assigned blob, refined by the detector, written TRACKED with size.
- Margin at or below `T_epi`: write the lowest-cost assignment as PREDICTED and keep the group in a pending state. On each following frame recompute the margin from the current points. When it exceeds `T_epi`, commit; if the committed permutation differs from the provisional one, swap the pending frames between the markers and upgrade them to TRACKED. The swap is exact because each 2D track followed a continuous blob. If the group is still pending after `K` frames, or any member fails validity meanwhile, reset the pending frames for the group to UNDEFINED and stop the run. Pending frames never overwrite SET or MANUAL points.

**D. Audit of clean tracks.** For every marker measured in two or more cameras, the distance of each camera's point to the epipolar line from the other camera is an inconsistency measure that does not depend on triangulation. Above `T_audit` and not in a merge group means one camera has jumped to the wrong blob: reset and stop that marker. The existing `MaximumReprojectionError` gate (default 5 px of reprojection error) catches the same thing only once the inconsistency is roughly twice that, so this audit tightens it rather than duplicating it.

## 6. Rigid-body assist (optional, second step)

When a marker belongs to a rigid body whose other reference markers are tracked at this frame, `RigidBody::computePose` and `transformPoint` give its 3D position directly. Two uses:
- as the search centre for the merged camera instead of the linear 2D prediction;
- as a second term in the assignment cost, the distance from each blob to the body-predicted projection, weighted by the body's fit error.

This resolves cross-body merges that separate along the epipolar line immediately, the pig 6/7 case being one. It does not help the rat's most frequent merging markers, 10, 16, 17 and 18, which are not rigid with respect to anything.

## 7. Thresholds

| Name | Value | Source |
|---|---|---|
| `r` | per marker | mean detected radius, already maintained by `Marker` |
| merge distance | `2r + 1` px | blobs touching |
| size jump | `1.5 × meanSize` | merged blobs measured 5 to 8.5 px against 2 to 3 px normal |
| `T_epi` | 2 px default | 1.1 × the 95th-percentile epipolar residual of the project's own tracked points; the rat gives 1.8 px, the pig 0.7 px |
| `K` | 10 frames | pig needed 4, rat episodes have median length 2-3 |
| `T_audit` | 3 px | above the calibration residual, below the existing 5 px reprojection gate |

`T_epi` should be computed from the project once enough frames are tracked and shown in the settings dialog with the measured value, rather than left as a magic number.

## 8. Integration points

- New `src/processing/MergeMonitor.*` holding per-camera group state across frames, invoked from `WizardDigitizationFrame` where `checkIfValid` runs today for the 2D track types.
- `Marker::getEpipolarLine` already computes the undistorted line through `P2 · P1⁻¹`; expose the line coefficients and add a distance helper that undistorts the candidate point first.
- Status: PREDICTED is currently drawn in the undefined colour; give it its own colour so provisional points are visible.
- UI: the "Enable Volumetric Tracking" checkbox and its two button groups become a single "Resolve merges using other cameras" option that applies to the normal track buttons. `MarkerTracking3D` and track types 4 to 6 are removed.
- No changes to `MarkerTracking`, `MarkerDetection` or `Marker::isValid`.

## 9. Validation

- Rat frames 0 to 450: development set. Replay from frame 0 over all markers and compare every written TRACKED point to the reference. Report median and 95th-percentile error, number of stops, number of identity errors after a merge, and number of PREDICTED frames. Target: precision equal to 2D tracking, zero silent swaps, stops only where the reference itself is ambiguous.
- Rat frames beyond 450: hold-out once tracked.
- Pig frames 49 to 77: do-no-harm check. Precision identical to 2D, and the 6/7 merge resolved within five frames, or immediately with the rigid-body assist.
- A second rat trial, if available, guards against tuning to this animal's geometry.
- Comparison is done offline from exported 2D points with the scripts used for this note.

## 10. Open questions

1. Merge handling as a mode of the normal track buttons, or kept as a separate button group?
2. Should PREDICTED points get their own colour, and should the back-swap be shown to the user when it happens?
3. Is the rigid-body assist in scope for the first version?
4. `K` and `T_audit`: stop sooner or later than proposed?

## 11. Decisions (2026-10-14)

1. Separate button group. The existing volumetric button group is repurposed for merge-aware tracking; the plain 2D track buttons stay exactly as they are.
2. PREDICTED points get their own colour setting. Back-swaps are not shown in the UI; they are logged to the console.
3. Rigid-body assist is in the first version. It is active only for markers that belong to a rigid body, so it applies to the pig project and not the rat.
4. Thresholds as proposed in section 7.

## 12. Implementation status (2026-10-14)

Implemented and building; not yet exercised on data. Files:

- `src/processing/MergeMonitor.h/.cpp`: merge detection, holding, split assignment with deferral and back-swap, audit. Thresholds read from settings `MergeEpipolarThreshold` (2 px), `MergeAuditThreshold` (4 px), `MergeMaxPendingFrames` (10).
- `src/core/Marker`: `getEpipolarLineUndistorted`, `distanceToEpipolarLine`, `projectOntoEpipolarLine`, `getSize(camera, frame)`; PREDICTED drawn in `ColorPredicted`.
- `src/core/RigidBody`: `fitRigidTransform` factored out of `computePose`; `predictMarkerPosition` predicts a member from the other members' pose.
- `src/ui/WizardDigitizationFrame`: track types 5 and 6 now run `trackMerge` (2D tracking with templates from the last clean frame, detection, validity, then `MergeMonitor::processFrame`); console gets the monitor's messages. `MarkerTracking3D` removed.
- `src/ui/StatusColorDialog`: colour entry for PREDICTED.

Deviation from section 5: candidate blobs at a split are enumerated by seeding the existing detector around the group (nine seeds) rather than a new detector entry point; blobs within the merge distance of a marker outside the group are excluded. The audit default is 4 px rather than 3 px because the rat reference's 99th-percentile epipolar residual is 2.7 px.

First tests to run: pig frames 49 to 77 with merge-aware all (expect the 6/7 overlap in camera 1 to be held, then resolved within a few frames, or immediately via the rigid bodies); rat frames around 430 to 450 with markers 8 and 16 (the camera-2 overlap that defeated the old tracker).

## 13. Measured results (2026-10-14, later the same day)

A headless driver was added: `XMALab --merge-test project.xma start end output.xma [2d]` loads the project as the GUI does, clears frames start+1 to end (0-based), tracks forward with merge-aware all (or plain 2D all with `2d`), saves `output.xma` and writes `output.xma.log` with every MergeMonitor decision (debug detail is on in this mode). Scoring against the reference is done offline from the saved CSVs.

First GUI test by the user lost points on both datasets. The driver reproduced it and the log identified three faults, all fixed: (1) while a pair is merged the hold could only place a marker on its epipolar line, not along it, so a marker that moved along the line drifted; the hold now uses the rigid-body prediction when the marker belongs to a body; (2) when gating left a single valid assignment the margin was computed as zero and a correct split stayed provisional; margins are now per member and "no alternative" counts as decisive; (3) after a hold the 2D tracker extrapolated velocity from held positions and overshot; the frame after a release now uses the last position without velocity, and members placed on a clean blob refresh their template from it. Candidates at a split now come from every contour the detector finds around the trackers and the body predictions, not the closest contour to a few seeds, and a member whose best option is still inconsistent is reset rather than committed.

Pig, frames 50-77, 10 markers (560 marker-frames):

| | Merge-aware |
|---|---|
| Tracked / held / lost | 550 / 10 / 0 |
| Error where tracked, median / 95th pct | 0.00 / 0.50 px |
| The 6/7 overlap | held frames 51-55 on the jaw and cranium predictions, split found at 56 with a 12 px margin, both markers exact afterwards |

Rat, frames 301-450, 19 markers (5700 marker-frames), reference is the user's track verified to frame 450:

| | Plain 2D all | Merge-aware all |
|---|---|---|
| Tracked | 3875 (68%) | 3855 (68%) |
| Held as PREDICTED | 0 | 60 |
| Lost | 1825 | 1785 |
| Error where tracked, median / 95th pct | 0.31 / 3.44 px | 0.30 / 0.81 px |
| Tracked points more than 3 px wrong | 205 | 34 |

The 34 are mostly marker 10 in camera 1, which both runs track alone after camera 2 is lost at frame 302, so no consistency check is possible. Merges resolved correctly: 7/18, 2/9 twice, 6/16 after a 14-frame hold, 3/6 the first time. One merge (3/6 the second time) stayed provisional with a negative margin until a member was lost and both were cleared. Markers 18, 19, 17, 16 and 3 are tracked wrongly for 8 to 108 frames by plain 2D; merge-aware stops them instead.

The losses shared by both runs (markers 4, 11, 12, 14, 10, 20 and others) are the 2D path's own validity gate resetting a marker after one camera's tracker jumped, which stops the marker permanently. The reference got through those frames with the old tracker plus manual work. The next lever is not in the merge logic: a bounded "hold and recover" that keeps an inconsistent or oddly sized measurement as PREDICTED on the epipolar line for up to K frames while the tracker retries from there, and only then resets. That changes the stop policy and needs the user's agreement.

## 14. Hold-and-recover policy and results (2026-10-14, evening)

Agreed with the user: a doubtful measurement is kept as a PREDICTED point and retried rather than reset, and a marker left with only one camera is stopped in this mode. Section 5's "same validity check as 2D" is superseded: in merge-aware mode `MergeMonitor::checkMarkers` replaces `Marker::isValid` for markers outside a group.

Rules, in the order they are applied to each marker after the frame's 2D tracking and detection:

1. One camera only (with two or more visible): the tracked point is reset. Applies to group members too.
2. A camera held last frame must come back within `max(3r, 2.5 x the motion seen elsewhere + 3 px)` of its hold, else it stays held.
3. A step that differs from the median step of the other tracked markers in that camera by more than `max(3r + 2, 0.75 x median + 3)` px marks that camera suspect. Over five frames, covering less than 40% of the median distance, or going the opposite way, also marks it suspect.
4. Cameras disagreeing by more than `MergeAuditThreshold` (4 px; correct points never exceed 3.3 px on the rat) mark one camera suspect: the one with a blob over 2r, else the one that moved most. Blob size alone is never a trigger: 8 to 14% of correct points have sizes above 1.5r.
5. A suspect camera is held on the epipolar line of its other measurements at its previous position, PREDICTED, size cleared, template taken from its last single-blob frame, and the next frame's search starts there without velocity. After `MergeMaxHoldFrames` (10) consecutive holds, or when no other camera is available, the camera is reset.
6. Two trackers landing on one blob are only treated as a merge if neither jumped: a member that moved more than `max(3r + 2, 2.5 x the others' step + 2)` px, or that disagrees with its own other camera while a resident agrees, is an intruder and is held alone. Markers that no other camera sees cannot join a group, and group members that lose their other camera are dropped.

Measured against the same references as section 13:

| | Plain 2D | Merge-aware, section 13 | Merge-aware, this section |
|---|---|---|---|
| Rat 301-450 tracked | 3875 (68%) | 3855 (68%) | 4584 (80%) |
| Rat held as PREDICTED | 0 | 60 | 196 |
| Rat tracked points more than 3 px wrong | 205 | 34 | 54 |
| Rat PREDICTED points more than 3 px off | 0 | n/a | 78 |
| Rat error where tracked, 95th pct | 3.44 px | 0.81 px | 1.12 px |
| Pig 50-77 tracked / held / lost | | 550 / 10 / 0 | 551 / 9 / 0 |

Marker 5, which the user saw stopped in the GUI, is now tracked on every frame in both cameras with no error: its loss came from marker 14's tracker jumping onto its blob at frame 417, which the old grouping treated as a merge of the two.

The remaining wrong tracked points sit almost entirely in frames 435 to 451 in camera 2, where the head moves 5 to 9 px per frame. There the 2D tracker lags along the vertical epipolar line or latches onto bone that moves with the skull; both are invisible to epipolar geometry, and coherent bone motion is invisible to the coherence test too. Plain 2D fails the same frames worse. A faster-motion-aware 2D tracker, or a bigger template, is the next lever and is outside this layer.

## 15. Appearance test and overlap timeout (2026-10-14, late)

The user reported background being tracked after a long merge-aware run (around frame 542). Reproduced with the driver from frame 441: once most markers have stopped, fewer than four remain in a camera, the coherence test switches off, and survivors only have to agree with each other geometrically, which two trackers on the same bone feature do. Two additions close this:

- **Appearance.** On correct rat tracks the detected blob size lies outside 0.5r to 2r on under 2% of frames, so a camera with no marker-sized blob on three of the last five frames of this run is held, and a recovery must land on a marker-sized blob. Only frames the current run produced count, because points from the old tracker or manual placement carry no size. `MarkerDetection::detectionPoint` now reports size -1 when it finds nothing instead of an uninitialised value.
- **Overlap timeout.** A merge group held for `MergeMaxOverlapFrames` (20) frames without a resolvable split has its held frames cleared in that camera; the one-camera rule then stops those markers. A held point is on the right epipolar line but not at the right place along it, and markers 8, 11, 13 and 14 showed 3D distances to 6 and 7 drifting from 2.7 to 4.6 cm while held for 85 frames.

Results: verified rat range 301-450 unchanged at 79% tracked, 29 wrong tracked points, 95th-percentile error 1.0 px; pig unchanged. From frame 441 to 545, 17 markers start, 10 remain at 450, and from 480 only 6 and 7 survive in both cameras; their 3D separation stays at 0.731 ± 0.002 cm against a reference of 0.733 ± 0.006 cm for 100 frames of large head motion, so they are genuine. Every other marker stopped with a logged reason (no marker-sized blob, cameras disagree, lagged behind the others, overlap unresolved, one camera only). The recovery jump bound is now `max(3r, 1.5 x the motion seen elsewhere + 3 px)`.

## 16. Appearance-first doubt and clearing the unconfirmed window (2026-10-15)

The user reported marker 15 at frames 73-78: both trackers lost it at 74, the hold sat on a line through the wrong camera's point, and the stop at 77 left frames 74-76 wrong. Reproduced with the driver from frame 61. Two rule changes:

- When the cameras disagree, the camera to doubt is first the one whose blob is not marker-sized (outside 0.7r to 2r), then the one that moved most. If no camera shows a marker-sized blob, the marker stops at once instead of holding against a point that is not a marker. At frame 74 camera 1 sat on a 1.4 px speck; it is now the one held, camera 2 stays correct, and camera 1 recovers at frame 80 after six held frames at most 6 px off.
- Whenever a hold, an overlap, or a group member ends in a stop, every frame since the hold began is cleared in that camera, and with two visible cameras the other camera's tracked frames in that window go too, because nothing confirmed them. `clearWindow` does this for single holds, the overlap timeout, `stopGroup` and `dropMember`. The log line reports how many points were cleared and from which frame.

Results: rat frames 61-100 against the September 2D reference: 1341 tracked, 77 held, 102 undefined of 1520; 17 wrong tracked, 34 wrong held. Rat 301-450: 78% tracked, 23 wrong tracked (from 29), 41 wrong held (from 69), 95th-percentile error 1.0 px. Pig unchanged.

## 17. Motion-carried holds and bounded splits (2026-10-15)

The user reported marker 9 bouncing in camera 2 around frame 86 in a run from frame 1. Reproducing it needed the driver to behave like the GUI, which never re-tracks SET or MANUAL points: the driver now clears only tracker-written frames, so manual points act as anchors in both. In that run marker 9 is exact in camera 2 from 80 to 95 with the current rules; the bounce came from an earlier build. The reproduction surfaced the rest of the long tail, all fixed and measured:

- The intruder test used the 2 px split margin instead of the 4 px consistency threshold, so a genuine overlap was called a jump.
- A one-frame disagreement just over 4 px no longer causes a hold; it must exceed 8 px or repeat on the next frame.
- A recovery may not land on another tracked marker's blob.
- A fast independent marker (tongue, moving 12 px per frame against a 3 px median) is no longer flagged incoherent: a tracker jump changes the marker's own step abruptly, a tongue does not, so both conditions are required.
- With more blobs than members at a split, an ambiguous member is held rather than placed on an arbitrary blob; the provisional state is only entered when the sole ambiguity is which member is which.
- Holds are carried along with the median step of the other markers in that camera, and a recovery must reappear where that motion predicts. The coherence tolerance is `max(2r + 2, 0.5 x median + 2)`, so a marker standing still while the specimen moves is caught. Group holds take the member's tracker only when it sits where the motion predicts, and split candidates must lie within a motion-scaled reach of each member's expected position (marker 12 had been assigned a blob 99 px from its overlap).
- `MergeEpipolarThreshold` raised from 2 to 3 px: a 2.3 px margin had committed marker 16 wrongly and the calibration residual's 99th percentile is 2.7 px.

Measured, both runs keeping the user's manual points as the GUI does:

| | Plain 2D | Merge-aware |
|---|---|---|
| Rat 2-120 (Sept 2D reference): tracked / held / lost | 3531 / 0 / 794 | 3218 / 187 / 920 |
| Rat 2-120: tracked points more than 3 px wrong; 95th pct error | 146; 2.05 px | 37; 0.90 px |
| Rat 301-450 (user's verified track): tracked / held / lost | 4071 / 0 / 1538 | 4688 / 96 / 825 |
| Rat 301-450: wrong tracked; 95th pct error | 309; 11.5 px | 33; 0.77 px |
| Pig 50-77 | | all tracked, 16 held, 0 lost, median error 0 |

What remains wrong is the along-the-line family in the fast phase after frame 440 (markers 9, 10, 17 in camera 1) and a few 5 to 10 px lags, which only a better 2D tracker can remove.
