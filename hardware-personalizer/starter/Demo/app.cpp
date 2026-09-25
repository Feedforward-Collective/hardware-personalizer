// Demo: smooth canvas animation, Wi-Fi scan, record + playback, and tilt readout.
// Tap the screen to move to the next part. Reference code for apps that use these.
#include "esp32c6_board.h"
#include <WiFi.h>

Arduino_Canvas *canvas;
int16_t *rec = nullptr;
const size_t REC_N = 16000 * 2;  // 2 s at 16 kHz
int phase = 0;
uint32_t phaseStart = 0;

void showPhase() {
  auto *s = board.screen;
  s->fillScreen(RGB565_BLACK);
  const char *names[] = {"1 ANIMATION", "2 WIFI SCAN", "3 RECORD", "4 TILT"};
  board.centerText(names[phase], 240, 40, 3, RGB565_WHITE);
  board.centerText("tap for next test", 240, 450, 2, RGB565_DARKGREY);
  phaseStart = millis();
  Serial.printf("EXTRAS phase %d heap=%u\n", phase, (unsigned)ESP.getFreeHeap());
}

void setup() {
  board.begin();
  delay(1000);
  canvas = new Arduino_Canvas(480, 200, board.screen, 0, 140);
  Serial.printf("EXTRAS canvas begin=%d heap=%u\n", canvas->begin(GFX_SKIP_OUTPUT_BEGIN),
                (unsigned)ESP.getFreeHeap());
  rec = (int16_t *)malloc(REC_N * sizeof(int16_t));
  Serial.printf("EXTRAS rec buffer=%d heap=%u\n", rec != nullptr, (unsigned)ESP.getFreeHeap());
  showPhase();
}

void loop() {
  board.update();
  if (board.touch.tapped()) {
    Serial.printf("EXTRAS tap x=%d y=%d\n", board.touch.x, board.touch.y);
    phase = (phase + 1) % 4;
    showPhase();
  }
  if (phase == 0) {
    static uint32_t frames = 0, t0 = millis();
    float t = (millis() - phaseStart) / 1000.0f;
    canvas->fillScreen(RGB565_BLACK);
    int x = 240 + (int)(200 * sinf(t * 2)), y = 100 + (int)(60 * cosf(t * 3));
    canvas->fillCircle(x, y, 30, RGB565_ORANGE);
    canvas->setTextColor(RGB565_WHITE);
    canvas->setTextSize(2);
    canvas->setCursor(10, 10);
    canvas->printf("smooth? fps=%u", (unsigned)(frames * 1000 / max<uint32_t>(1, millis() - t0)));
    canvas->flush();
    frames++;
    if (millis() - t0 > 3000) { Serial.printf("EXTRAS fps=%u\n", (unsigned)(frames * 1000 / (millis() - t0))); frames = 0; t0 = millis(); }
  } else if (phase == 1 && millis() - phaseStart < 100) {
    WiFi.mode(WIFI_STA);
    int n = WiFi.scanNetworks();
    Serial.printf("EXTRAS wifi networks=%d heap=%u\n", n, (unsigned)ESP.getFreeHeap());
    board.centerText(String(n) + " networks found", 240, 200, 3, RGB565_GREEN);
    for (int i = 0; i < min(n, 5); i++) board.centerText(WiFi.SSID(i) + " " + WiFi.RSSI(i) + "dB", 240, 260 + i * 30, 2, RGB565_WHITE);
    WiFi.mode(WIFI_OFF);
  } else if (phase == 2 && millis() - phaseStart < 100) {
    board.centerText("speak now (2 s)...", 240, 200, 3, RGB565_RED);
    delay(300);
    size_t got = board.audio.record(rec, REC_N);
    int32_t peak = 0;
    for (size_t i = 0; i < got; i++) peak = max<int32_t>(peak, abs(rec[i]));
    Serial.printf("EXTRAS recorded=%u peak=%d\n", (unsigned)got, (int)peak);
    board.centerText("playing back...", 240, 260, 3, RGB565_GREEN);
    // boost quiet recordings so playback is audible
    int gain = peak > 0 ? min<int>(8, 20000 / peak) : 1;
    for (size_t i = 0; i < got; i++) rec[i] = (int16_t)constrain(rec[i] * gain, -32767, 32767);
    board.audio.play(rec, got);
    board.centerText("done", 240, 320, 3, RGB565_WHITE);
  } else if (phase == 3) {
    static uint32_t last = 0;
    if (millis() - last > 200) {
      last = millis();
      board.screen->fillRect(0, 150, 480, 200, RGB565_BLACK);
      board.centerText(String("pitch ") + (int)board.motion.pitch() + "  roll " + (int)board.motion.roll(), 240, 200, 3, RGB565_WHITE);
      board.centerText(board.motion.faceUp() ? "FACE UP" : board.motion.faceDown() ? "FACE DOWN" : "tilted", 240, 260, 3, RGB565_YELLOW);
      Serial.printf("EXTRAS tilt a=%.2f,%.2f,%.2f pitch=%.0f roll=%.0f\n", board.motion.ax, board.motion.ay, board.motion.az, board.motion.pitch(), board.motion.roll());
    }
  }
}
