# M5Stack Core2: Educational Coding Project

A collection of interactive games designed for the M5Stack Core2 ESP32 development kit. The games feature offline touch controls, real audio playback, and vibration feedback.

> [!NOTE]
> **Co-Creation Project**: This is an open-source educational hobby project. We are **not selling any products or hardware**. All code here is for personal fun and learning.

## 📂 Future Multi-Game Architecture
To support adding other types of learning games in the future (e.g. math, spelling, puzzles), the repository and the SD card structure are organized into self-contained subfolders per game:
* `/hanzi/` — Holds the dynamic questions configuration and voice audio files for the Chinese Hanzi learning game.
* `/math/` (Future) — Will hold configurations for arithmetic quizzes.
* `/spelling/` (Future) — Will hold spelling cards.

---

## 🛒 Hardware Requirements & Buying Guide

To run this project, you will need the following hardware:

1. **M5Stack Core2 IoT Development Kit**:
   * Official Store: [M5Stack Core2 ESP32 Kit](https://shop.m5stack.com/products/m5stack-core2-esp32-iot-development-kit)
   * Other distributors: AliExpress (search "M5Stack Core2"), Mouser, or DigiKey.
2. **MicroSD Card**:
   * Any standard microSD card formatted to **FAT32** (16GB or smaller recommended).
3. **USB Type-C Cable**:
   * Included with the M5Stack Core2 kit, used for firmware flashing and serial sync.

---

## 🚀 Deployment Guide

### Option A: Assisted Deployment (Using AI Coding Assistants / Engines) [RECOMMENDED]

If you are developing or deploying with the help of an AI Coding Assistant or Coding Engine (such as Google Antigravity, Anthropic Claude Code, GitHub Copilot, OpenAI Codex, or others):

> [!WARNING]
> **Accuracy is not guaranteed**: AI coding assistants may generate code errors, introduce bugs, or hallucinate syntax. Always review proposed plans, inspect generated code changes, and verify before deploying onto physical hardware.

1. **Connect the Device**:
   * Connect your M5Stack Core2 to your PC via USB-C.
2. **Command the Assistant**:
   * Simply ask your AI assistant in chat to compile the PlatformIO project and sync the assets (e.g., *"Sync my local vocabulary and compile the firmware"*).
   * The AI coding assistant can run the build, flash the firmware to your device, and invoke the Python synchronizer script to transfer files to the SD card.
3. **Manual Synchronizer Utility**:
   * You can manually trigger the USB sync script from your computer's terminal:
     ```bash
     python sync_assets.py
     ```
   * Ensure your private questions are saved in `core2/data/hanzi/questions.csv` and audio files in `core2/data/hanzi/` before running.

---

### Option B: Manual Deployment (No AI Assistant)

If you are a standard developer compiling this project manually:

1. **Setup Development Environment**:
   * Install [VS Code](https://code.visualstudio.com/).
   * Install the **PlatformIO IDE** extension inside VS Code.
2. **Open the Project**:
   * Open the `core2` folder in VS Code.
3. **Prepare the SD Card**:
   * Copy the public template [`data/hanzi/questions.csv.template`](./data/hanzi/questions.csv.template) and rename it to `questions.csv`.
   * Create a folder named `hanzi` in the root of your microSD card.
   * Copy your `questions.csv` into the `hanzi` folder on the card.
   * Put your custom `.wav` voice files (16000Hz, 16-bit, mono PCM WAV format) into the `hanzi` folder on the card, matching the pinyin filenames (e.g. `shan.wav`, `shui.wav`).
   * Insert the microSD card into the slot on the M5Stack Core2.
4. **Compile & Upload**:
   * Connect the M5Stack Core2 to your PC via the USB-C cable.
   * Click the **PlatformIO: Upload** arrow icon in the bottom status bar (or run `pio run -t upload` in the terminal).
   * The device will reboot and launch the game.

---

## 📄 License & Disclaimer

This project is licensed under the **Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International (CC BY-NC-SA 4.0)** license.

### ⚠️ Liability Disclaimer:
* **USE AT YOUR OWN RISK**: Flashing firmware or modifications are done entirely at your own risk.
* **NO WARRANTY**: The authors and contributors assume no liability for hardware damage (such as screen burn-in, bricked devices, or speaker wear), battery issues, or personal data loss.
* **NON-COMMERCIAL**: This project is strictly for personal educational use and may not be used for commercial purposes.
