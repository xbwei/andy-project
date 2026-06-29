# Core2 Offline Game

This directory contains Andy's standalone game firmware for the M5Stack Core2.
It runs entirely on the device and does not connect to Wi-Fi, a Mac mini,
Ollama, LBSocial Local AI, or any other AI service.

## Included game

**Hanzi Quest** is a touch-based Chinese character game with:

- 10 built-in Hanzi questions
- three meaning choices per question
- local scoring and combo bonuses
- on-device sound and vibration feedback
- replay support without a network connection

No gameplay data leaves the Core2, and no account, API key, server, or
configuration file is required.

## Requirements

- M5Stack Core2
- USB-C data cable (a charge-only cable will not work for flashing)
- Git
- [PlatformIO](https://platformio.org/)

## Set up a new computer

### Recommended: VS Code and PlatformIO IDE

This route works on macOS, Windows, and Linux.

1. Install [Git](https://git-scm.com/downloads) and confirm that
   `git --version` works.
2. Install [Visual Studio Code](https://code.visualstudio.com/).
3. In VS Code Extensions, install the official
   [PlatformIO IDE extension](https://docs.platformio.org/en/stable/integration/ide/vscode.html).
   It includes PlatformIO Core, so a separate CLI installation is not needed.
4. Clone the repository:

   ```bash
   git clone https://github.com/xbwei/andy-project.git
   cd andy-project/core2
   ```

5. Open the `core2` directory in VS Code. It must be the opened project folder
   because it contains `platformio.ini`.
6. Connect the Core2 with a USB-C data cable.
7. Open the PlatformIO terminal and confirm:

   ```bash
   pio --version
   pio device list
   ```

PlatformIO downloads the ESP32 toolchain and M5Unified dependency during the
first build, so that first build takes longer and requires internet access.
After dependencies are installed, building and playing the game do not require
an AI service or network connection.

### CLI-only alternative

An agent or developer who does not use VS Code can install
[PlatformIO Core](https://docs.platformio.org/en/stable/core/installation/index.html)
and run the same `pio` commands from a terminal. Follow the official PlatformIO
installer for the operating system rather than adding tool binaries to this
repository.

### USB driver

If `pio device list` does not show the Core2, install the correct CP210x or
CH9102 USB driver from the
[official M5Stack Core2 page](https://docs.m5stack.com/en/core/core2).
Core2 hardware revisions may use either USB bridge.

## Build

Run from `andy-project/core2`:

```bash
pio run -e m5stack-core2
```

The successful firmware image is generated under
`.pio/build/m5stack-core2/firmware.bin`. The entire `.pio/` directory is local
build output and must not be committed.

## Upload

Connect the Core2 by USB, then run:

```bash
pio run -e m5stack-core2 -t upload
```

If PlatformIO cannot select the serial port automatically:

```bash
pio device list
pio run -e m5stack-core2 -t upload --upload-port <serial-port>
```

Uploading replaces the firmware currently installed on the device. Agents
should build freely but upload only when Xuebin or Andy explicitly requests a
physical-device deployment.

## Controls

- On the home screen, tap **Hanzi Quest** or press button A.
- During the game, tap one of the three answers.
- Press button C to return to the home screen.
- On the final screen, choose **Play Again** or **Home**.

## First-run smoke test

After an authorized upload:

1. Confirm the **Andy's Core2** home screen and battery level appear.
2. Start **Hanzi Quest** using touch and again using button A.
3. Tap a wrong answer and confirm the orange feedback, tone, and vibration.
4. Tap the correct answer and confirm score/combo progression.
5. Press button C during the game and during feedback to return Home.
6. Complete all questions and test **Play Again** and **Home**.
7. Confirm the game still works with no Wi-Fi or Mac mini available.

## Change the game

### Project files

- `src/main.cpp` owns device startup, the home screen, battery display, and
  app-mode navigation.
- `src/hanzi_game.cpp` owns Hanzi questions, scoring, touch layout, feedback,
  sound, and vibration.
- `src/hanzi_game.h` exposes the game's `start()` and `update()` functions.
- `platformio.ini` defines the Core2 board and M5Unified dependency.
- `AGENTS.md` contains mandatory rules for coding agents.

### Add or edit a Hanzi question

Edit the `QUESTIONS` array near the top of `src/hanzi_game.cpp`:

```cpp
{"山", "shan", {"mountain", "water", "tree"}, 0},
```

The fields are:

1. Hanzi displayed on the card.
2. Pinyin shown after a correct answer.
3. Exactly three short English choices.
4. Zero-based index of the correct choice: `0`, `1`, or `2`.

Keep choices short enough to fit the three 96-pixel-wide answer buttons.
`QUESTION_COUNT` updates automatically from the array size.

### Add another game

Create a separate source/header pair such as `src/math_game.cpp` and
`src/math_game.h`. Give it a small `start()`/`update()` interface similar to
Hanzi Quest, then add a new mode and home-screen tile in `src/main.cpp`.

Do not add a server, account, network sync, AI generation, or secrets file.
All required gameplay content must be compiled into the firmware.

## Development checklist

Before committing:

```bash
pio run -e m5stack-core2
git diff --check
rg -n -i \
  'WiFi|HTTPClient|ArduinoJson|Ollama|Local AI|Mac mini|api[_-]?key|password|secret' \
  src platformio.ini
```

The final command should return no matches.

Do not commit:

- `.pio/`
- `.vscode/`
- firmware binaries
- serial logs
- credentials or device-specific configuration

## Troubleshooting

| Problem | What to check |
|---------|---------------|
| `pio` is not found | Use the PlatformIO terminal in VS Code, or complete the official PlatformIO Core shell setup. |
| No serial port appears | Use a USB data cable, reconnect the Core2, and install the appropriate CP210x/CH9102 driver. |
| Upload cannot open the port | Close serial monitors and other apps using the Core2 port, then retry with `--upload-port`. |
| Upload times out | Reconnect/reset the Core2, verify the selected port, and reinstall the official USB driver if needed. |
| Build fails under `.platformio` | Check local PlatformIO permissions/cache before changing game code. |
| Touch feels unresponsive | Ensure loops and timed feedback continue calling `M5.update()`. |

## Project boundary

This firmware is maintained in `xbwei/andy-project` as Andy's local game.
It is intentionally independent from `lbsocial/lbsocial-local-ai`. Do not add
Mac mini connectivity, Local AI APIs, Ollama integration, cloud AI, or
network-required gameplay without an explicit new project decision.
