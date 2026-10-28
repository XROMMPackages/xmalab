# Merge-aware tracking, in plain English

This is what the "merge-aware" buttons do, written without jargon. The technical record is in `merge-aware-tracking-design.md`; this page is the version to read first.

## The one-sentence version

The normal 2D tracker still does all the tracking. After every frame, a checker looks at what the tracker just wrote and asks one question for each marker: **"is this point definitely the marker?"** If yes, it stays as a normal tracked point. If it is not sure, the point is kept as a guess (orange) and the tracker is told to try again next frame from there. If it cannot become sure within a few frames, the guess and everything that depended on it are deleted, and the marker stops. Nothing is ever written as "tracked" while the checker is unsure.

## What the checker uses

It has four things to go on, in rough order of how much they are trusted.

1. **The other camera.** A point in camera 1 pins the marker down to a line in camera 2 (the blue epipolar line you can draw in XMALab). If camera 2's point is off that line by more than 4 px, the two cameras disagree and one of them is wrong. On your rat data, a correct pair never disagrees by more than about 3 px, so 4 px really does mean something went wrong. The limit of this: a point that slides *along* the line looks fine to the other camera. In your lateral camera the lines run up and down, so a marker that lags behind a vertical jaw movement is invisible to this check.

2. **The blob.** A real marker shows up as a small dark blob of a consistent size. Background, bone edges and two markers on top of each other do not. The checker looks at the size of the blob the detector found. One odd frame is ignored, because sizes are noisy; three odd frames out of the last five mean the tracker is not on a marker.

3. **The other markers.** Everything on the skull moves together. If all the other markers in a camera moved 8 px down and this one did not move, or moved somewhere else entirely, it has probably lost its blob. A marker that genuinely moves on its own (tongue, jaw) is allowed to disagree with the crowd as long as it does so smoothly; a tracker jumping onto something else changes direction suddenly, and that is what gets flagged.

4. **The rigid bodies, if you have defined them.** If a marker belongs to a rigid body and three or more of the body's other markers are tracked, the body's pose says where this marker must be, to within a pixel or two. This is the strongest cue of all, but the rat project has no bodies defined, so it only helps on the pig.

## What happens to a doubtful point

A doubtful point is not deleted straight away. It is **held**: moved onto the epipolar line from the other camera, carried along with the movement of the other markers, and written in orange (status PREDICTED). Next frame the tracker searches from there, using a template of the marker from the last frame where it was clearly a single blob.

If the tracker comes back somewhere believable, meaning on a marker-sized blob, roughly where the movement of the other markers says it should be, not on another marker's blob, and agreeing with the other camera, the hold ends and normal tracking resumes. The orange frames stay orange, so you can see where the tracker was guessing.

If it does not come back believably within 10 frames, the marker is stopped in that camera. Every frame since the hold started is deleted, in both cameras, because nothing confirmed them. The console says why, and from which frame.

A marker that is left with only one camera is stopped too, because nothing can check it. Your manually set points are never touched.

## What happens when two markers overlap

When two trackers land on the same blob, the checker first asks whether this is a real overlap or whether one tracker simply jumped onto the other marker's blob. A jump is a sudden move, or one marker disagreeing with its own other camera while the other marker agrees. A jumper is held on its own; the resident marker carries on untouched. That is what was happening to marker 5: marker 14 jumped onto it.

A real overlap becomes a group. While the blobs are merged, both markers are held on their own epipolar lines, so each stays where its other camera says it must be, and their templates are frozen at the last clean frame. Each frame the checker looks for separate blobs around the group, using the detector on everything nearby, not only where the trackers are.

When separate blobs appear, each marker is matched to the blob that fits its line (and its rigid body, if any). If the fit is clear, more than 3 px better than the alternative, the split is accepted and the markers are tracked again. If it is close, the checker picks the better option but keeps both as orange guesses, watches the next few frames, and only commits once the gap has grown; if it turns out the two were the wrong way round, the orange frames are swapped. If after 10 frames it still cannot tell, or the pair overlaps again first, the guessed frames are deleted. An overlap that has not resolved after 20 frames is given up in that camera.

A marker that comes out of an overlap onto a blob that is too far from where it could plausibly be is not accepted, even if the blob fits its line.

## What you will see on screen

- **Normal colour:** a tracked point the checker is happy with, in both cameras.
- **Orange:** a guess. The checker thinks the marker is about here but could not confirm it. Treat these as "look at me".
- **Gap:** the checker gave up. The last few frames before the gap were deleted because they were never confirmed. Your earlier points after the gap are untouched.
- **Console:** one line per decision, with the frame, the camera, the marker and the reason. If a marker is held or stopped and it looks fine to you, that line is what I need.

## What it is not

It does not track in 3D and it does not search a volume. The old volumetric tracker did that, and on this data its search step was 35 to 50 px, which is why it drifted. This replaces it entirely.

It does not make the 2D tracker better at following a fast marker. When the marker runs along the epipolar line in the fast chewing frames, the 2D tracker can lag behind and the checker cannot see it, because the other camera agrees and the blob looks right. That is the main thing still wrong, and it lives in the 2D tracker, not here.

## How it was tested

`XMALab --merge-test project.xma start end output.xma` runs the whole thing without the GUI from a given frame, keeping your manually set points, and saves a new project plus a log. A script then compares the output to a reference track frame by frame. Over the 150 verified rat frames, plain 2D tracking writes 309 points that are more than 3 px off; merge-aware writes 33, with more frames tracked, and the typical error is under a pixel. On the pig nothing is lost and the one overlap resolves correctly.

## Settings

All in pixels, in the settings file, with no dialog entry yet:

- `MergeAuditThreshold` 4: how far the cameras may disagree.
- `MergeEpipolarThreshold` 3: how clear a split must be to accept it.
- `MergeMaxHoldFrames` 10: how long a doubtful point is held before giving up.
- `MergeMaxPendingFrames` 10: how long an uncertain split is watched.
- `MergeMaxOverlapFrames` 20: how long an overlap is held before giving up.
