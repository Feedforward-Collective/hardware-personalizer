// Starter app for the ESP32-C6 device. Replace this with your own app.
// Shows a greeting; tap to change colour, shake for a surprise, KEY to beep.
#include "esp32c6_board.h"

const uint16_t colors[] = {RGB565_ORANGE, RGB565_CYAN, RGB565_MAGENTA, RGB565_GREENYELLOW};
int colorIndex = 0;

void drawGreeting() {
  board.screen->fillScreen(RGB565_BLACK);
  board.screen->setFont(u8g2_font_logisoso58_tr);  // big, smooth
  board.centerText("Hello!", 240, 210, 1, colors[colorIndex]);
  board.screen->setFont(u8g2_font_helvR18_tr);
  board.centerText("tap, shake, or press KEY", 240, 310, 1, RGB565_DARKGREY);
}

void setup() {
  board.begin();
  drawGreeting();
}

void loop() {
  board.update();  // always first: reads touch, buttons and motion

  if (board.touch.tapped()) {
    colorIndex = (colorIndex + 1) % 4;
    drawGreeting();
  }
  if (board.motion.shaken()) {
    board.screen->fillScreen(RGB565_BLACK);
    board.screen->setFont(u8g2_font_logisoso58_tr);
    board.centerText("Whoa!", 240, 240, 1, RGB565_YELLOW);
    board.audio.chime();
    delay(800);
    drawGreeting();
  }
  if (board.key.wasPressed()) board.audio.beep();

  delay(5);
}
