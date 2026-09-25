// Self-test for the ESP32-C6 device. Checks every part of esp32c6_board.h and
// reports on screen and over USB serial (115200 baud).
//   Touch: a dot follows your finger.   KEY: beep.   PWR: chime.
//   BOOT: redraw the screen.            Shake: screen flashes.
//   Tilt: pitch/roll update live (raise the TOP edge -> pitch goes positive).
#include "esp32c6_board.h"

uint32_t lastReport = 0;

void drawHome() {
  auto *s = board.screen;
  s->fillScreen(RGB565_BLACK);
  s->fillTriangle(240, 10, 220, 40, 260, 40, RGB565_WHITE);
  board.centerText("TOP", 240, 62, 3, RGB565_WHITE);
  s->fillRect(40, 130, 133, 50, RGB565_RED);
  s->fillRect(173, 130, 134, 50, RGB565_GREEN);
  s->fillRect(307, 130, 133, 50, RGB565_BLUE);
  board.centerText("drag: dot follows finger", 240, 300, 2, RGB565_YELLOW);
  board.centerText("KEY beep  PWR chime", 240, 330, 2, RGB565_YELLOW);
  board.centerText("BOOT redraw  shake flash", 240, 360, 2, RGB565_YELLOW);
}

void setup() {
  bool ok = board.begin();
  delay(1500);  // let the serial monitor attach
  Serial.printf("SELFTEST begin ok=%d\n", ok);
  const uint8_t addrs[] = {0x18, 0x34, 0x40, 0x51, 0x5A, 0x6B};
  const char *names[] = {"speaker chip", "power chip", "mic chip", "clock", "touch", "motion"};
  for (int i = 0; i < 6; i++)
    Serial.printf("SELFTEST %-12s %s\n", names[i], c6i2c::present(addrs[i]) ? "ok" : "MISSING");
  drawHome();
  if (board.audio.ready()) board.audio.chime();
}

void loop() {
  board.update();

  if (board.touch.pressed()) board.screen->fillCircle(board.touch.x, board.touch.y, 8, RGB565_CYAN);
  if (board.touch.tapped()) Serial.printf("SELFTEST tap %d,%d\n", board.touch.x, board.touch.y);
  if (char sw = board.touch.swipe()) Serial.printf("SELFTEST swipe %c\n", sw);
  if (board.key.wasPressed()) { Serial.println("SELFTEST key"); board.audio.beep(); }
  if (board.power.pwrPressed()) { Serial.println("SELFTEST pwr"); board.audio.chime(); }
  if (board.boot.wasPressed()) {
    Serial.println("SELFTEST boot");
    drawHome();
  }
  if (board.motion.shaken()) {
    Serial.println("SELFTEST shake");
    board.screen->fillScreen(RGB565_WHITE);
    delay(150);
    drawHome();
  }

  if (millis() - lastReport > 500) {
    lastReport = millis();
    board.screen->fillRect(0, 200, 480, 80, RGB565_BLACK);
    board.centerText(String("pitch ") + (int)board.motion.pitch() + "  roll " + (int)board.motion.roll(),
                     240, 215, 2, RGB565_WHITE);
    int pct = board.power.percent();
    board.centerText(String("battery ") + (pct < 0 ? String("none") : String(pct) + "%") +
                         (board.power.charging() ? " charging" : ""), 240, 250, 2, RGB565_WHITE);
    Serial.printf("SELFTEST a=%.2f,%.2f,%.2f pitch=%.0f roll=%.0f bat=%d%% %.2fV usb=%d chg=%d heap=%u\n",
                  board.motion.ax, board.motion.ay, board.motion.az, board.motion.pitch(),
                  board.motion.roll(), pct, board.power.volts(), board.power.usbPowered(),
                  board.power.charging(), (unsigned)ESP.getFreeHeap());
  }
  delay(5);
}
