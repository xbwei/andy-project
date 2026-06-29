#include <M5Unified.h>

#include "hanzi_game.h"

namespace {

enum class AppMode {
  Home,
  Hanzi,
};

AppMode appMode = AppMode::Home;
unsigned long lastBatteryRefresh = 0;
int lastBatteryLevel = -1;

constexpr unsigned long BATTERY_REFRESH_INTERVAL = 5000;

bool pointInside(int x, int y, int left, int top, int width, int height) {
  return x >= left && x < left + width && y >= top && y < top + height;
}

void drawCentered(const char* text, int x, int y, int textSize, uint16_t color) {
  M5.Display.setFont(&fonts::Font0);
  M5.Display.setTextDatum(middle_center);
  M5.Display.setTextSize(textSize);
  M5.Display.setTextColor(color);
  M5.Display.drawString(text, x, y);
  M5.Display.setTextDatum(top_left);
}

void drawHeader() {
  M5.Display.fillRect(0, 0, 320, 30, BLACK);
  M5.Display.setFont(&fonts::Font0);
  M5.Display.setTextSize(1);
  M5.Display.setTextColor(WHITE);
  M5.Display.setCursor(8, 10);
  M5.Display.print("ANDY'S CORE2");

  M5.Display.setCursor(260, 10);
  M5.Display.printf("%d%%", M5.Power.getBatteryLevel());
}

void showHome() {
  appMode = AppMode::Home;

  const uint16_t background = M5.Display.color565(8, 12, 28);
  const uint16_t panel = M5.Display.color565(21, 139, 103);
  const uint16_t cyan = M5.Display.color565(33, 205, 224);

  M5.Display.fillScreen(background);
  drawHeader();

  drawCentered("LOCAL GAME", 160, 52, 2, cyan);
  drawCentered("Works offline - no Wi-Fi or AI", 160, 77, 1, WHITE);

  M5.Display.fillRoundRect(63, 101, 200, 75, 10, BLACK);
  M5.Display.fillRoundRect(59, 97, 200, 75, 10, panel);
  M5.Display.drawRoundRect(59, 97, 200, 75, 10, WHITE);
  drawCentered("HANZI QUEST", 159, 123, 2, WHITE);
  drawCentered("Tap to play", 159, 151, 1, WHITE);

  M5.Display.fillRect(0, 205, 320, 35, M5.Display.color565(24, 35, 64));
  drawCentered("A: PLAY", 63, 222, 1, WHITE);
  drawCentered("Touch the game card", 207, 222, 1, WHITE);
}

void startGame() {
  appMode = AppMode::Hanzi;
  HanziGame::start();
}

}  // namespace

void setup() {
  auto config = M5.config();
  config.serial_baudrate = 115200;
  M5.begin(config);

  M5.Speaker.begin();
  M5.Speaker.setVolume(160);
  M5.Display.setRotation(1);
  M5.Display.setTextWrap(true, true);

  showHome();
  lastBatteryLevel = M5.Power.getBatteryLevel();
  lastBatteryRefresh = millis();
}

void loop() {
  M5.update();

  if (appMode == AppMode::Home) {
    bool playRequested = M5.BtnA.wasPressed();
    if (M5.Touch.getCount() > 0) {
      const auto& touch = M5.Touch.getDetail();
      playRequested = playRequested ||
        (touch.wasPressed() && pointInside(touch.x, touch.y, 59, 97, 200, 75));
    }
    if (playRequested) {
      startGame();
    }
  } else if (appMode == AppMode::Hanzi) {
    if (M5.BtnC.wasPressed() || HanziGame::update() == HanziGameAction::Home) {
      showHome();
    }
  }

  if (appMode == AppMode::Home &&
      millis() - lastBatteryRefresh > BATTERY_REFRESH_INTERVAL) {
    lastBatteryRefresh = millis();
    const int currentBatteryLevel = M5.Power.getBatteryLevel();
    if (currentBatteryLevel != lastBatteryLevel) {
      lastBatteryLevel = currentBatteryLevel;
      drawHeader();
    }
  }

  delay(20);
}
