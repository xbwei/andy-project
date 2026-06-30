# Agent Instructions - Core2 Offline Game

## Scope

This directory is the complete PlatformIO project for Andy's M5Stack Core2
game. It must remain a standalone, deterministic game that works without a
network or another computer.

## Hard boundaries

- Do not add Wi-Fi, Bluetooth networking, HTTP, remote APIs, telemetry,
  accounts, analytics, or AI features.
- Do not connect to a Mac mini, Ollama, `lbsocial-local-ai`, or cloud services.
- Do not create `secrets.h`, `.env`, credential, server URL, or model
  configuration files.
- Do not commit `.pio/`, `.vscode/`, firmware binaries, serial logs, or local
  editor files.
- Do not upload to a physical Core2 unless the user explicitly requests it.

## Read before editing

1. `README.md` for setup, controls, and troubleshooting.
2. `platformio.ini` for the board and dependency configuration.
3. `src/main.cpp` for startup, home-screen, and mode navigation.
4. `src/hanzi_game.cpp` for questions, scoring, touch UI, sound, and vibration.
5. `src/hanzi_game.h` for the small game interface.

## Code map

- `src/main.cpp`: initializes M5Unified, draws the home screen, starts the game,
  refreshes battery status, and returns from the game to Home.
- `src/hanzi_game.cpp`: owns the built-in question data and all Hanzi Quest
  gameplay.
- `src/hanzi_game.h`: exposes `start()` and `update()`.
- `platformio.ini`: declares the `m5stack-core2` environment and M5Unified
  dependency.

Keep new games in separate `.cpp` and `.h` files. Let `main.cpp` select modes;
do not turn it into one large file.

## Editing rules

- Keep all gameplay data compiled into the firmware.
- Keep touch targets large enough for the 320 x 240 display.
- Call `M5.update()` frequently in the main loop and inside any blocking wait
  so buttons and touch remain responsive.
- Every timed feedback sequence must preserve a button-C path back to Home.
- Use M5Unified APIs already established in the project.
- Keep UI text short enough for the display and verify long strings visually
  on hardware when practical.
- In `Question.correctChoice`, use a zero-based choice index: `0`, `1`, or `2`.

## Required verification

From this directory, run:

```bash
pio run -e m5stack-core2
git diff --check
```

Also confirm that source and configuration files did not gain network or
secret-related code:

```bash
rg -n -i \
  'WiFi|HTTPClient|ArduinoJson|Ollama|Local AI|Mac mini|api[_-]?key|password|secret' \
  src platformio.ini
```

Expected result: no matches.

If a connected device deployment is explicitly requested:

```bash
pio device list
pio run -e m5stack-core2 -t upload --upload-port <serial-port>
```

After upload, smoke-test Home, one correct answer, one wrong answer, button C,
the completion screen, sound, and vibration.

## Git workflow

- Check `git status --short --branch` before editing.
- Keep changes within `games/core2/` unless the repository README also needs a short
  project-list update.
- Review generated diffs and ensure `.pio/` is not staged.
- Follow the workflow explicitly requested by the user; do not assume a PR or
  direct-`main` push.
