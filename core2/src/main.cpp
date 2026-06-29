#include <M5Unified.h>
#include <SPI.h>
#include <SD.h>

#include "hanzi_game.h"
#include "defense_game.h"

namespace {

enum class AppMode {
  Home,
  Hanzi,
  Defense,
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

  drawCentered("GAMES", 160, 48, 2, cyan);

  // Card 1: Hanzi Quest
  M5.Display.fillRoundRect(24, 68, 272, 48, 8, BLACK);
  M5.Display.fillRoundRect(22, 66, 272, 48, 8, panel);
  M5.Display.drawRoundRect(22, 66, 272, 48, 8, WHITE);
  drawCentered("HANZI QUEST", 158, 82, 2, WHITE);
  drawCentered("Tap to play", 158, 100, 1, WHITE);

  // Card 2: Base Defense
  const uint16_t defPanel = M5.Display.color565(160, 55, 35);
  M5.Display.fillRoundRect(24, 126, 272, 48, 8, BLACK);
  M5.Display.fillRoundRect(22, 124, 272, 48, 8, defPanel);
  M5.Display.drawRoundRect(22, 124, 272, 48, 8, WHITE);
  drawCentered("BASE DEFENSE", 158, 140, 2, WHITE);
  drawCentered("Tap to play", 158, 158, 1, WHITE);

  M5.Display.fillRect(0, 205, 320, 35, M5.Display.color565(24, 35, 64));
  drawCentered("Touch a game card to play!", 160, 222, 1, WHITE);
}

void startHanzi() {
  appMode = AppMode::Hanzi;
  HanziGame::start();
}

void startDefense() {
  appMode = AppMode::Defense;
  DefenseGame::start();
}

void handleSerialTransfer() {
  if (Serial.available()) {
    String cmd = Serial.readStringUntil('\n');
    if (cmd.startsWith("TRANS:")) {
      int firstColon = cmd.indexOf(':');
      int secondColon = cmd.indexOf(':', firstColon + 1);
      if (firstColon != -1 && secondColon != -1) {
        String filename = cmd.substring(firstColon + 1, secondColon);
        long fileSize = cmd.substring(secondColon + 1).toInt();
        
        M5.Display.fillScreen(BLACK);
        drawCentered("FILE SYNC ACTIVE", 160, 80, 2, YELLOW);
        drawCentered(filename.c_str(), 160, 120, 1, WHITE);
        
        Serial.printf("READY:%s\n", filename.c_str());
        
        if (!SD.exists("/hanzi")) {
          SD.mkdir("/hanzi");
        }
        String path = "/hanzi/" + filename;
        File file = SD.open(path, FILE_WRITE);
        if (!file) {
          Serial.println("ERROR:Failed to open file on SD card");
          showHome();
          return;
        }
        
        long bytesReceived = 0;
        unsigned long lastPacketTime = millis();
        
        while (bytesReceived < fileSize && millis() - lastPacketTime < 10000) {
          if (Serial.available()) {
            uint8_t buf[512];
            size_t toRead = min((long)sizeof(buf), fileSize - bytesReceived);
            size_t readBytes = Serial.readBytes(buf, toRead);
            if (readBytes > 0) {
              file.write(buf, readBytes);
              bytesReceived += readBytes;
              lastPacketTime = millis();
              
              int barWidth = (bytesReceived * 200) / fileSize;
              M5.Display.fillRect(60, 140, 200, 10, BLACK);
              M5.Display.fillRect(60, 140, barWidth, 10, GREEN);
              M5.Display.drawRoundRect(60, 140, 200, 10, 2, WHITE);
            }
          }
          delay(1);
        }
        file.close();
        
        if (bytesReceived == fileSize) {
          Serial.println("OK:Success");
        } else {
          Serial.println("ERROR:Timeout");
        }
        
        showHome();
      }
    }
  }
}

}  // namespace

void setup() {
  auto config = M5.config();
  config.serial_baudrate = 115200;
  M5.begin(config);

  // Initialize micro SD card (Core2 uses CS GPIO 4)
  if (!SD.begin(GPIO_NUM_4, SPI, 25000000)) {
    Serial.println("SD Card mount failed! Game will play only in beep/buzzer mode.");
  } else {
    Serial.println("SD Card mounted successfully!");
  }

  M5.Speaker.begin();
  M5.Speaker.setVolume(160);
  M5.Display.setRotation(1);
  M5.Display.setTextWrap(true, true);

  showHome();
  lastBatteryLevel = M5.Power.getBatteryLevel();
  lastBatteryRefresh = millis();
}

void loop() {
  handleSerialTransfer();
  M5.update();

  if (appMode == AppMode::Home) {
    if (M5.Touch.getCount() > 0) {
      const auto& touch = M5.Touch.getDetail();
      if (touch.wasPressed()) {
        if (pointInside(touch.x, touch.y, 22, 66, 272, 48)) {
          startHanzi();
        } else if (pointInside(touch.x, touch.y, 22, 124, 272, 48)) {
          startDefense();
        }
      }
    }
  } else if (appMode == AppMode::Hanzi) {
    if (M5.BtnC.wasPressed() || HanziGame::update() == HanziGameAction::Home) {
      showHome();
    }
  } else if (appMode == AppMode::Defense) {
    if (M5.BtnC.wasPressed() || DefenseGame::update() == DefenseGameAction::Home) {
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
