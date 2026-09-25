// Tour: a swipeable set of mini demos that show what the ESP32-C6 device can do.
// These are deliberately rough sketches of the hardware's abilities, not finished
// designs: a real app can be far more polished.
// Swipe left/right (or press KEY / BOOT) to move between demos.
//
// To choose which demos appear, edit the DEMOS list near the bottom of this file.
// Each demo is a self-contained section (enter + loop functions) that can also be
// lifted into a real app as a starting point.
#include "esp32c6_board.h"

// ------------------------------------------------------------ shared helpers
const int TOP = 60, BOTTOM = 420;  // demo area between the title bar and the footer
const int CX = 240, CY = (TOP + BOTTOM) / 2;

void clearArea(uint16_t bg = RGB565_BLACK) { board.screen->fillRect(0, TOP, 480, BOTTOM - TOP, bg); }

// Draw text word-wrapped to maxW, centred on cy, in the current font.
void wrapText(const String &text, int cy, int maxW, uint16_t color, uint16_t bg = RGB565_BLACK) {
  String lines[6];
  int n = 0;
  String line;
  int16_t x1, y1;
  uint16_t w, h;
  int start = 0;
  while (start <= (int)text.length() && n < 6) {
    int sp = text.indexOf(' ', start);
    if (sp < 0) sp = text.length();
    String word = text.substring(start, sp);
    String trial = line.length() ? line + " " + word : word;
    board.screen->getTextBounds(trial.c_str(), 0, 0, &x1, &y1, &w, &h);
    if (w > maxW && line.length()) { lines[n++] = line; line = word; }
    else line = trial;
    start = sp + 1;
  }
  if (line.length() && n < 6) lines[n++] = line;
  board.screen->getTextBounds("Ag", 0, 0, &x1, &y1, &w, &h);
  int lh = h + 12;
  int y = cy - (n - 1) * lh / 2;
  for (int i = 0; i < n; i++) board.centerText(lines[i], CX, y + i * lh, 1, color, bg);
}

// Ring from 12 o'clock, clockwise, sweeping `deg` degrees (arcs split at 360).
void ring(int cx, int cy, int rOut, int rIn, float deg, uint16_t color) {
  if (deg <= 0) return;
  float end = 270 + min(deg, 359.9f);
  board.screen->fillArc(cx, cy, rOut, rIn, 270, min(end, 360.0f), color);
  if (end > 360) board.screen->fillArc(cx, cy, rOut, rIn, 0, end - 360, color);
}

// ============================================================ 1. Shake for a tip
const char *TIPS[] = {
    "Ask AI to turn your meeting notes into three action items.",
    "Paste a long email thread and ask for the one decision needed.",
    "Have AI draft the first version; you edit the second.",
    "Ask for five counter-arguments before you send that proposal.",
    "Turn a messy spreadsheet into a one-paragraph story.",
    "Ask AI to explain your industry to a new hire, then check it.",
};
void fortuneEnter() {
  board.screen->setFont(u8g2_font_logisoso42_tr);
  board.centerText("Shake me", CX, CY - 10, 1, RGB565_GOLD);
  board.screen->setFont(u8g2_font_helvR18_tr);
  board.centerText("for a tip", CX, CY + 50, 1, RGB565_DARKGREY);
}
void fortuneLoop() {
  if (!board.motion.shaken()) return;
  clearArea();
  for (int i = 0; i < 12; i++) {  // a quick swirl
    int a = i * 30;
    board.screen->fillArc(CX, CY, 110, 80, a, a + 20, i % 2 ? RGB565_GOLD : RGB565_ORANGE);
    delay(40);
  }
  board.audio.chime();
  clearArea();
  board.screen->setFont(u8g2_font_helvB24_tr);
  wrapText(TIPS[random(6)], CY, 400, RGB565_WHITE);
}

// ============================================================ 2. Status badge
struct Status { const char *text; uint16_t color; };
const Status STATUSES[] = {{"Heads down", RGB565(200, 30, 40)},
                           {"Ask me anything", RGB565(20, 150, 70)},
                           {"On a call", RGB565(110, 50, 180)},
                           {"Back in 10", RGB565(220, 140, 0)}};
int statusIndex = 0;
void statusDraw() {
  const Status &s = STATUSES[statusIndex];
  clearArea(s.color);
  board.screen->setFont(u8g2_font_logisoso42_tr);
  wrapText(s.text, CY - 10, 420, RGB565_WHITE, s.color);
  board.screen->setFont(u8g2_font_helvR14_tr);
  board.centerText("tap to change", CX, BOTTOM - 30, 1, RGB565_WHITE, s.color);
}
void statusEnter() { statusDraw(); }
void statusLoop() {
  if (!board.touch.tapped()) return;
  statusIndex = (statusIndex + 1) % 4;
  board.audio.tone(1200, 40);
  statusDraw();
}

// ============================================================ 3. Flip timer
enum TimerState { T_IDLE, T_RUN, T_PAUSE, T_DONE };
TimerState tState = T_IDLE;
const uint32_t TIMER_MS = 60000;
uint32_t tRemaining = TIMER_MS, tLast = 0, orientSince = 0;
int orient = 0, orientCandidate = 0;  // 1 face-down, -1 face-up, 0 tilted
int lastSecShown = -1;
void timerDraw(bool full) {
  if (full) clearArea();
  board.screen->fillArc(CX, CY, 150, 130, 0, 360, RGB565(30, 30, 30));  // track
  ring(CX, CY, 150, 130, 360.0f * tRemaining / TIMER_MS, RGB565_GREEN);  // time left
  char buf[8];
  snprintf(buf, sizeof buf, "%d:%02d", (int)(tRemaining / 60000), (int)(tRemaining / 1000 % 60));
  board.screen->setFont(u8g2_font_logisoso50_tr);
  board.centerText(buf, CX, CY - 8, 1, RGB565_WHITE);
  board.screen->setFont(u8g2_font_helvR14_tr);
  const char *label = tState == T_IDLE ? "flip face-down to start" : tState == T_PAUSE ? "   paused   " : "  focusing  ";
  board.screen->fillRect(100, CY + 40, 280, 30, RGB565_BLACK);
  board.centerText(label, CX, CY + 52, 1, RGB565_DARKGREY);
}
void timerEnter() {
  tState = T_IDLE;
  tRemaining = TIMER_MS;
  orient = orientCandidate = board.motion.faceDown() ? 1 : board.motion.faceUp() ? -1 : 0;
  timerDraw(true);
}
void timerLoop() {
  int now = board.motion.faceDown() ? 1 : board.motion.faceUp() ? -1 : 0;
  if (now != orientCandidate) { orientCandidate = now; orientSince = millis(); }
  bool changed = false;
  if (orientCandidate != orient && millis() - orientSince > 300) { orient = orientCandidate; changed = true; }

  if (tState == T_DONE) {
    if (board.touch.tapped()) timerEnter();
    return;
  }
  if (changed && orient == 1 && tState != T_RUN) {  // flipped face-down: go
    if (tState == T_IDLE) board.audio.chime();
    tState = T_RUN;
    tLast = millis();
    timerDraw(false);
  } else if (changed && orient != 1 && tState == T_RUN) {  // turned back up: pause
    tState = T_PAUSE;
    timerDraw(false);
  }
  if (tState == T_RUN) {
    uint32_t t = millis();
    uint32_t dt = t - tLast;
    tLast = t;
    tRemaining = dt >= tRemaining ? 0 : tRemaining - dt;
    int sec = tRemaining / 1000;
    if (sec != lastSecShown) { lastSecShown = sec; timerDraw(false); }
    if (tRemaining == 0) {
      tState = T_DONE;
      clearArea(RGB565_GOLD);
      board.screen->setFont(u8g2_font_logisoso58_tr);
      board.centerText("Done!", CX, CY, 1, RGB565_BLACK, RGB565_GOLD);
      board.audio.tone(784, 150); board.audio.tone(988, 150); board.audio.tone(1175, 300);
    }
  }
}

// ============================================================ 4. Noise meter
float noiseSmooth = 0;
int noiseZone = -1;
uint32_t noiseLast = 0;
void noiseEnter() {
  noiseZone = -1;
  board.screen->setFont(u8g2_font_helvR14_tr);
  board.centerText("make some noise", CX, BOTTOM - 30, 1, RGB565_DARKGREY);
}
void noiseLoop() {
  if (millis() - noiseLast < 60) return;
  noiseLast = millis();
  float lv = board.audio.level(40);
  noiseSmooth = noiseSmooth * 0.7f + lv * 0.3f;
  float frac = constrain(noiseSmooth / 0.3f, 0.0f, 1.0f);
  int zone = frac < 0.35f ? 0 : frac < 0.7f ? 1 : 2;
  const uint16_t colors[] = {RGB565_GREEN, RGB565_ORANGE, RGB565_RED};
  const char *words[] = {"calm", "lively", "LOUD"};
  int barH = (int)(frac * 220);  // meter bar
  board.screen->fillRect(200, TOP + 30, 80, 220 - barH, RGB565(30, 30, 30));
  board.screen->fillRect(200, TOP + 30 + 220 - barH, 80, barH, colors[zone]);
  if (zone != noiseZone) {
    noiseZone = zone;
    board.screen->setFont(u8g2_font_logisoso34_tr);
    board.screen->fillRect(60, TOP + 262, 360, 50, RGB565_BLACK);
    board.centerText(words[zone], CX, TOP + 285, 1, colors[zone]);
  }
}

// ============================================================ 5. Tilt marble
float mx, my, vx, vy;
int tx, ty, score;
void marbleTarget() {
  tx = random(60, 420);
  ty = random(TOP + 50, BOTTOM - 50);
}
void marbleEnter() {
  mx = CX; my = CY; vx = vy = 0; score = 0;
  marbleTarget();
  board.screen->setFont(u8g2_font_helvR14_tr);
  board.centerText("tilt to roll the ball into the ring", CX, TOP + 20, 1, RGB565_DARKGREY);
}
void marbleLoop() {
  static uint32_t last = 0;
  if (millis() - last < 30) return;
  last = millis();
  int ox = (int)mx, oy = (int)my;
  vx += -board.motion.roll() * 0.02f;   // right edge up -> rolls left
  vy += board.motion.pitch() * 0.02f;   // top edge up -> rolls down
  vx *= 0.97f; vy *= 0.97f;
  mx += vx; my += vy;
  const int R = 16, minY = TOP + 40 + R, maxY = BOTTOM - R;
  if (mx < R) { mx = R; vx = -vx * 0.5f; }
  if (mx > 479 - R) { mx = 479 - R; vx = -vx * 0.5f; }
  if (my < minY) { my = minY; vy = -vy * 0.5f; }
  if (my > maxY) { my = maxY; vy = -vy * 0.5f; }
  board.screen->fillCircle(ox, oy, R + 1, RGB565_BLACK);
  board.screen->fillArc(tx, ty, 26, 20, 0, 360, RGB565_CYAN);
  board.screen->fillCircle((int)mx, (int)my, R, RGB565_ORANGE);
  if (hypotf(mx - tx, my - ty) < 14) {
    score++;
    board.audio.tone(1500, 60);
    board.screen->fillCircle(tx, ty, 27, RGB565_BLACK);
    marbleTarget();
  }
}

// ============================================================ 6. KPI gauge
float kpi = 0;
int kpiMilestone = 0;
uint32_t kpiLast = 0;
const float KPI_GOAL = 100;
void kpiDraw() {
  float frac = constrain(kpi / KPI_GOAL, 0.0f, 1.0f);
  // 240-degree gauge from 150 deg (lower left) clockwise to 30 deg (lower right)
  board.screen->fillArc(CX, CY + 20, 160, 130, 150, 360, RGB565(40, 40, 40));
  board.screen->fillArc(CX, CY + 20, 160, 130, 0, 30, RGB565(40, 40, 40));
  float end = 150 + 240 * frac;
  if (frac > 0) {
    board.screen->fillArc(CX, CY + 20, 160, 130, 150, min(end, 360.0f), RGB565_CYAN);
    if (end > 360) board.screen->fillArc(CX, CY + 20, 160, 130, 0, end - 360, RGB565_CYAN);
  }
  board.screen->setFont(u8g2_font_logisoso58_tr);
  board.screen->fillRect(120, CY - 30, 240, 80, RGB565_BLACK);
  board.centerText(String((int)kpi), CX, CY + 10, 1, RGB565_WHITE);
}
void kpiEnter() {
  kpiMilestone = (int)(kpi / 25);
  board.screen->setFont(u8g2_font_helvR18_tr);
  board.centerText("sales today", CX, CY + 80, 1, RGB565_DARKGREY);
  board.screen->setFont(u8g2_font_helvR14_tr);
  board.centerText("sample data - tap to add a sale", CX, BOTTOM - 20, 1, RGB565_DARKGREY);
  kpiDraw();
}
void kpiLoop() {
  bool changed = false;
  if (millis() - kpiLast > 1500) { kpiLast = millis(); kpi += random(0, 3); changed = true; }
  if (board.touch.tapped()) { kpi += 5; changed = true; }
  if (kpi > KPI_GOAL) kpi = 0;
  if (!changed) return;
  kpiDraw();
  int m = (int)(kpi / 25);
  if (m > kpiMilestone) board.audio.chime();
  kpiMilestone = m;
}

// ============================================================ 7. Desk pet
enum PetMood { P_IDLE, P_HAPPY, P_DIZZY, P_SURPRISED, P_BLINK };
PetMood petMood = P_IDLE, petDrawn = P_BLINK;
uint32_t petUntil = 0, petNextBlink = 0, petMicLast = 0;
void petDraw() {
  clearArea();
  uint16_t c = RGB565(255, 200, 80);
  int ey = CY - 40;
  for (int side = -1; side <= 1; side += 2) {
    int ex = CX + side * 80;
    switch (petMood) {
      case P_HAPPY:  board.screen->fillArc(ex, ey + 10, 34, 24, 200, 340, c); break;          // ^ ^
      case P_DIZZY:  board.screen->drawLine(ex - 22, ey - 22, ex + 22, ey + 22, c);
                     board.screen->drawLine(ex - 22, ey + 22, ex + 22, ey - 22, c);
                     board.screen->drawLine(ex - 21, ey - 22, ex + 23, ey + 22, c);
                     board.screen->drawLine(ex - 21, ey + 22, ex + 23, ey - 22, c); break;   // X X
      case P_BLINK:  board.screen->fillRect(ex - 28, ey - 4, 56, 8, c); break;                // - -
      default:       board.screen->fillCircle(ex, ey, petMood == P_SURPRISED ? 34 : 26, c);  // o o
    }
  }
  int my = CY + 70;
  if (petMood == P_SURPRISED) board.screen->fillArc(CX, my, 28, 18, 0, 360, c);
  else if (petMood == P_DIZZY) board.screen->fillRect(CX - 40, my - 4, 80, 8, c);
  else board.screen->fillArc(CX, my - 30, 70, 58, 30, 150, c);  // smile
  petDrawn = petMood;
}
void petSet(PetMood m, uint32_t ms) {
  petMood = m;
  petUntil = millis() + ms;
}
void petEnter() { petMood = P_IDLE; petNextBlink = millis() + 3000; petDraw(); }
void petLoop() {
  uint32_t now = millis();
  if (board.touch.tapped()) { petSet(P_HAPPY, 1500); board.audio.tone(1400, 60); board.audio.tone(1800, 80); }
  if (board.motion.shaken()) { petSet(P_DIZZY, 2000); board.audio.tone(300, 200); }
  if (now - petMicLast > 250) {
    petMicLast = now;
    if (petMood == P_IDLE && board.audio.level(30) > 0.3f) petSet(P_SURPRISED, 1200);
  }
  if (petMood != P_IDLE && petMood != P_BLINK && now > petUntil) petMood = P_IDLE;
  if (petMood == P_IDLE && now > petNextBlink) { petSet(P_BLINK, 150); petNextBlink = now + 3000 + random(2000); }
  if (petMood == P_BLINK && now > petUntil) petMood = P_IDLE;
  if (petMood != petDrawn) petDraw();
}

// ============================================================ 8. Quick poll
const char *POLL_Q = "How useful was today?";
const char *POLL_A[] = {"Very", "Somewhat", "Not yet"};
const uint16_t POLL_C[] = {RGB565(20, 150, 70), RGB565(200, 140, 0), RGB565(180, 50, 50)};
int votes[3] = {0, 0, 0};
void pollRow(int i) {
  int y = TOP + 70 + i * 100, total = votes[0] + votes[1] + votes[2];
  board.screen->fillRoundRect(30, y, 420, 80, 16, RGB565(35, 35, 35));
  int w = total ? 420 * votes[i] / total : 0;
  if (w > 32) board.screen->fillRoundRect(30, y, w, 80, 16, POLL_C[i]);
  board.screen->setFont(u8g2_font_helvB24_tr);
  board.screen->setTextColor(RGB565_WHITE);
  board.screen->setCursor(54, y + 52);
  board.screen->print(POLL_A[i]);
  board.screen->setCursor(390, y + 52);
  board.screen->print(votes[i]);
}
void pollEnter() {
  board.screen->setFont(u8g2_font_helvB18_tr);
  board.centerText(POLL_Q, CX, TOP + 30, 1, RGB565_WHITE);
  for (int i = 0; i < 3; i++) pollRow(i);
}
void pollLoop() {
  if (!board.touch.tapped()) return;
  int y = board.touch.y - (TOP + 70);
  if (y < 0 || y % 100 > 80 || y / 100 > 2) return;
  votes[y / 100]++;
  board.audio.tone(1300, 50);
  for (int i = 0; i < 3; i++) pollRow(i);
}

// ============================================================ the tour
struct Demo {
  const char *name;   // title bar
  const char *idea;   // footer: what this could become
  void (*enter)();
  void (*loop)();
};

// Choose the demos: delete or reorder lines. (Unused demo code above can stay.)
const Demo DEMOS[] = {
    {"Shake for a tip", "booth magnet, icebreaker, daily tip", fortuneEnter, fortuneLoop},
    {"Status badge", "desk or door sign, calendar-driven", statusEnter, statusLoop},
    {"Flip timer", "focus sessions, standups, workshops", timerEnter, timerLoop},
    {"Noise meter", "open-office or classroom nudge", noiseEnter, noiseLoop},
    {"Tilt marble", "booth game, bubble level", marbleEnter, marbleLoop},
    {"Live number", "sales, sign-ups, tickets via Wi-Fi", kpiEnter, kpiLoop},
    {"Desk pet", "a companion that reacts to you", petEnter, petLoop},
    {"Quick poll", "event feedback, retros, votes", pollEnter, pollLoop},
};
const int NUM_DEMOS = sizeof(DEMOS) / sizeof(DEMOS[0]);
int current = 0;

void showDemo(int i) {
  current = (i + NUM_DEMOS) % NUM_DEMOS;
  const Demo &d = DEMOS[current];
  board.screen->fillScreen(RGB565_BLACK);
  board.screen->setFont(u8g2_font_helvB18_tr);
  board.centerText(d.name, CX, 30, 1, RGB565_WHITE);
  board.screen->setFont(u8g2_font_helvR14_tr);
  board.centerText(String("Could be: ") + d.idea, CX, 440, 1, RGB565_DARKGREY);
  int x0 = CX - (NUM_DEMOS - 1) * 9;  // page dots
  for (int k = 0; k < NUM_DEMOS; k++)
    board.screen->fillCircle(x0 + k * 18, 466, 4, k == current ? RGB565_WHITE : RGB565(70, 70, 70));
  Serial.printf("[tour] demo %d: %s\n", current + 1, d.name);
  d.enter();
}

void setup() {
  board.begin();
  randomSeed(esp_random());
  showDemo(0);
}

void loop() {
  board.update();
  char sw = board.touch.swipe();
  if (sw == 'L' || board.key.wasPressed()) { board.audio.tone(900, 30); showDemo(current + 1); return; }
  if (sw == 'R' || board.boot.wasPressed()) { board.audio.tone(700, 30); showDemo(current - 1); return; }
  DEMOS[current].loop();
  delay(5);
}
