#include "hanzi_game.h"

#include <M5Unified.h>

namespace {  // Local game state

struct Question {
  const char* hanzi;
  const char* pinyin;
  const char* choices[3];
  uint8_t correctChoice;
};

constexpr Question QUESTIONS[] = {
  {"山", "shan", {"mountain", "water", "tree"}, 0},
  {"水", "shui", {"fire", "water", "moon"}, 1},
  {"火", "huo", {"person", "fire", "sun"}, 1},
  {"木", "mu", {"tree", "mouth", "big"}, 0},
  {"日", "ri", {"moon", "small", "sun"}, 2},
  {"月", "yue", {"water", "moon", "mountain"}, 1},
  {"人", "ren", {"person", "tree", "mouth"}, 0},
  {"口", "kou", {"big", "sun", "mouth"}, 2},
  {"大", "da", {"small", "big", "fire"}, 1},
  {"小", "xiao", {"mountain", "small", "person"}, 1},
};

constexpr size_t QUESTION_COUNT = sizeof(QUESTIONS) / sizeof(QUESTIONS[0]);
constexpr int OPTION_Y = 145;
constexpr int OPTION_W = 96;
constexpr int OPTION_H = 50;
constexpr int OPTION_GAP = 8;

size_t currentQuestion = 0;
int score = 0;
int streak = 0;
bool missedCurrentQuestion = false;
bool finished = false;
unsigned long stateStartedAt = 0;

uint16_t backgroundColor;
uint16_t panelColor;
uint16_t panelShadowColor;
uint16_t cyanColor;
uint16_t purpleColor;
uint16_t greenColor;
uint16_t orangeColor;

void useEnglishFont() {
  M5.Display.setFont(&fonts::Font0);
  M5.Display.setTextSize(1);
}

void drawCentered(const char* text, int x, int y, int textSize, uint16_t color) {
  useEnglishFont();
  M5.Display.setTextDatum(middle_center);
  M5.Display.setTextSize(textSize);
  M5.Display.setTextColor(color);
  M5.Display.drawString(text, x, y);
  M5.Display.setTextDatum(top_left);
}

void drawPixelDecor() {
  const uint16_t colors[] = {cyanColor, purpleColor, greenColor, orangeColor};
  for (int i = 0; i < 8; ++i) {
    int x = 6 + i * 13;
    int y = 34 + (i % 3) * 7;
    M5.Display.fillRect(x, y, 6, 6, colors[i % 4]);
  }
  for (int i = 0; i < 7; ++i) {
    int x = 226 + i * 13;
    int y = 34 + ((i + 1) % 3) * 7;
    M5.Display.fillRect(x, y, 6, 6, colors[(i + 2) % 4]);
  }
}

void drawTopBar() {
  M5.Display.fillRect(0, 0, 320, 30, panelShadowColor);
  drawCentered("HANZI QUEST", 68, 15, 1, WHITE);

  char progress[20];
  snprintf(
    progress,
    sizeof(progress),
    "%u/%u",
    static_cast<unsigned>(currentQuestion + 1),
    static_cast<unsigned>(QUESTION_COUNT)
  );
  drawCentered(progress, 164, 15, 1, cyanColor);

  char scoreText[28];
  snprintf(scoreText, sizeof(scoreText), "SCORE %d", score);
  drawCentered(scoreText, 270, 15, 1, greenColor);
}

void drawOption(size_t index, uint16_t fillColor = 0, uint16_t borderColor = 0) {
  const Question& question = QUESTIONS[currentQuestion];
  int x = OPTION_GAP + static_cast<int>(index) * (OPTION_W + OPTION_GAP);
  uint16_t fill = fillColor == 0 ? panelColor : fillColor;
  uint16_t border = borderColor == 0 ? cyanColor : borderColor;

  M5.Display.fillRoundRect(x + 3, OPTION_Y + 4, OPTION_W, OPTION_H, 7, panelShadowColor);
  M5.Display.fillRoundRect(x, OPTION_Y, OPTION_W, OPTION_H, 7, fill);
  M5.Display.drawRoundRect(x, OPTION_Y, OPTION_W, OPTION_H, 7, border);
  drawCentered(question.choices[index], x + OPTION_W / 2, OPTION_Y + OPTION_H / 2, 1, WHITE);
}

void drawQuestion() {
  M5.Display.fillScreen(backgroundColor);
  drawTopBar();
  drawPixelDecor();

  const Question& question = QUESTIONS[currentQuestion];

  M5.Display.fillRoundRect(112, 48, 104, 84, 9, panelShadowColor);
  M5.Display.fillRoundRect(108, 44, 104, 84, 9, panelColor);
  M5.Display.drawRoundRect(108, 44, 104, 84, 9, purpleColor);

  M5.Display.setFont(&fonts::efontCN_24);
  M5.Display.setTextSize(3);
  M5.Display.setTextColor(WHITE);
  M5.Display.setTextDatum(middle_center);
  M5.Display.drawString(question.hanzi, 160, 84);
  M5.Display.setTextDatum(top_left);

  drawCentered("Tap the meaning", 160, 136, 1, cyanColor);
  for (size_t i = 0; i < 3; ++i) {
    drawOption(i);
  }

  M5.Display.fillRect(0, 205, 320, 35, panelShadowColor);
  char comboText[24];
  snprintf(comboText, sizeof(comboText), "COMBO x%d", streak);
  drawCentered(comboText, 72, 222, 1, orangeColor);
  drawCentered("C: HOME", 260, 222, 1, WHITE);
  stateStartedAt = millis();
}

void drawFeedbackBanner(const char* title, const char* detail, uint16_t color) {
  M5.Display.fillRoundRect(45, 91, 230, 46, 8, panelShadowColor);
  M5.Display.fillRoundRect(42, 87, 230, 46, 8, color);
  drawCentered(title, 157, 99, 2, WHITE);
  drawCentered(detail, 157, 121, 1, WHITE);
}

void drawCelebrationPixels() {
  const int positions[][2] = {
    {18, 51}, {40, 73}, {72, 45}, {242, 47}, {274, 72}, {300, 50},
    {25, 115}, {289, 113}, {64, 126}, {250, 126},
  };
  const uint16_t colors[] = {cyanColor, purpleColor, greenColor, orangeColor};
  for (size_t i = 0; i < sizeof(positions) / sizeof(positions[0]); ++i) {
    M5.Display.fillRect(positions[i][0], positions[i][1], 8, 8, colors[i % 4]);
  }
}

bool waitForHomeRequest(unsigned long durationMs) {
  unsigned long startedAt = millis();
  while (millis() - startedAt < durationMs) {
    M5.update();
    if (M5.BtnC.wasPressed()) {
      return true;
    }
    delay(10);
  }
  return false;
}

bool correctFeedback(size_t choice) {
  if (missedCurrentQuestion) {
    streak = 0;
  } else {
    ++streak;
  }
  int earned = missedCurrentQuestion ? 50 : 100 + (streak - 1) * 20;
  score += earned;

  drawOption(choice, greenColor, WHITE);
  char detail[64];
  snprintf(
    detail,
    sizeof(detail),
    "%s = %s   +%d",
    QUESTIONS[currentQuestion].pinyin,
    QUESTIONS[currentQuestion].choices[QUESTIONS[currentQuestion].correctChoice],
    earned
  );
  drawFeedbackBanner("NICE!", detail, greenColor);
  drawCelebrationPixels();

  M5.Power.setVibration(180);
  M5.Speaker.tone(660, 90);
  if (waitForHomeRequest(90)) {
    M5.Power.setVibration(0);
    return true;
  }
  M5.Speaker.tone(880, 130);
  if (waitForHomeRequest(50)) {
    M5.Power.setVibration(0);
    return true;
  }
  M5.Power.setVibration(0);
  if (waitForHomeRequest(500)) {
    return true;
  }

  ++currentQuestion;
  missedCurrentQuestion = false;
  if (currentQuestion >= QUESTION_COUNT) {
    finished = true;
  } else {
    drawQuestion();
  }
  return false;
}

bool wrongFeedback(size_t choice) {
  streak = 0;
  missedCurrentQuestion = true;
  drawOption(choice, orangeColor, WHITE);
  drawFeedbackBanner("TRY AGAIN", "Look closely and tap again", orangeColor);

  M5.Power.setVibration(150);
  M5.Speaker.tone(260, 180);
  if (waitForHomeRequest(90)) {
    M5.Power.setVibration(0);
    return true;
  }
  M5.Power.setVibration(0);
  if (waitForHomeRequest(70)) {
    return true;
  }
  M5.Power.setVibration(120);
  if (waitForHomeRequest(70)) {
    M5.Power.setVibration(0);
    return true;
  }
  M5.Power.setVibration(0);
  if (waitForHomeRequest(420)) {
    return true;
  }
  drawQuestion();
  return false;
}

void drawFinalScreen() {
  M5.Display.fillScreen(backgroundColor);
  drawPixelDecor();
  drawCentered("QUEST COMPLETE!", 160, 50, 2, greenColor);

  char scoreText[32];
  snprintf(scoreText, sizeof(scoreText), "%d POINTS", score);
  drawCentered(scoreText, 160, 88, 3, WHITE);

  const char* message = score >= 1250
    ? "Pixel-perfect mastery!"
    : score >= 1050 ? "Great rhythm!" : "Every retry builds skill.";
  drawCentered(message, 160, 118, 1, cyanColor);

  M5.Display.fillRoundRect(23, 145, 126, 48, 8, panelColor);
  M5.Display.drawRoundRect(23, 145, 126, 48, 8, purpleColor);
  drawCentered("PLAY AGAIN", 86, 169, 1, WHITE);

  M5.Display.fillRoundRect(171, 145, 126, 48, 8, panelColor);
  M5.Display.drawRoundRect(171, 145, 126, 48, 8, cyanColor);
  drawCentered("HOME", 234, 169, 1, WHITE);

  M5.Display.fillRect(0, 205, 320, 35, panelShadowColor);
  drawCentered("Touch a tile or press C for Home", 160, 222, 1, WHITE);
}

bool pointInside(int x, int y, int left, int top, int width, int height) {
  return x >= left && x < left + width && y >= top && y < top + height;
}

}  // namespace

namespace HanziGame {

void start() {
  backgroundColor = M5.Display.color565(8, 12, 28);
  panelColor = M5.Display.color565(24, 35, 64);
  panelShadowColor = M5.Display.color565(3, 7, 18);
  cyanColor = M5.Display.color565(33, 205, 224);
  purpleColor = M5.Display.color565(150, 86, 255);
  greenColor = M5.Display.color565(45, 204, 112);
  orangeColor = M5.Display.color565(242, 126, 45);

  currentQuestion = 0;
  score = 0;
  streak = 0;
  missedCurrentQuestion = false;
  finished = false;
  stateStartedAt = millis();
  drawQuestion();
}

HanziGameAction update() {
  if (millis() - stateStartedAt < 250) {
    return HanziGameAction::None;
  }

  if (M5.Touch.getCount() == 0) {
    return HanziGameAction::None;
  }

  const auto& touch = M5.Touch.getDetail();
  if (!touch.wasPressed()) {
    return HanziGameAction::None;
  }

  if (finished) {
    if (pointInside(touch.x, touch.y, 23, 145, 126, 48)) {
      start();
    } else if (pointInside(touch.x, touch.y, 171, 145, 126, 48)) {
      return HanziGameAction::Home;
    }
    return HanziGameAction::None;
  }

  if (touch.y >= OPTION_Y && touch.y < OPTION_Y + OPTION_H) {
    for (size_t i = 0; i < 3; ++i) {
      int x = OPTION_GAP + static_cast<int>(i) * (OPTION_W + OPTION_GAP);
      if (pointInside(touch.x, touch.y, x, OPTION_Y, OPTION_W, OPTION_H)) {
        if (i == QUESTIONS[currentQuestion].correctChoice) {
          if (correctFeedback(i)) {
            return HanziGameAction::Home;
          }
          if (finished) {
            drawFinalScreen();
            stateStartedAt = millis();
          }
        } else {
          if (wrongFeedback(i)) {
            return HanziGameAction::Home;
          }
        }
        break;
      }
    }
  }

  return HanziGameAction::None;
}

}  // namespace HanziGame
