# Ping Pong Scorekeeper · Codex

A fully local ping pong scorekeeper MVP. No third-party scripts, cloud APIs, telemetry, or upload logic.

## Getting Started

Open the [Ping Pong Scorekeeper on GitHub Pages](https://xbwei.github.io/andy-project/games/pingpong/) in your browser.
After opening it for the first time on your phone, you can install it as a PWA by selecting "Add to Home Screen" in Safari/Chrome.

## Usage

1. Mount your phone horizontally, ensuring the entire table and landing areas for both players are visible.
2. Tap the green "Start Match" button on the right. Allow camera and microphone access on first use.
3. If the dashed quadrilateral doesn't align with the table, tap `Set 4 Table Corners`, and tap the four corners of the table on the screen in any order (the system will sort them automatically). The four points are saved in the current mobile browser, so you don't need to reset them if the camera position remains unchanged.
4. A rally is triggered only after at least four consecutive hits spanning more than 1.8 seconds. A candidate score is automatically suggested about 1.5 seconds after hitting stops.
5. "Start Match" will change to "Pause Match". "Close Camera" will stop both the match and the camera.
6. In case of a misjudgment, tap "Undo" immediately, or manually score by directly tapping the large score areas for either side.

`Camera: Rear / Camera: Front` switches between front and rear cameras. Cameras cannot be switched during recording.

Tap `Record Match` to start recording camera video and live audio; tap again to stop. Once processing is complete, tap `Save Recording` to save it to your phone. The recording does not include the scoring interface overlay.

Scoring follows the 11-point system (must win by 2 points). Serves switch every 2 points, and every 1 point after a 10:10 tie.

## Privacy

- Local videos are handled using `URL.createObjectURL` and are never sent to a server.
- Camera frames are only analyzed locally within the page's Canvas.
- The microphone is only used for instantaneous hit detection in the browser; audio is not recorded or saved.
- Video is only recorded when the user explicitly taps `Record Match`. The recording is held in the phone's memory and saved locally only after tapping `Save Recording`.
- The page has no external dependencies, cloud voice recognition, network requests, upload, or sharing features.

## Current Limitations

The `BALL?` circle on the screen only indicates the candidate ball position currently detected by the system. It is not a button and does not mean a score is confirmed. The visual judgment is an experimental aid; occlusions, motion blur, lighting changes, and balls going out of bounds can all cause misjudgments.
