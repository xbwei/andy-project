#include "defense_game.h"

#include <M5Unified.h>
#include <cmath>
#include <cstdlib>

namespace {  // ---------- Local game state ----------

// --- Display constants ---
constexpr int SCREEN_W = 320;
constexpr int SCREEN_H = 240;
constexpr int CENTER_X = SCREEN_W / 2;
constexpr int CENTER_Y = SCREEN_H / 2;

// --- Base ---
constexpr int BASE_RADIUS = 14;
constexpr int BASE_MAX_HP = 3;

// --- Player ---
constexpr float PLAYER_ORBIT_RADIUS = 32.0f;
constexpr int   PLAYER_SIZE = 6;
constexpr float PLAYER_SPEED = 3.5f;
constexpr unsigned long SHOOT_COOLDOWN = 350;

// --- Projectiles ---
constexpr int   BULLET_RADIUS = 3;
constexpr float BULLET_SPEED  = 4.0f;
constexpr int   MAX_BULLETS   = 12;

// --- Enemies ---
constexpr int   MAX_ENEMIES    = 24;
constexpr int   ENEMY_SIZE     = 8;
constexpr float ENEMY_BASE_SPEED = 0.6f;

// --- Special attack ---
constexpr int   SPECIAL_MAX    = 3;
constexpr unsigned long SPECIAL_RECHARGE_MS = 8000;

// --- Wave config ---
constexpr int   STARTING_ENEMIES = 4;
constexpr int   ENEMIES_PER_WAVE = 2;
constexpr float SPEED_RAMP       = 0.08f;

// --- Timing ---
constexpr unsigned long FRAME_MS = 33;
constexpr unsigned long DAMAGE_FLASH_MS = 400;
constexpr unsigned long WAVE_PAUSE_MS   = 2000;
constexpr unsigned long GAMEOVER_DELAY  = 3000;

// --- Double-buffer canvas (eliminates screen flicker) ---
M5Canvas canvas(&M5.Display);

// --- Colors (set in start()) ---
uint16_t bgColor;
uint16_t baseColor;
uint16_t baseGlowColor;
uint16_t playerColor;
uint16_t bulletColor;
uint16_t enemyRedColor;
uint16_t enemyBlueColor;
uint16_t enemyGreenColor;
uint16_t hpFullColor;
uint16_t hpEmptyColor;
uint16_t headerBgColor;
uint16_t waveTextColor;
uint16_t scoreTextColor;
uint16_t explosionColor;

// --- Upgrade types ---
enum class UpgradeType {
  RapidFire,     // character: shoot faster
  BigBullets,    // character: larger + faster bullets
  SpeedBoost,    // character: move faster
  Heal,          // base: restore 1 HP
  Shield,        // base: block next hit
  ExtraSpecial,  // base: +1 max special & refill
  COUNT
};

constexpr int UPGRADE_COUNT = (int)UpgradeType::COUNT;
constexpr int UPGRADE_CHOICES = 3;

const char* upgradeNames[] = {
  "RAPID FIRE", "BIG BULLETS", "SPEED UP",
  "HEAL +1", "SHIELD", "EXTRA SPECIAL"
};

const char* upgradeDescs[] = {
  "Shoot faster", "Bigger bullets", "Move faster",
  "Restore 1 HP", "Block next hit", "+1 special ammo"
};

bool isCharUpgrade(UpgradeType t) { return (int)t < 3; }

// --- Structs ---
struct Bullet {
  float x, y;
  float dx, dy;
  bool  active;
};

struct Enemy {
  float x, y;
  float speed;
  int   hp;
  int   type;   // 0 = blue, 1 = red, 2 = green
  bool  active;
};

struct Particle {
  float x, y;
  float dx, dy;
  int   life;
  uint16_t color;
  bool active;
};

constexpr int MAX_PARTICLES = 30;

// --- Game state ---
float playerAngle;
int   baseHP;
int   score;
int   wave;
int   specialAmmo;
int   specialMax;
unsigned long lastSpecialRecharge;
unsigned long lastShotTime;
unsigned long lastFrameTime;
unsigned long damageFlashUntil;
unsigned long vibrationOffTime;
unsigned long wavePauseUntil;
unsigned long gameOverTime;
bool  gameOver;
bool  waveCleared;
bool  waveDamageFree;
int   enemiesRemaining;
unsigned long nextSpawnTime;

// --- Mutable stats (modified by upgrades) ---
unsigned long curShootCooldown;
int   curBulletRadius;
float curBulletSpeed;
float curPlayerSpeed;
bool  shieldActive;

// --- Upgrade selection state ---
bool  showingUpgrades;
UpgradeType upgradeChoices[UPGRADE_CHOICES];

Bullet   bullets[MAX_BULLETS];
Enemy    enemies[MAX_ENEMIES];
Particle particles[MAX_PARTICLES];

// ---------- Helper drawing (draws to canvas) ----------

void drawCentered(const char* text, int x, int y, int sz, uint16_t color) {
  canvas.setFont(&fonts::Font0);
  canvas.setTextDatum(middle_center);
  canvas.setTextSize(sz);
  canvas.setTextColor(color);
  canvas.drawString(text, x, y);
  canvas.setTextDatum(top_left);
}

// ---------- Particles ----------

void spawnParticles(float x, float y, uint16_t color, int count) {
  for (int i = 0; i < MAX_PARTICLES && count > 0; ++i) {
    if (!particles[i].active) {
      particles[i].active = true;
      particles[i].x  = x;
      particles[i].y  = y;
      particles[i].dx = ((random(100) - 50) / 50.0f) * 2.5f;
      particles[i].dy = ((random(100) - 50) / 50.0f) * 2.5f;
      particles[i].life = 6 + random(6);
      particles[i].color = color;
      --count;
    }
  }
}

void updateParticles() {
  for (int i = 0; i < MAX_PARTICLES; ++i) {
    if (!particles[i].active) continue;
    particles[i].x += particles[i].dx;
    particles[i].y += particles[i].dy;
    --particles[i].life;
    if (particles[i].life <= 0) particles[i].active = false;
  }
}

void drawParticles() {
  for (int i = 0; i < MAX_PARTICLES; ++i) {
    if (!particles[i].active) continue;
    int sz = (particles[i].life > 4) ? 3 : 2;
    canvas.fillRect((int)particles[i].x, (int)particles[i].y, sz, sz,
                    particles[i].color);
  }
}

// ---------- Sound helpers ----------

void playShootSound()  { M5.Speaker.tone(1200, 40); }
void playHitSound()    { M5.Speaker.tone(800, 60); }
void playDamageSound() { M5.Speaker.tone(200, 200); }

void playWaveCompleteSound() {
  M5.Speaker.tone(880, 80);
  delay(60);
  M5.Speaker.tone(1100, 80);
  delay(60);
  M5.Speaker.tone(1320, 120);
}

void playGameOverSound() {
  M5.Speaker.tone(400, 200);
  delay(150);
  M5.Speaker.tone(300, 200);
  delay(150);
  M5.Speaker.tone(200, 400);
}

void playSpecialSound() {
  M5.Speaker.tone(600, 40);
  delay(30);
  M5.Speaker.tone(1400, 80);
}

// ---------- Spawn enemies ----------

void spawnEnemy() {
  for (int i = 0; i < MAX_ENEMIES; ++i) {
    if (enemies[i].active) continue;

    int side = random(4);
    float ex, ey;
    switch (side) {
      case 0: ex = (float)random(SCREEN_W); ey = -ENEMY_SIZE; break;
      case 1: ex = (float)random(SCREEN_W); ey = SCREEN_H + ENEMY_SIZE; break;
      case 2: ex = -ENEMY_SIZE;             ey = (float)random(SCREEN_H); break;
      default: ex = SCREEN_W + ENEMY_SIZE;  ey = (float)random(SCREEN_H); break;
    }

    int type = 0;
    if (wave >= 3) {
      int roll = random(100);
      if (wave >= 5 && roll < 15)      type = 2;
      else if (roll < 35)              type = 1;
    }

    enemies[i].active = true;
    enemies[i].x = ex;
    enemies[i].y = ey;
    enemies[i].type = type;

    float waveSpeed = ENEMY_BASE_SPEED + wave * SPEED_RAMP;
    if (type == 1)      { enemies[i].speed = waveSpeed * 1.6f; enemies[i].hp = 1; }
    else if (type == 2) { enemies[i].speed = waveSpeed * 0.7f; enemies[i].hp = 3; }
    else                { enemies[i].speed = waveSpeed;         enemies[i].hp = 1; }
    return;
  }
}

// ---------- Fire a bullet ----------

void fireBullet(float fromX, float fromY, float toX, float toY) {
  float dx = toX - fromX;
  float dy = toY - fromY;
  float len = sqrtf(dx * dx + dy * dy);
  if (len < 1.0f) return;
  dx /= len;
  dy /= len;

  for (int i = 0; i < MAX_BULLETS; ++i) {
    if (!bullets[i].active) {
      bullets[i].active = true;
      bullets[i].x  = fromX;
      bullets[i].y  = fromY;
      bullets[i].dx = dx * curBulletSpeed;
      bullets[i].dy = dy * curBulletSpeed;
      playShootSound();
      return;
    }
  }
}

// ---------- Find nearest enemy ----------

int findNearestEnemy(float fromX, float fromY) {
  int best = -1;
  float bestDist = 99999.0f;
  for (int i = 0; i < MAX_ENEMIES; ++i) {
    if (!enemies[i].active) continue;
    float dx = enemies[i].x - fromX;
    float dy = enemies[i].y - fromY;
    float d = sqrtf(dx * dx + dy * dy);
    if (d < bestDist) { bestDist = d; best = i; }
  }
  return best;
}

// ---------- Special attack ----------

void fireSpecial() {
  if (specialAmmo <= 0) return;
  --specialAmmo;
  lastSpecialRecharge = millis();
  playSpecialSound();

  for (int i = 0; i < MAX_ENEMIES; ++i) {
    if (!enemies[i].active) continue;
    enemies[i].hp -= 2;
    spawnParticles(enemies[i].x, enemies[i].y, explosionColor, 4);
    if (enemies[i].hp <= 0) {
      enemies[i].active = false;
      score += 1;
      spawnParticles(enemies[i].x, enemies[i].y, explosionColor, 4);
    }
  }
  // No white flash — it was hurting eyes
}

// ---------- Drawing ----------

void drawHeader() {
  canvas.fillRect(0, 0, SCREEN_W, 20, headerBgColor);

  canvas.setFont(&fonts::Font0);
  canvas.setTextSize(1);
  canvas.setTextDatum(top_left);

  canvas.setTextColor(waveTextColor);
  canvas.setCursor(4, 5);
  canvas.printf("WAVE %d", wave);

  canvas.setTextColor(scoreTextColor);
  canvas.setCursor(130, 5);
  canvas.printf("CASH $%d", score);

  // HP as hearts
  for (int i = 0; i < BASE_MAX_HP; ++i) {
    uint16_t c = (i < baseHP) ? hpFullColor : hpEmptyColor;
    int hx = 270 + i * 16;
    canvas.fillCircle(hx + 3, 8, 4, c);
    canvas.fillCircle(hx + 9, 8, 4, c);
    canvas.fillTriangle(hx, 10, hx + 12, 10, hx + 6, 17, c);
  }

  // Special ammo dots (uses specialMax for correct count)
  for (int i = 0; i < specialMax; ++i) {
    uint16_t c = (i < specialAmmo) ? canvas.color565(255, 200, 50)
                                    : canvas.color565(60, 60, 60);
    canvas.fillCircle(80 + i * 12, 10, 4, c);
  }
}

void drawBase() {
  unsigned long now = millis();
  bool flashing = (now < damageFlashUntil);

  // Shield ring (outermost)
  if (shieldActive) {
    canvas.drawCircle(CENTER_X, CENTER_Y, BASE_RADIUS + 7,
                      canvas.color565(100, 200, 255));
    canvas.drawCircle(CENTER_X, CENTER_Y, BASE_RADIUS + 6,
                      canvas.color565(60, 150, 220));
  }

  // Glow ring
  uint16_t glowC = flashing ? canvas.color565(255, 60, 60) : baseGlowColor;
  canvas.drawCircle(CENTER_X, CENTER_Y, BASE_RADIUS + 4, glowC);
  canvas.drawCircle(CENTER_X, CENTER_Y, BASE_RADIUS + 3, glowC);

  // Base body
  uint16_t bodyC = flashing ? canvas.color565(200, 30, 30) : baseColor;
  canvas.fillCircle(CENTER_X, CENTER_Y, BASE_RADIUS, bodyC);

  // Inner detail
  canvas.drawCircle(CENTER_X, CENTER_Y, BASE_RADIUS - 4,
                    canvas.color565(120, 180, 255));
}

void drawPlayer() {
  float px = CENTER_X + cosf(playerAngle) * PLAYER_ORBIT_RADIUS;
  float py = CENTER_Y + sinf(playerAngle) * PLAYER_ORBIT_RADIUS;

  int ix = (int)px;
  int iy = (int)py;
  canvas.fillTriangle(ix, iy - PLAYER_SIZE,
                      ix - PLAYER_SIZE, iy,
                      ix + PLAYER_SIZE, iy, playerColor);
  canvas.fillTriangle(ix, iy + PLAYER_SIZE,
                      ix - PLAYER_SIZE, iy,
                      ix + PLAYER_SIZE, iy, playerColor);

  // Direction indicator
  int ne = findNearestEnemy(px, py);
  if (ne >= 0) {
    float dx = enemies[ne].x - px;
    float dy = enemies[ne].y - py;
    float len = sqrtf(dx * dx + dy * dy);
    if (len > 1.0f) {
      int tx = ix + (int)(dx / len * 12);
      int ty = iy + (int)(dy / len * 12);
      canvas.drawLine(ix, iy, tx, ty, bulletColor);
    }
  }
}

void drawEnemies() {
  for (int i = 0; i < MAX_ENEMIES; ++i) {
    if (!enemies[i].active) continue;
    int ex = (int)enemies[i].x;
    int ey = (int)enemies[i].y;
    uint16_t c;
    switch (enemies[i].type) {
      case 1:  c = enemyRedColor;   break;
      case 2:  c = enemyGreenColor; break;
      default: c = enemyBlueColor;  break;
    }

    if (enemies[i].type == 2) {
      canvas.fillRect(ex - ENEMY_SIZE, ey - ENEMY_SIZE,
                      ENEMY_SIZE * 2, ENEMY_SIZE * 2, c);
      canvas.drawRect(ex - ENEMY_SIZE, ey - ENEMY_SIZE,
                      ENEMY_SIZE * 2, ENEMY_SIZE * 2, WHITE);
    } else if (enemies[i].type == 1) {
      canvas.fillTriangle(ex, ey - ENEMY_SIZE,
                          ex - ENEMY_SIZE, ey + ENEMY_SIZE,
                          ex + ENEMY_SIZE, ey + ENEMY_SIZE, c);
    } else {
      canvas.fillCircle(ex, ey, ENEMY_SIZE, c);
    }
  }
}

void drawBullets() {
  for (int i = 0; i < MAX_BULLETS; ++i) {
    if (!bullets[i].active) continue;
    canvas.fillCircle((int)bullets[i].x, (int)bullets[i].y,
                      curBulletRadius, bulletColor);
  }
}

void drawControlHints() {
  canvas.setFont(&fonts::Font0);
  canvas.setTextSize(1);
  canvas.setTextColor(canvas.color565(100, 100, 120));
  canvas.setTextDatum(bottom_center);
  canvas.drawString("A:SPECIAL", 54, SCREEN_H - 2);
  canvas.drawString("C:HOME", 267, SCREEN_H - 2);
  canvas.setTextDatum(top_left);
}

void drawWaveBanner() {
  canvas.fillRoundRect(80, 80, 160, 50, 8, canvas.color565(20, 30, 60));
  canvas.drawRoundRect(80, 80, 160, 50, 8, waveTextColor);
  char buf[24];
  snprintf(buf, sizeof(buf), "WAVE %d", wave);
  drawCentered(buf, CENTER_X, 100, 2, waveTextColor);
  drawCentered("GET READY!", CENTER_X, 118, 1, WHITE);
}

void drawGameOver() {
  canvas.fillScreen(bgColor);
  canvas.fillRoundRect(40, 60, 240, 120, 10, canvas.color565(30, 10, 10));
  canvas.drawRoundRect(40, 60, 240, 120, 10, canvas.color565(255, 60, 60));

  drawCentered("GAME OVER", CENTER_X, 88, 3, canvas.color565(255, 60, 60));

  char buf[32];
  snprintf(buf, sizeof(buf), "Cash: $%d", score);
  drawCentered(buf, CENTER_X, 120, 2, WHITE);

  snprintf(buf, sizeof(buf), "Waves: %d", wave - 1);
  drawCentered(buf, CENTER_X, 145, 1, canvas.color565(180, 180, 200));

  drawCentered("Tap or A: Retry  C: Home", CENTER_X, 168, 1,
               canvas.color565(150, 150, 170));
  canvas.pushSprite(0, 0);
}

// ---------- Collision detection ----------

bool circleHit(float ax, float ay, float ar, float bx, float by, float br) {
  float dx = ax - bx;
  float dy = ay - by;
  return (dx * dx + dy * dy) < (ar + br) * (ar + br);
}

// ---------- Core update logic ----------

void updateEnemies() {
  for (int i = 0; i < MAX_ENEMIES; ++i) {
    if (!enemies[i].active) continue;
    float dx = CENTER_X - enemies[i].x;
    float dy = CENTER_Y - enemies[i].y;
    float len = sqrtf(dx * dx + dy * dy);
    if (len < 1.0f) len = 1.0f;
    enemies[i].x += (dx / len) * enemies[i].speed;
    enemies[i].y += (dy / len) * enemies[i].speed;

    // Hit base?
    if (circleHit(enemies[i].x, enemies[i].y, ENEMY_SIZE,
                  CENTER_X, CENTER_Y, BASE_RADIUS)) {
      enemies[i].active = false;
      if (shieldActive) {
        shieldActive = false;
        spawnParticles(enemies[i].x, enemies[i].y,
                       canvas.color565(100, 200, 255), 6);
        M5.Speaker.tone(1500, 60);
      } else {
        --baseHP;
        waveDamageFree = false;
        damageFlashUntil = millis() + DAMAGE_FLASH_MS;
        playDamageSound();
        M5.Power.setVibration(200);
        vibrationOffTime = millis() + 150;
        spawnParticles(enemies[i].x, enemies[i].y,
                       canvas.color565(255, 80, 80), 5);
        if (baseHP <= 0) {
          gameOver = true;
          gameOverTime = millis();
          playGameOverSound();
        }
      }
    }
  }
}

void updateBullets() {
  for (int i = 0; i < MAX_BULLETS; ++i) {
    if (!bullets[i].active) continue;
    bullets[i].x += bullets[i].dx;
    bullets[i].y += bullets[i].dy;

    if (bullets[i].x < -10 || bullets[i].x > SCREEN_W + 10 ||
        bullets[i].y < -10 || bullets[i].y > SCREEN_H + 10) {
      bullets[i].active = false;
      continue;
    }

    for (int e = 0; e < MAX_ENEMIES; ++e) {
      if (!enemies[e].active) continue;
      if (circleHit(bullets[i].x, bullets[i].y, curBulletRadius,
                    enemies[e].x, enemies[e].y, ENEMY_SIZE)) {
        bullets[i].active = false;
        enemies[e].hp--;
        playHitSound();
        spawnParticles(enemies[e].x, enemies[e].y, explosionColor, 3);
        if (enemies[e].hp <= 0) {
          enemies[e].active = false;
          score += 1;
          spawnParticles(enemies[e].x, enemies[e].y, explosionColor, 5);
        }
        break;
      }
    }
  }
}

void autoShoot() {
  unsigned long now = millis();
  if (now - lastShotTime < curShootCooldown) return;

  float px = CENTER_X + cosf(playerAngle) * PLAYER_ORBIT_RADIUS;
  float py = CENTER_Y + sinf(playerAngle) * PLAYER_ORBIT_RADIUS;

  int nearest = findNearestEnemy(px, py);
  if (nearest < 0) return;

  fireBullet(px, py, enemies[nearest].x, enemies[nearest].y);
  lastShotTime = now;
}

bool allEnemiesCleared() {
  if (enemiesRemaining > 0) return false;
  for (int i = 0; i < MAX_ENEMIES; ++i) {
    if (enemies[i].active) return false;
  }
  return true;
}

void startNextWave() {
  ++wave;
  enemiesRemaining = STARTING_ENEMIES + (wave - 1) * ENEMIES_PER_WAVE;
  nextSpawnTime = millis();
  waveCleared = false;
  waveDamageFree = true;
  wavePauseUntil = millis() + WAVE_PAUSE_MS;
}

// ---------- Upgrade system ----------

void pickRandomUpgrades() {
  bool used[UPGRADE_COUNT] = {};
  for (int i = 0; i < UPGRADE_CHOICES; ++i) {
    int pick;
    int tries = 0;
    do {
      pick = random(UPGRADE_COUNT);
      ++tries;
    } while (used[pick] && tries < 50);
    used[pick] = true;
    upgradeChoices[i] = (UpgradeType)pick;
  }
}

void applyUpgrade(UpgradeType up) {
  switch (up) {
    case UpgradeType::RapidFire:
      if (curShootCooldown > 100) curShootCooldown -= 50;
      break;
    case UpgradeType::BigBullets:
      curBulletRadius += 2;
      curBulletSpeed += 0.8f;
      break;
    case UpgradeType::SpeedBoost:
      curPlayerSpeed += 1.5f;
      break;
    case UpgradeType::Heal:
      if (baseHP < BASE_MAX_HP) ++baseHP;
      break;
    case UpgradeType::Shield:
      shieldActive = true;
      break;
    case UpgradeType::ExtraSpecial:
      ++specialMax;
      specialAmmo = specialMax;
      break;
    default: break;
  }
  M5.Speaker.tone(1000, 50);
  delay(30);
  M5.Speaker.tone(1400, 80);
}

// Upgrade card layout constants
constexpr int UPG_CARD_W = 96;
constexpr int UPG_CARD_H = 110;
constexpr int UPG_CARD_Y = 55;
constexpr int UPG_GAP    = 8;
constexpr int UPG_TOTAL_W = UPG_CARD_W * 3 + UPG_GAP * 2;
constexpr int UPG_START_X = (SCREEN_W - UPG_TOTAL_W) / 2;

void drawUpgradeScreen() {
  canvas.fillScreen(bgColor);
  drawHeader();

  drawCentered("PICK AN UPGRADE!", CENTER_X, 38, 2, scoreTextColor);

  bool canAfford = (score >= 10);

  for (int i = 0; i < UPGRADE_CHOICES; ++i) {
    int cx = UPG_START_X + i * (UPG_CARD_W + UPG_GAP);
    int idx = (int)upgradeChoices[i];
    bool isChar = isCharUpgrade(upgradeChoices[i]);

    uint16_t cardColor = isChar
      ? canvas.color565(30, 80, 140)
      : canvas.color565(130, 50, 30);

    if (!canAfford) {
      // Gray out the card
      cardColor = canvas.color565(40, 40, 45);
    }

    // Shadow + card
    canvas.fillRoundRect(cx + 2, UPG_CARD_Y + 2, UPG_CARD_W, UPG_CARD_H, 6, BLACK);
    canvas.fillRoundRect(cx, UPG_CARD_Y, UPG_CARD_W, UPG_CARD_H, 6, cardColor);

    uint16_t borderColor = canAfford ? WHITE : canvas.color565(80, 80, 80);
    canvas.drawRoundRect(cx, UPG_CARD_Y, UPG_CARD_W, UPG_CARD_H, 6, borderColor);

    // Label
    uint16_t labelColor;
    if (canAfford) {
      labelColor = isChar ? canvas.color565(100, 200, 255) : canvas.color565(255, 150, 100);
    } else {
      labelColor = canvas.color565(120, 120, 120);
    }
    drawCentered(isChar ? "CHARACTER" : "BASE",
                 cx + UPG_CARD_W / 2, UPG_CARD_Y + 14, 1, labelColor);

    // Icon
    int iconY = UPG_CARD_Y + 30;
    int iconCX = cx + UPG_CARD_W / 2;
    uint16_t iconColor = canAfford ? playerColor : canvas.color565(100, 100, 100);
    uint16_t bColor = canAfford ? baseColor : canvas.color565(100, 100, 100);
    uint16_t bGlowColor = canAfford ? baseGlowColor : canvas.color565(80, 80, 80);
    if (isChar) {
      canvas.fillTriangle(iconCX, iconY - 8, iconCX - 8, iconY,
                          iconCX + 8, iconY, iconColor);
      canvas.fillTriangle(iconCX, iconY + 8, iconCX - 8, iconY,
                          iconCX + 8, iconY, iconColor);
    } else {
      canvas.fillCircle(iconCX, iconY, 8, bColor);
      canvas.drawCircle(iconCX, iconY, 10, bGlowColor);
    }

    // Name + description
    uint16_t textColor = canAfford ? WHITE : canvas.color565(150, 150, 150);
    uint16_t descColor = canAfford ? canvas.color565(180, 180, 200) : canvas.color565(110, 110, 110);
    drawCentered(upgradeNames[idx], cx + UPG_CARD_W / 2, UPG_CARD_Y + 52, 1, textColor);
    drawCentered(upgradeDescs[idx], cx + UPG_CARD_W / 2, UPG_CARD_Y + 70, 1, descColor);

    // Cost display
    uint16_t costColor = canAfford ? canvas.color565(100, 255, 100) : canvas.color565(255, 100, 100);
    drawCentered("Cost: $10", cx + UPG_CARD_W / 2, UPG_CARD_Y + 92, 1, costColor);
  }

  // Draw "NEXT WAVE" button
  int btnX = 90;
  int btnY = 184;
  int btnW = 140;
  int btnH = 26;
  canvas.fillRoundRect(btnX, btnY, btnW, btnH, 4, canvas.color565(50, 50, 60));
  canvas.drawRoundRect(btnX, btnY, btnW, btnH, 4, WHITE);
  drawCentered("NEXT WAVE", CENTER_X, btnY + btnH / 2, 1, WHITE);

  canvas.pushSprite(0, 0);
}

void spawnTick() {
  if (enemiesRemaining <= 0) return;
  unsigned long now = millis();
  if (now < nextSpawnTime) return;

  spawnEnemy();
  --enemiesRemaining;

  unsigned long interval = 1200;
  if (wave > 3) interval = 900;
  if (wave > 6) interval = 700;
  if (wave > 10) interval = 500;
  nextSpawnTime = now + interval;
}

void rechargeSpecial() {
  if (specialAmmo >= specialMax) return;
  if (millis() - lastSpecialRecharge >= SPECIAL_RECHARGE_MS) {
    ++specialAmmo;
    lastSpecialRecharge = millis();
  }
}

}  // namespace

// ============================================================
//  Public API
// ============================================================

namespace DefenseGame {

void start() {
  // Create canvas for double-buffered drawing (no flicker)
  canvas.createSprite(SCREEN_W, SCREEN_H);
  canvas.setColorDepth(16);

  // Colors
  bgColor        = canvas.color565(8, 12, 28);
  baseColor      = canvas.color565(60, 120, 220);
  baseGlowColor  = canvas.color565(80, 160, 255);
  playerColor    = canvas.color565(33, 205, 224);
  bulletColor    = canvas.color565(255, 255, 100);
  enemyRedColor  = canvas.color565(230, 60, 60);
  enemyBlueColor = canvas.color565(60, 120, 230);
  enemyGreenColor= canvas.color565(50, 200, 80);
  hpFullColor    = canvas.color565(255, 50, 70);
  hpEmptyColor   = canvas.color565(60, 60, 60);
  headerBgColor  = canvas.color565(15, 20, 40);
  waveTextColor  = canvas.color565(33, 205, 224);
  scoreTextColor = canvas.color565(255, 220, 100);
  explosionColor = canvas.color565(255, 180, 50);

  // Reset state
  playerAngle    = -M_PI / 2.0f;
  baseHP         = BASE_MAX_HP;
  score          = 0;
  wave           = 0;
  specialMax     = SPECIAL_MAX;
  specialAmmo    = SPECIAL_MAX;
  lastSpecialRecharge = millis();
  lastShotTime   = 0;
  lastFrameTime  = millis();
  damageFlashUntil = 0;
  vibrationOffTime = 0;
  wavePauseUntil = 0;
  gameOverTime   = 0;
  gameOver       = false;
  waveCleared    = false;
  waveDamageFree = true;
  enemiesRemaining = 0;
  nextSpawnTime  = 0;
  showingUpgrades = false;

  // Reset upgradeable stats
  curShootCooldown = SHOOT_COOLDOWN;
  curBulletRadius  = BULLET_RADIUS;
  curBulletSpeed   = BULLET_SPEED;
  curPlayerSpeed   = PLAYER_SPEED;
  shieldActive     = false;

  for (int i = 0; i < MAX_BULLETS;   ++i) bullets[i].active   = false;
  for (int i = 0; i < MAX_ENEMIES;   ++i) enemies[i].active   = false;
  for (int i = 0; i < MAX_PARTICLES; ++i) particles[i].active  = false;

  randomSeed(millis());

  startNextWave();
}

DefenseGameAction update() {
  unsigned long now = millis();
  if (now - lastFrameTime < FRAME_MS) return DefenseGameAction::None;
  lastFrameTime = now;

  // NOTE: M5.update() is called by main.cpp — do NOT call it here
  // or it will consume touch/button events before we can read them.

  // --- Turn off vibration when time is up ---
  if (vibrationOffTime > 0 && now >= vibrationOffTime) {
    M5.Power.setVibration(0);
    vibrationOffTime = 0;
  }

  // --- Game Over state ---
  if (gameOver) {
    M5.Power.setVibration(0);
    drawGameOver();

    while (millis() - gameOverTime < GAMEOVER_DELAY) {
      M5.update();
      if (M5.BtnC.wasPressed()) { canvas.deleteSprite(); return DefenseGameAction::Home; }
      delay(50);
    }

    while (true) {
      M5.update();
      if (M5.BtnC.wasPressed()) { canvas.deleteSprite(); return DefenseGameAction::Home; }
      if (M5.BtnA.wasPressed()) { start(); return DefenseGameAction::None; }
      if (M5.Touch.getCount() > 0) {
        const auto& t = M5.Touch.getDetail();
        if (t.wasPressed()) { start(); return DefenseGameAction::None; }
      }
      delay(50);
    }
  }

  // --- Upgrade selection screen ---
  if (showingUpgrades) {
    drawUpgradeScreen();

    if (M5.BtnC.wasPressed()) { canvas.deleteSprite(); return DefenseGameAction::Home; }

    if (M5.Touch.getCount() > 0) {
      const auto& t = M5.Touch.getDetail();
      if (t.wasPressed()) {
        // Check if cards are clicked
        for (int i = 0; i < UPGRADE_CHOICES; ++i) {
          int cx = UPG_START_X + i * (UPG_CARD_W + UPG_GAP);
          if (t.x >= cx && t.x < cx + UPG_CARD_W &&
              t.y >= UPG_CARD_Y && t.y < UPG_CARD_Y + UPG_CARD_H) {
            Serial.printf("Card touched: %d, Current Cash: %d\n", i, score);
            if (score >= 10) {
              score -= 10;
              Serial.printf("Deducted $10. New Cash: %d\n", score);
              applyUpgrade(upgradeChoices[i]);
              showingUpgrades = false;
              startNextWave();
            } else {
              Serial.printf("Not enough cash for card %d!\n", i);
              // Play error buzzer tone
              M5.Speaker.tone(150, 150);
              M5.Power.setVibration(100);
              vibrationOffTime = millis() + 100;
            }
            break;
          }
        }
        // Check if NEXT WAVE button is clicked
        if (t.x >= 90 && t.x < 90 + 140 &&
            t.y >= 184 && t.y < 184 + 26) {
          M5.Speaker.tone(800, 50);
          showingUpgrades = false;
          startNextWave();
        }
      }
    }
    return DefenseGameAction::None;
  }

  // --- Wave pause banner ---
  if (now < wavePauseUntil) {
    canvas.fillScreen(bgColor);
    drawHeader();
    drawBase();
    drawWaveBanner();
    drawControlHints();
    canvas.pushSprite(0, 0);
    return DefenseGameAction::None;
  }

  // --- Input: move player ---
  if (M5.Touch.getCount() > 0) {
    const auto& t = M5.Touch.getDetail();
    if (t.isPressed()) {
      float tx = t.x - CENTER_X;
      float ty = t.y - CENTER_Y;
      float targetAngle = atan2f(ty, tx);

      float diff = targetAngle - playerAngle;
      while (diff >  M_PI) diff -= 2.0f * M_PI;
      while (diff < -M_PI) diff += 2.0f * M_PI;

      float step = curPlayerSpeed * (FRAME_MS / 1000.0f);
      if (fabsf(diff) < step) {
        playerAngle = targetAngle;
      } else {
        playerAngle += (diff > 0) ? step : -step;
      }
    }
  }

  // --- Input: special attack ---
  if (M5.BtnA.wasPressed() && specialAmmo > 0) {
    fireSpecial();
  }

  // --- Spawn ---
  spawnTick();

  // --- Update entities ---
  autoShoot();
  updateBullets();
  updateEnemies();
  updateParticles();
  rechargeSpecial();

  // --- Check wave clear ---
  if (!waveCleared && allEnemiesCleared()) {
    waveCleared = true;
    if (waveDamageFree) {
      score += 5;
    }
    playWaveCompleteSound();
    showingUpgrades = true;
    pickRandomUpgrades();
    return DefenseGameAction::None;
  }

  // --- Draw everything to canvas, then push to screen (no flicker!) ---
  canvas.fillScreen(bgColor);
  drawHeader();
  drawBase();
  drawEnemies();
  drawBullets();
  drawPlayer();
  drawParticles();
  drawControlHints();
  canvas.pushSprite(0, 0);

  return DefenseGameAction::None;
}

}  // namespace DefenseGame
