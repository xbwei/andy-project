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
- USB data cable
- [PlatformIO](https://platformio.org/)

## Build

From this directory:

```bash
pio run -e m5stack-core2
```

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

## Controls

- On the home screen, tap **Hanzi Quest** or press button A.
- During the game, tap one of the three answers.
- Press button C to return to the home screen.
- On the final screen, choose **Play Again** or **Home**.

## Project boundary

This firmware is maintained in `xbwei/andy-project` as Andy's local game.
It is intentionally independent from `lbsocial/lbsocial-local-ai`. Do not add
Mac mini connectivity, Local AI APIs, Ollama integration, cloud AI, or
network-required gameplay without an explicit new project decision.
