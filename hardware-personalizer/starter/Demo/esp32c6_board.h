// esp32c6_board.h -- one-file board support for the Waveshare
// ESP32-C6-Touch-AMOLED-2.16 ("the ESP32-C6 device").
//
// Keep this file next to your sketch (.ino) in the same folder. It sets up
// every part of the device so your sketch only describes what the app does:
//
//   #include "esp32c6_board.h"
//   void setup() { board.begin(); board.screen->println("Hello"); }
//   void loop()  { board.update(); if (board.touch.tapped()) { ... } }
//
// Needs: Arduino-ESP32 core 3.3.x (Board: "ESP32C6 Dev Module",
// USB CDC On Boot: Enabled, Flash Size: 16MB) and two libraries:
// "GFX Library for Arduino" by Moon On Our Nation (1.6.x) and "U8g2" by oliver.
//
// Hardware facts were verified on the device; do not change the pins,
// addresses or init sequences below unless the device itself proves them wrong.
// Register sequences for the ES8311/ES7210 audio chips follow Espressif's
// esp_codec_dev drivers (Apache-2.0); the rest is original.

#pragma once
#include <Arduino.h>
#include <Wire.h>
#include <ESP_I2S.h>
#include <U8g2lib.h>  // before the GFX library, so it enables smooth u8g2 fonts
#include <Arduino_GFX_Library.h>
#include <sys/time.h>
#include <time.h>

namespace c6pins {
constexpr int I2C_SDA = 8, I2C_SCL = 7;
constexpr int LCD_CS = 15, LCD_SCK = 0, LCD_D0 = 1, LCD_D1 = 2, LCD_D2 = 3, LCD_D3 = 4;
constexpr int TOUCH_INT = 5, TOUCH_RST = 11;
constexpr int KEY_BUTTON = 10;  // third button on the case, labelled +/KEY
constexpr int BOOT_BUTTON = 9;  // first button; usable by apps once running
constexpr int I2S_MCLK = 19, I2S_BCLK = 20, I2S_WS = 22, I2S_DOUT = 23, I2S_DIN = 21;
constexpr int SD_CS = 6;        // SD shares the display's SPI clock/data pins
constexpr uint8_t ADDR_PMIC = 0x34, ADDR_TOUCH = 0x5A, ADDR_IMU = 0x6B,
                  ADDR_RTC = 0x51, ADDR_DAC = 0x18, ADDR_MICADC = 0x40;
}  // namespace c6pins

constexpr int SCREEN_W = 480, SCREEN_H = 480;

// ---------------------------------------------------------------- I2C helpers
namespace c6i2c {
inline bool write(uint8_t addr, uint8_t reg, uint8_t val) {
  Wire.beginTransmission(addr);
  Wire.write(reg);
  Wire.write(val);
  return Wire.endTransmission() == 0;
}
inline bool read(uint8_t addr, uint8_t reg, uint8_t *buf, size_t n) {
  Wire.beginTransmission(addr);
  Wire.write(reg);
  if (Wire.endTransmission(false) != 0) return false;
  if (Wire.requestFrom(addr, (uint8_t)n) != n) return false;
  for (size_t i = 0; i < n; i++) buf[i] = Wire.read();
  return true;
}
inline int read8(uint8_t addr, uint8_t reg) {
  uint8_t v;
  return read(addr, reg, &v, 1) ? v : -1;
}
inline bool update(uint8_t addr, uint8_t reg, uint8_t mask, uint8_t val) {
  int v = read8(addr, reg);
  if (v < 0) return false;
  return write(addr, reg, (uint8_t)((v & ~mask) | (val & mask)));
}
inline bool present(uint8_t addr) {
  Wire.beginTransmission(addr);
  return Wire.endTransmission() == 0;
}
}  // namespace c6i2c

// ------------------------------------------------------------------- Display
// The panel is a CO5300/SH8601-class AMOLED. Black pixels are off, so a black
// background saves battery and looks best. The panel's power rail doubles as
// its reset line, so the PMIC must be up before the screen starts.
class C6Panel : public Arduino_CO5300 {
 public:
  explicit C6Panel(Arduino_DataBus *bus)
      : Arduino_CO5300(bus, GFX_NOT_DEFINED, 0, SCREEN_W, SCREEN_H, 0, 0, 0, 0) {}

  // This panel cannot rotate its picture, so the screen always stays upright.
  // Calls to setRotation() are ignored on purpose.
  void setRotation(uint8_t) override {
    Arduino_TFT::setRotation(0);
    _bus->beginWrite();
    _bus->writeC8D8(0x36, 0x30);
    _bus->endWrite();
  }

  // The panel only accepts drawing windows that start on an even row and
  // column and end on an odd one. Round every fill out to that grid so thin
  // lines and small shapes are never dropped (edges may grow by one pixel).
  void writeFillRectPreclipped(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) override {
    int16_t x0 = x & ~1, y0 = y & ~1;
    int16_t x1 = min<int16_t>((x + w - 1) | 1, SCREEN_W - 1);
    int16_t y1 = min<int16_t>((y + h - 1) | 1, SCREEN_H - 1);
    Arduino_TFT::writeFillRectPreclipped(x0, y0, x1 - x0 + 1, y1 - y0 + 1, color);
  }
  void writePixelPreclipped(int16_t x, int16_t y, uint16_t color) override {
    writeFillRectPreclipped(x, y, 1, 1, color);
  }

  // Remember the current font so text can be drawn crisply off-screen.
  void setFont(const uint8_t *font) { u8font = font; Arduino_GFX::setFont(font); }
  void setFont(const GFXfont *f = nullptr) { u8font = nullptr; Arduino_GFX::setFont(f); }
  const uint8_t *u8font = nullptr;

  void applyVendorSettings() {
    _bus->beginWrite();
    _bus->writeC8D8(0xFE, 0x20);
    _bus->writeC8D8(0x19, 0x10);
    _bus->writeC8D8(0x1C, 0xA0);
    _bus->writeC8D8(0xFE, 0x00);
    _bus->writeC8D8(0x35, 0x00);
    _bus->endWrite();
  }
};

// ---------------------------------------------------------------------- Power
class C6Power {
 public:
  bool begin() {
    using namespace c6pins;
    if (c6i2c::read8(ADDR_PMIC, 0x03) != 0x4A) return false;  // AXP2101 chip id
    // Supply rails ALDO1-4 at 3.3 V, as Waveshare's own firmware sets them; a
    // power-chip reset (for example a long PWR press) returns them to 3.0/1.8 V.
    for (uint8_t reg = 0x92; reg <= 0x95; reg++)
      if (c6i2c::read8(ADDR_PMIC, reg) != 0x1C) c6i2c::write(ADDR_PMIC, reg, 0x1C);
    // Charger settings for the fitted 1000 mAh cell (verified on the device).
    c6i2c::update(ADDR_PMIC, 0x64, 0x07, 0x03);  // charge to 4.20 V
    c6i2c::update(ADDR_PMIC, 0x62, 0x1F, 0x09);  // 300 mA charge current
    c6i2c::update(ADDR_PMIC, 0x63, 0x1F, 0x11);  // stop at 25 mA
    c6i2c::update(ADDR_PMIC, 0x61, 0x0F, 0x02);  // 50 mA pre-charge
    c6i2c::update(ADDR_PMIC, 0x24, 0x07, 0x04);  // power off below 3.0 V
    c6i2c::update(ADDR_PMIC, 0x30, 0x01, 0x01);  // battery voltage ADC on
    c6i2c::update(ADDR_PMIC, 0x18, 0x02, 0x02);  // charging enabled
    c6i2c::update(ADDR_PMIC, 0x41, 0x0C, 0x0C);  // report PWR short/long press
    c6i2c::write(ADDR_PMIC, 0x49, 0x0C);         // clear stale presses
    return true;
  }
  // Power-cycling ALDO3 resets the display.
  void resetDisplay() {
    using namespace c6pins;
    c6i2c::update(ADDR_PMIC, 0x90, 0x04, 0x04);
    delay(100);
    c6i2c::update(ADDR_PMIC, 0x90, 0x04, 0x00);
    delay(100);
    c6i2c::update(ADDR_PMIC, 0x90, 0x04, 0x04);
    delay(100);
  }
  bool batteryPresent() { return regBit(0x00, 3); }
  bool usbPowered() { return regBit(0x00, 5); }
  bool charging() {
    int v = c6i2c::read8(c6pins::ADDR_PMIC, 0x01);
    return v >= 0 && ((v >> 5) & 0x03) == 1;
  }
  // Battery level 0-100, or -1 without a battery.
  int percent() {
    if (!batteryPresent()) return -1;
    int v = c6i2c::read8(c6pins::ADDR_PMIC, 0xA4);
    return v < 0 ? -1 : constrain(v, 0, 100);
  }
  float volts() {
    uint8_t b[2];
    if (!c6i2c::read(c6pins::ADDR_PMIC, 0x34, b, 2)) return 0;
    return (((b[0] & 0x1F) << 8) | b[1]) / 1000.0f;
  }
  // PWR button (middle). A short press is reported once; holding it for
  // about 6 seconds switches the device off in hardware.
  bool pwrPressed() { return takeIrq(0x08); }
  bool pwrLongPressed() { return takeIrq(0x04); }
  void powerOff() { c6i2c::update(c6pins::ADDR_PMIC, 0x10, 0x01, 0x01); }

 private:
  bool regBit(uint8_t reg, int b) {
    int v = c6i2c::read8(c6pins::ADDR_PMIC, reg);
    return v >= 0 && (v >> b) & 1;
  }
  bool takeIrq(uint8_t mask) {
    int v = c6i2c::read8(c6pins::ADDR_PMIC, 0x49);
    if (v < 0 || !(v & mask)) return false;
    c6i2c::write(c6pins::ADDR_PMIC, 0x49, mask);
    return true;
  }
};

// ---------------------------------------------------------------------- Touch
class C6Touch {
 public:
  int x = 0, y = 0;  // last touch point in screen pixels (0-479)

  void begin() {
    pinMode(c6pins::TOUCH_RST, OUTPUT);
    digitalWrite(c6pins::TOUCH_RST, HIGH);
    delay(20);
    digitalWrite(c6pins::TOUCH_RST, LOW);
    delay(20);
    digitalWrite(c6pins::TOUCH_RST, HIGH);
    delay(200);
  }

  void poll() {
    bool down = readPoint();
    uint32_t now = millis();
    tappedFlag_ = false;
    if (down && !down_) { downAt_ = now; startX_ = x; startY_ = y; }
    if (!down && down_ && now - downAt_ < 400 &&
        abs(x - startX_) < 40 && abs(y - startY_) < 40) tappedFlag_ = true;
    if (!down && down_) { endX_ = x; endY_ = y; releasedFlag_ = true; }
    down_ = down;
  }
  bool pressed() const { return down_; }          // finger is on the screen now
  bool tapped() const { return tappedFlag_; }     // a quick tap just finished
  // A swipe that just finished: returns 'L','R','U','D' or 0.
  char swipe() {
    if (!releasedFlag_) return 0;
    releasedFlag_ = false;
    int dx = endX_ - startX_, dy = endY_ - startY_;
    if (abs(dx) < 80 && abs(dy) < 80) return 0;
    if (abs(dx) > abs(dy)) return dx > 0 ? 'R' : 'L';
    return dy > 0 ? 'D' : 'U';
  }

 private:
  bool readPoint() {
    uint8_t d[10] = {0};
    Wire.beginTransmission(c6pins::ADDR_TOUCH);
    Wire.write(0xD0);
    Wire.write(0x00);
    if (Wire.endTransmission() != 0) return false;  // this chip needs a full stop
    delayMicroseconds(2000);                          // and a pause before reading
    if (Wire.requestFrom(c6pins::ADDR_TOUCH, (uint8_t)10) != 10) return false;
    for (int i = 0; i < 10; i++) d[i] = Wire.read();
    if (d[6] != 0xAB || (d[5] & 0x7F) == 0 || (d[0] & 0x0F) != 0x06) return false;
    int rx = (d[1] << 4) | (d[3] >> 4);
    int ry = (d[2] << 4) | (d[3] & 0x0F);
    // Map the touch chip's axes onto the screen.
    x = constrain(SCREEN_W - 1 - ry, 0, SCREEN_W - 1);
    y = constrain(rx, 0, SCREEN_H - 1);
    return true;
  }
  bool down_ = false, tappedFlag_ = false, releasedFlag_ = false;
  uint32_t downAt_ = 0;
  int startX_ = 0, startY_ = 0, endX_ = 0, endY_ = 0;
};

// ---------------------------------------------------------------- Motion (IMU)
// QMI8658 accelerometer + gyroscope. Units: g (1 g = gravity) and degrees/s.
// Axes follow the screen: x points right, y points to the top edge, z points
// out of the screen. Lying flat, screen up: az is about +1. Standing upright
// on its bottom edge: ay is about +1.
class C6Motion {
 public:
  float ax = 0, ay = 0, az = 1, gx = 0, gy = 0, gz = 0;

  bool begin() {
    using namespace c6pins;
    if (c6i2c::read8(ADDR_IMU, 0x00) != 0x05) return false;
    c6i2c::write(ADDR_IMU, 0x60, 0xB0);  // soft reset
    delay(50);
    c6i2c::read8(ADDR_IMU, 0x4D);
    c6i2c::write(ADDR_IMU, 0x02, 0x60);  // auto-increment addresses
    c6i2c::write(ADDR_IMU, 0x03, 0x26);  // accel +/-8 g
    c6i2c::write(ADDR_IMU, 0x04, 0x56);  // gyro +/-512 dps
    c6i2c::write(ADDR_IMU, 0x08, 0x03);  // accel + gyro on
    delay(100);                          // sensors need time before valid data
    return true;
  }

  void poll() {
    uint8_t b[12];
    shakenFlag_ = false;
    if (!c6i2c::read(c6pins::ADDR_IMU, 0x35, b, 12)) return;
    int16_t r[6];
    for (int i = 0; i < 6; i++) r[i] = (int16_t)(b[2 * i] | (b[2 * i + 1] << 8));
    if (r[0] == (int16_t)0x8000 && r[1] == (int16_t)0x8000) return;  // not ready
    // The sensor faces into the case: flip x and z so axes match the screen.
    ax = -r[0] / 4096.0f; ay = r[1] / 4096.0f; az = -r[2] / 4096.0f;
    gx = -r[3] / 64.0f;   gy = r[4] / 64.0f;   gz = -r[5] / 64.0f;

    // Shake: several strong jolts within a short window.
    float jolt = fabsf(sqrtf(ax * ax + ay * ay + az * az) - 1.0f);
    uint32_t now = millis();
    if (jolt > shakeThreshold) {
      if (now - lastJolt_ > 60) { jolts_++; lastJolt_ = now; }
    }
    if (now - lastJolt_ > 500) jolts_ = 0;
    if (jolts_ >= 3 && now - lastShake_ > 1000) {
      shakenFlag_ = true;
      lastShake_ = now;
      jolts_ = 0;
    }
  }

  float shakeThreshold = 0.8f;  // g; lower = more sensitive
  bool shaken() const { return shakenFlag_; }  // true once per shake
  // Tilt in degrees, 0,0 when flat. pitch > 0: top edge raised.
  // roll > 0: right edge raised.
  float pitch() const { return atan2f(ay, az) * 57.2958f; }
  float roll() const { return atan2f(ax, az) * 57.2958f; }
  bool faceDown() const { return az < -0.8f; }
  bool faceUp() const { return az > 0.8f; }
  bool moving() const { return fabsf(gx) + fabsf(gy) + fabsf(gz) > 30; }

 private:
  bool shakenFlag_ = false;
  int jolts_ = 0;
  uint32_t lastJolt_ = 0, lastShake_ = 0;
};

// -------------------------------------------------------------------- Buttons
class C6Button {
 public:
  explicit C6Button(int pin) : pin_(pin) {}
  void begin() { pinMode(pin_, INPUT_PULLUP); }
  void poll() {
    bool raw = digitalRead(pin_) == LOW;
    uint32_t now = millis();
    pressedFlag_ = releasedFlag_ = longFlag_ = false;
    if (raw != last_ && now - changedAt_ > 25) {
      last_ = raw;
      changedAt_ = now;
      if (raw) { pressedFlag_ = true; longSent_ = false; }
      else releasedFlag_ = true;
    }
    if (last_ && !longSent_ && now - changedAt_ > 800) { longFlag_ = true; longSent_ = true; }
  }
  bool isDown() const { return last_; }
  bool wasPressed() const { return pressedFlag_; }
  bool wasReleased() const { return releasedFlag_; }
  bool wasLongPressed() const { return longFlag_; }  // held ~0.8 s

 private:
  int pin_;
  bool last_ = false, pressedFlag_ = false, releasedFlag_ = false, longFlag_ = false,
       longSent_ = false;
  uint32_t changedAt_ = 0;
};

// ---------------------------------------------------------------------- Audio
// ES8311 drives the speaker, ES7210 reads the two microphones. Samples are
// signed 16-bit. Default rate 16000 Hz; setSampleRate() accepts 8000, 16000,
// 22050, 24000, 44100 or 48000.
class C6Audio {
 public:
  bool begin(uint32_t rate = 16000) {
    using namespace c6pins;
    rate_ = rate;
    i2s_.setPins(I2S_BCLK, I2S_WS, I2S_DOUT, I2S_DIN, I2S_MCLK);
    if (!i2s_.begin(I2S_MODE_STD, rate, I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_STEREO))
      return false;
    ok_ = initDac() && initMics();
    setVolume(volume_);
    return ok_;
  }
  bool ready() const { return ok_; }

  bool setSampleRate(uint32_t rate) {
    if (rate == rate_) return true;
    i2s_.end();
    rate_ = rate;
    return begin(rate);
  }
  uint32_t sampleRate() const { return rate_; }

  // 0 = silent, 100 = loudest.
  void setVolume(int v) {
    volume_ = constrain(v, 0, 100);
    uint8_t reg = volume_ == 0 ? 0 : (uint8_t)(0xBF - (100 - volume_));  // 0.5 dB steps
    c6i2c::write(c6pins::ADDR_DAC, 0x32, reg);
  }

  // Play a tone (blocks for its duration). freq in Hz.
  void tone(float freq, uint32_t ms) {
    const int N = 256;
    int16_t buf[N * 2];
    uint32_t total = rate_ * ms / 1000;
    float ph = 0, step = 2.0f * PI * freq / rate_;
    for (uint32_t done = 0; done < total; done += N) {
      int n = min<uint32_t>(N, total - done);
      for (int i = 0; i < n; i++) {
        // short fade in/out avoids clicks
        float env = min(1.0f, min((float)(done + i), (float)(total - done - i)) / (rate_ * 0.005f));
        int16_t s = (int16_t)(sinf(ph) * 12000 * env);
        ph += step;
        if (ph > 2 * PI) ph -= 2 * PI;
        buf[2 * i] = buf[2 * i + 1] = s;
      }
      i2s_.write((uint8_t *)buf, n * 4);
    }
  }
  void beep() { tone(1760, 80); }
  void chime() { tone(1319, 120); tone(1760, 180); }

  // Play mono 16-bit samples at the current sample rate (blocks).
  void play(const int16_t *samples, size_t count) {
    int16_t buf[512];
    for (size_t i = 0; i < count;) {
      size_t n = min<size_t>(256, count - i);
      for (size_t k = 0; k < n; k++) buf[2 * k] = buf[2 * k + 1] = samples[i + k];
      i2s_.write((uint8_t *)buf, n * 4);
      i += n;
    }
  }
  // Play a whole .wav file held in memory (16-bit PCM, mono or stereo).
  bool playWav(const uint8_t *wav, size_t len) {
    if (len < 44 || memcmp(wav, "RIFF", 4) || memcmp(wav + 8, "WAVE", 4)) return false;
    size_t p = 12;
    uint16_t channels = 1, bits = 16;
    uint32_t rate = 16000;
    while (p + 8 <= len) {
      uint32_t sz = wav[p + 4] | wav[p + 5] << 8 | wav[p + 6] << 16 | (uint32_t)wav[p + 7] << 24;
      if (!memcmp(wav + p, "fmt ", 4)) {
        channels = wav[p + 10] | wav[p + 11] << 8;
        rate = wav[p + 12] | wav[p + 13] << 8 | wav[p + 14] << 16 | (uint32_t)wav[p + 15] << 24;
        bits = wav[p + 22] | wav[p + 23] << 8;
      } else if (!memcmp(wav + p, "data", 4)) {
        if (bits != 16 || !setSampleRate(rate)) return false;
        const uint8_t *d = wav + p + 8;
        size_t bytes = min<size_t>(sz, len - p - 8);
        if (channels == 2) i2s_.write((uint8_t *)d, bytes);
        else play((const int16_t *)d, bytes / 2);
        return true;
      }
      p += 8 + sz + (sz & 1);
    }
    return false;
  }

  // Record mono samples from the microphones (blocks until count are read).
  size_t record(int16_t *out, size_t count) {
    int16_t buf[512];
    size_t got = 0;
    while (got < count) {
      size_t want = min<size_t>(256, count - got);
      size_t n = i2s_.readBytes((char *)buf, want * 4) / 4;
      if (n == 0) break;
      for (size_t k = 0; k < n; k++) out[got + k] = (int16_t)(((int)buf[2 * k] + buf[2 * k + 1]) / 2);
      got += n;
    }
    return got;
  }
  // Loudness over the next `ms` milliseconds: 0.0 (silence) to 1.0 (very loud).
  float level(uint32_t ms = 50) {
    int16_t buf[256];
    size_t total = rate_ * ms / 1000;
    double sum = 0;
    size_t n = 0;
    while (n < total) {
      size_t got = record(buf, min<size_t>(256, total - n));
      if (!got) break;
      for (size_t i = 0; i < got; i++) sum += (double)buf[i] * buf[i];
      n += got;
    }
    if (!n) return 0;
    return min(1.0, sqrt(sum / n) / 8000.0);
  }
  // Microphone gain in dB, 0-37.
  void setMicGain(float db) {
    uint8_t g = db < 33 ? (uint8_t)((db + 0.5f) / 3) : (db < 34.5f ? 10 : (db < 36 ? 11 : (db < 37 ? 12 : 13)));
    c6i2c::update(c6pins::ADDR_MICADC, 0x43, 0x0F, g);
    c6i2c::update(c6pins::ADDR_MICADC, 0x44, 0x0F, g);
  }
  I2SClass &i2s() { return i2s_; }  // raw access for streaming

 private:
  bool initDac() {
    using namespace c6i2c;
    const uint8_t A = c6pins::ADDR_DAC;
    bool ok = write(A, 0x44, 0x08);
    ok = write(A, 0x44, 0x08) && ok;
    uint8_t seq[][2] = {{0x01, 0x30}, {0x02, 0x00}, {0x03, 0x10}, {0x16, 0x24}, {0x04, 0x10},
                        {0x05, 0x00}, {0x0B, 0x00}, {0x0C, 0x00}, {0x10, 0x1F}, {0x11, 0x7F},
                        {0x00, 0x80}, {0x01, 0x3F}};
    for (auto &s : seq) ok = write(A, s[0], s[1]) && ok;
    update(A, 0x06, 0x20, 0x00);
    write(A, 0x13, 0x10);
    write(A, 0x1B, 0x0A);
    write(A, 0x1C, 0x6A);
    write(A, 0x44, 0x08);
    // 16-bit standard I2S, MCLK = 256 x sample rate
    write(A, 0x09, 0x0C);
    write(A, 0x0A, 0x4C);  // ADC output off: the ES7210 supplies microphone data
    write(A, 0x02, 0x00);
    write(A, 0x05, 0x00);
    write(A, 0x03, 0x10);
    write(A, 0x04, rate_ <= 16000 ? 0x20 : 0x10);
    update(A, 0x07, 0x3F, 0x00);
    write(A, 0x08, 0xFF);
    update(A, 0x06, 0x1F, 0x03);
    // start, DAC only
    write(A, 0x00, 0x80);
    write(A, 0x01, 0x3F);
    write(A, 0x17, 0xBF);
    write(A, 0x0E, 0x02);
    write(A, 0x12, 0x00);
    write(A, 0x14, 0x1A);
    write(A, 0x0D, 0x01);
    write(A, 0x15, 0x40);
    write(A, 0x37, 0x08);
    write(A, 0x45, 0x00);
    write(A, 0x31, 0x00);  // unmute
    return ok;
  }
  bool initMics() {
    using namespace c6i2c;
    const uint8_t A = c6pins::ADDR_MICADC;
    bool ok = write(A, 0x00, 0xFF);
    uint8_t seq[][2] = {{0x00, 0x41}, {0x01, 0x3F}, {0x09, 0x30}, {0x0A, 0x30}, {0x23, 0x2A},
                        {0x22, 0x0A}, {0x20, 0x0A}, {0x21, 0x2A}, {0x08, 0x00}, {0x40, 0x43},
                        {0x41, 0x70}, {0x42, 0x70}, {0x07, 0x20}, {0x02, 0xC1},
                        {0x11, 0x60},  // 16-bit, standard I2S
                        {0x12, 0x00},  // no TDM
                        {0x06, 0x00}, {0x47, 0x08}, {0x48, 0x08}, {0x49, 0x08}, {0x4A, 0x08},
                        {0x4B, 0x00}, {0x4C, 0xFF}, {0x01, 0x34},  // clocks for mics 1+2
                        {0x43, 0x1A}, {0x44, 0x1A},                // mic 1+2 on, 30 dB
                        {0x40, 0x43}, {0x00, 0x71}, {0x00, 0x41}};
    for (auto &s : seq) ok = write(A, s[0], s[1]) && ok;
    return ok;
  }
  I2SClass i2s_;
  uint32_t rate_ = 16000;
  int volume_ = 70;
  bool ok_ = false;
};

// ------------------------------------------------------------------ Clock
// PCF85063 real-time clock keeps time while the device is powered by battery.
// board.begin() copies it into the system clock, so time()/localtime() work.
class C6Clock {
 public:
  bool readRTC(struct tm &t) {
    uint8_t b[7];
    if (!c6i2c::read(c6pins::ADDR_RTC, 0x04, b, 7)) return false;
    if (b[0] & 0x80) return false;  // oscillator stopped: time never set
    t = {};
    t.tm_sec = bcd(b[0] & 0x7F);
    t.tm_min = bcd(b[1] & 0x7F);
    t.tm_hour = bcd(b[2] & 0x3F);
    t.tm_mday = bcd(b[3] & 0x3F);
    t.tm_wday = b[4] & 0x07;
    t.tm_mon = bcd(b[5] & 0x1F) - 1;
    t.tm_year = bcd(b[6]) + 100;
    return true;
  }
  bool writeRTC(const struct tm &t) {
    Wire.beginTransmission(c6pins::ADDR_RTC);
    Wire.write(0x04);
    Wire.write(dec(t.tm_sec));
    Wire.write(dec(t.tm_min));
    Wire.write(dec(t.tm_hour));
    Wire.write(dec(t.tm_mday));
    Wire.write(t.tm_wday);
    Wire.write(dec(t.tm_mon + 1));
    Wire.write(dec(t.tm_year % 100));
    return Wire.endTransmission() == 0;
  }
  // RTC holds UTC. Returns false if the RTC was never set.
  bool loadIntoSystem() {
    struct tm t;
    if (!readRTC(t)) return false;
    setenv("TZ", "UTC0", 1);
    tzset();
    time_t secs = mktime(&t);
    timeval tv = {secs, 0};
    settimeofday(&tv, nullptr);
    applyTimezone();
    return true;
  }
  void saveFromSystem() {
    time_t now = time(nullptr);
    struct tm t;
    gmtime_r(&now, &t);
    writeRTC(t);
  }
  // POSIX timezone, e.g. "EST5EDT,M3.2.0,M11.1.0" (New York), "GMT0BST,M3.5.0/1,M10.5.0" (London).
  void setTimezone(const char *tz) { tz_ = tz; applyTimezone(); }
  // Needs Wi-Fi. Sets the system clock and the RTC from the internet.
  bool syncFromInternet(uint32_t timeoutMs = 10000) {
    configTzTime(tz_.c_str(), "pool.ntp.org", "time.google.com");
    uint32_t start = millis();
    while (time(nullptr) < 1700000000 && millis() - start < timeoutMs) delay(100);
    if (time(nullptr) < 1700000000) return false;
    saveFromSystem();
    return true;
  }
  bool isSet() const { return time(nullptr) > 1700000000; }

 private:
  void applyTimezone() { setenv("TZ", tz_.c_str(), 1); tzset(); }
  static int bcd(uint8_t v) { return (v >> 4) * 10 + (v & 0x0F); }
  static uint8_t dec(int v) { return ((v / 10) << 4) | (v % 10); }
  String tz_ = "UTC0";
};

// ---------------------------------------------------------------------- Board
class C6Board {
 public:
  C6Panel *screen = nullptr;  // draw with Adafruit-GFX-style calls
  C6Touch touch;
  C6Motion motion;
  C6Button key{c6pins::KEY_BUTTON};    // +/KEY button (third)
  C6Button boot{c6pins::BOOT_BUTTON};  // BOOT button (first)
  C6Power power;
  C6Audio audio;
  C6Clock clock;

  // Starts everything. Pass audio=false to leave the speaker/mics off.
  bool begin(bool withAudio = true) {
    Serial.begin(115200);
    Wire.begin(c6pins::I2C_SDA, c6pins::I2C_SCL, 400000);
    bool ok = true;
    if (!power.begin()) { Serial.println("[board] power chip not found"); ok = false; }
    power.resetDisplay();

    bus_ = new Arduino_ESP32QSPI(c6pins::LCD_CS, c6pins::LCD_SCK, c6pins::LCD_D0,
                                 c6pins::LCD_D1, c6pins::LCD_D2, c6pins::LCD_D3);
    panel_ = new C6Panel(bus_);
    if (!panel_->begin(40000000)) { Serial.println("[board] display failed"); ok = false; }
    panel_->applyVendorSettings();
    panel_->setRotation(0);
    panel_->fillScreen(RGB565_BLACK);
    screen = panel_;
    setBrightness(255);

    touch.begin();
    if (!motion.begin()) { Serial.println("[board] motion sensor not found"); ok = false; }
    key.begin();
    boot.begin();
    clock.loadIntoSystem();
    if (withAudio && !audio.begin()) { Serial.println("[board] audio failed"); ok = false; }
    Serial.printf("[board] ready=%d, free memory %u bytes\n", ok, (unsigned)ESP.getFreeHeap());
    return ok;
  }

  // Call once at the top of every loop(). Reads touch, buttons and motion.
  void update() {
    touch.poll();
    motion.poll();
    key.poll();
    boot.poll();
  }

  // 0 (dark) to 255 (brightest). Lower brightness saves battery.
  void setBrightness(uint8_t b) {
    bus_->beginWrite();
    bus_->writeC8D8(0x51, b);
    bus_->endWrite();
  }
  // Turn the screen off/on (the app keeps running).
  void screenOff() { bus_->sendCommand(0x28); }
  void screenOn() { bus_->sendCommand(0x29); }

  // Crisp text in the current font (screen->setFont), drawn on a solid
  // background box. text(): top-left corner at (x, y). centerText(): centred
  // on (cx, cy). size scales the font (1 = normal).
  void text(const String &s, int x, int y, uint16_t color, uint16_t bg = RGB565_BLACK,
            uint8_t size = 1) {
    drawTextBox(s, x, y, false, size, color, bg);
  }
  void centerText(const String &s, int cx, int cy, uint8_t size, uint16_t color,
                  uint16_t bg = RGB565_BLACK) {
    drawTextBox(s, cx, cy, true, size, color, bg);
  }

 private:
  // Renders into a small off-screen canvas aligned to the panel's 2-pixel
  // grid, then sends it in one piece: pixel-perfect text on any background.
  void drawTextBox(const String &s, int x, int y, bool centred, uint8_t size, uint16_t color,
                   uint16_t bg) {
    int16_t bx, by;
    uint16_t w, h;
    screen->setTextSize(size);
    screen->getTextBounds(s.c_str(), 0, 0, &bx, &by, &w, &h);
    if (!w || !h) return;
    int left = centred ? x - (int)w / 2 : x, top = centred ? y - (int)h / 2 : y;
    int x0 = max(0, (left - 1) & ~1), y0 = max(0, (top - 1) & ~1);
    int x1 = min(SCREEN_W - 1, (left + (int)w) | 1), y1 = min(SCREEN_H - 1, (top + (int)h) | 1);
    if (x1 <= x0 || y1 <= y0) return;
    Arduino_Canvas box(x1 - x0 + 1, y1 - y0 + 1, screen, x0, y0);
    if (!box.begin(GFX_SKIP_OUTPUT_BEGIN)) {  // not enough memory: draw directly
      screen->setTextColor(color);
      screen->setCursor(left - bx, top - by);
      screen->print(s);
      return;
    }
    box.fillScreen(bg);
    if (screen->u8font) box.setFont(screen->u8font);
    box.setTextSize(size);
    box.setTextColor(color);
    box.setCursor(left - bx - x0, top - by - y0);
    box.print(s);
    box.flush();
  }
  Arduino_DataBus *bus_ = nullptr;
  C6Panel *panel_ = nullptr;
};

inline C6Board board;
