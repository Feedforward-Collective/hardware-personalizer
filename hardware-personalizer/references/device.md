# ESP32-C6 device: coding reference

Everything here was verified on the device. Code that stays inside this reference runs first time; code that strays from it (other pins, other libraries, rotation, a full-screen frame buffer) usually fails on this hardware.

## The hardware

Waveshare **ESP32-C6-Touch-AMOLED-2.16**: ESP32-C6 chip (single 160 MHz core, 512 KB RAM, **no PSRAM**, 16 MB flash, Wi-Fi 6, Bluetooth LE 5), 480 × 480 AMOLED touch screen, QMI8658 motion sensor, ES8311 speaker chip with a fitted speaker, ES7210 with two microphones, PCF85063 clock, AXP2101 power chip, 1000 mAh battery, USB-C.

Buttons, counted from the BOOT end of the case: **BOOT** (usable in apps once running), **PWR** (power; short press readable by apps, ~6 s hold switches off), **KEY** (+/KEY, free for apps).

## Sketch layout

Every app is one Arduino sketch folder with three files:

| File | Contents |
|---|---|
| `ESP32C6_App.ino` | A comment only. The folder and `.ino` share a name; rename both together if you like. Code in the `.ino` needs extra tools that Apple-silicon Macs lack, so it stays empty. |
| `app.cpp` | The whole app: `#include "esp32c6_board.h"`, then `setup()` and `loop()`. Plain C++: declare a function above its first use. |
| `esp32c6_board.h` | Board support, copied unchanged from `starter/ESP32C6_App/`. |

Extra `.h` files in the same folder are fine (for example a sound clip or image as a `const` array). Libraries: **GFX Library for Arduino** and **U8g2** (both required; the board file includes them). Add another library only when the app truly needs it (for example **ArduinoJson** for parsing API replies), and tell the person to install it.

## The `board` object

```cpp
#include "esp32c6_board.h"

void setup() {
  board.begin();          // starts everything; board.begin(false) leaves audio off
}
void loop() {
  board.update();         // first line of every loop: reads touch, buttons, motion
  // ... react to events ...
  delay(5);
}
```

Events such as `tapped()`, `wasPressed()` and `shaken()` are true for exactly one `update()` after they happen, so check them every loop and keep each loop short (under ~50 ms). A long `delay()` or a blocking call makes the app miss taps and presses.

### Screen: `board.screen` (Arduino_GFX, Adafruit-GFX style)

Coordinates: (0,0) is top-left, (479,479) bottom-right. The screen always draws upright; `setRotation()` is ignored on purpose.

```cpp
auto *s = board.screen;
s->fillScreen(RGB565_BLACK);
s->fillRect(x, y, w, h, color);        s->drawRect(...);
s->fillRoundRect(x, y, w, h, r, color); s->fillCircle(x, y, r, color);
s->drawLine(x0, y0, x1, y1, color);    s->fillTriangle(x0, y0, x1, y1, x2, y2, color);
s->fillArc(cx, cy, rOuter, rInner, startDeg, endDeg, color);  // gauges and rings, see below
s->setFont(u8g2_font_helvB24_tr);      // smooth fonts, see the list below
board.text("Hi", x, y, color, bg);              // crisp text, top-left at (x, y), current font
board.centerText("Hello", cx, cy, 1, color, bg); // crisp text centred on (cx, cy)
board.setBrightness(0..255);  board.screenOff();  board.screenOn();
```

- **Arcs**: angles in degrees, 0° at 3 o'clock, increasing clockwise (so 270° is 12 o'clock). For a ring that starts at 12 o'clock and sweeps `d` degrees, draw from 270 to 270 + d, split into two calls where it passes 360 (270→360, then 0→rest). Equal start and end draw nothing; 0→360 draws a full ring.
- **Colours**: `RGB565_BLACK`, `RGB565_WHITE`, `RGB565_RED`, `RGB565_ORANGE`, `RGB565_GOLD`, `RGB565_GREEN`, `RGB565_CYAN`, `RGB565_BLUE`, `RGB565_MAGENTA`, `RGB565_DARKGREY` and the other web colour names, or `RGB565(r, g, b)` for any colour.
- **Fonts** (verified names): `u8g2_font_logisoso16_tr` … `u8g2_font_logisoso58_tr` (big, modern; even sizes 16–34, then 38, 42, 46, 50, 54, 58), `u8g2_font_helvR08_tr`…`helvR24_tr` and `helvB08_tr`…`helvB24_tr` (sizes 08, 10, 12, 14, 18, 24), `u8g2_font_fub11_tr`…`fub42_tr` and `fur…` (sizes 11, 14, 17, 20, 25, 30, 35, 42). The number in a name is roughly the text height in pixels; `centerText` measures text exactly, and `screen->getTextBounds()` gives its width and height before drawing. `_tr` has ASCII; `_tf` adds accented letters. With no `setFont`, the built-in 6×8 font is used, scaled by `setTextSize(n)`; blocky, fine for debug text.
- **Text**: draw all text with `board.text()` or `board.centerText()`. They render off-screen and paint a solid box in `bg` (default black) behind the letters, so pass the colour of whatever is underneath. The size argument scales the font (keep 1 for smooth fonts). Plain `setCursor()` + `print()` also works but looks slightly bolder.
- **Redrawing**: each text box overwrites only its own area. When new text is shorter than the old, clear the old area first (`fillRect` in the background colour). Redraw only what changed.
- **Panel grid**: the panel accepts updates only on a 2-pixel grid. Shapes are rounded out automatically (an edge may grow by one pixel); canvases and bitmaps need an even x, y, width and height.
- **Smooth animation**: draw into an off-screen canvas, then push it in one go (verified at 40 frames/s for 480×200):

  ```cpp
  Arduino_Canvas *canvas = new Arduino_Canvas(480, 200, board.screen, 0, 140); // w, h, target, x, y
  canvas->begin(GFX_SKIP_OUTPUT_BEGIN);   // once, in setup()
  canvas->fillScreen(RGB565_BLACK); /* draw with canvas-> ... */ canvas->flush();
  ```

  Give a canvas an even x, y, width and height. It costs w × h × 2 bytes of RAM. The full screen (450 KB) does not fit; see memory below.
- AMOLED: black pixels are off. Black backgrounds look best and save battery. Avoid a static bright image for hours (burn-in): return bright "result" screens to a darker idle screen after a minute or so, and dim or `screenOff()` while the screen can't be seen (face-down, idle for minutes).

### Touch: `board.touch`

`pressed()` finger is down now · `tapped()` a quick tap just ended · `swipe()` returns `'L'`, `'R'`, `'U'`, `'D'` once after a swipe, else 0 · `x`, `y` position of the last touch. One finger only; make touch targets at least 60 px.

### Buttons: `board.key` (+/KEY), `board.boot` (BOOT), and PWR

`wasPressed()` · `wasReleased()` · `wasLongPressed()` (held ~0.8 s) · `isDown()`. PWR: `board.power.pwrPressed()` for a short press (holding PWR ~6 s powers the device off in hardware; don't assign long-press actions to it).

### Motion: `board.motion`

Axes follow the screen: x right, y toward the top edge, z out of the screen; units g and degrees/second.
`ax, ay, az` (flat and face-up: az ≈ +1) · `gx, gy, gz` · `pitch()` degrees, positive when the top edge is raised · `roll()` positive when the right edge is raised · `faceUp()` · `faceDown()` · `moving()` · `shaken()` once per shake · `shakeThreshold` (default 0.8 g; lower is more sensitive).

- Treat an orientation as real once it has held for ~300 ms, and act on the change into it (a device that starts face-down shouldn't trigger a "flip").
- Turning the device over by hand can register as a shake. When an app uses both, ignore `shaken()` for about 1 second after an orientation change.
- Between `faceUp()` and `faceDown()` is "tilted": decide which side of the line the app treats it as.

### Sound: `board.audio`

Speaker and microphones share one audio link; calls **block** until done.
`beep()` · `chime()` · `tone(freqHz, ms)` · `setVolume(0..100)` (default 70) · `play(const int16_t *mono, count)` · `playWav(const uint8_t *wav, len)` (16-bit PCM WAV, mono or stereo; switches the sample rate to match) · `setSampleRate(hz)` (8000, 16000, 22050, 24000, 44100, 48000; default 16000) · `record(int16_t *buf, count)` mono samples · `level(ms)` loudness 0.0–1.0 over `ms` milliseconds (50 ms is a good default; print a few readings to calibrate thresholds for the room, since quiet rooms read near 0.01) · `setMicGain(0..37 dB)` (default 30) · `ready()`.

- Melodies: sequence `tone()` calls. Keep sound bursts short inside `loop()`, since the app does nothing else while sound plays.
- Recording: 1 second at 16 kHz = 32 KB of RAM. Allocate with `malloc` once, in `setup()`.
- A sound clip in the app: a WAV stored as `const uint8_t clip[] = {...};` in its own `.h` (make it with `xxd -i clip.wav`, then change `unsigned char` to `const uint8_t`). `const` data lives in flash, not RAM; keep clips short (16 kHz mono ≈ 32 KB per second; the app has room for about 2 MB).

### Power: `board.power`

`percent()` battery 0–100 (-1 if no battery) · `volts()` · `charging()` · `usbPowered()` · `powerOff()`.

### Time: `board.clock` and standard C time

`board.begin()` loads the battery-backed clock into the system clock, so `time()`, `localtime_r()` and `strftime()` work.
`setTimezone("EST5EDT,M3.2.0,M11.1.0")` (POSIX TZ string; London `GMT0BST,M3.5.0/1,M10.5.0`; Paris `CET-1CEST,M3.5.0,M10.5.0/3`; Los Angeles `PST8PDT,M3.2.0,M11.1.0`) · `syncFromInternet()` (needs Wi-Fi; also saves to the clock) · `isSet()`.

### Wi-Fi and the internet

Standard Arduino-ESP32 libraries: `WiFi.h`, `HTTPClient.h`, `NetworkClientSecure.h`.

```cpp
#include <WiFi.h>
#include <HTTPClient.h>
#include <NetworkClientSecure.h>

const char *WIFI_NAME = "your-network";   // <-- fill in
const char *WIFI_PASSWORD = "your-password";

bool connectWiFi() {
  WiFi.begin(WIFI_NAME, WIFI_PASSWORD);
  for (int i = 0; i < 40 && WiFi.status() != WL_CONNECTED; i++) delay(250);
  return WiFi.status() == WL_CONNECTED;
}

String fetch(const char *url) {              // HTTPS GET, returns the body or ""
  NetworkClientSecure client;
  client.setInsecure();                      // skip certificate checks: fine for public data
  HTTPClient http;
  if (!http.begin(client, url)) return "";
  int code = http.GET();
  String body = code == 200 ? http.getString() : "";
  http.end();
  return body;
}
```

- Show a "connecting…" screen, and a friendly error with a retry (tap or KEY) when Wi-Fi or a fetch fails.
- Poll gently (every 30 s or slower) unless the app needs faster; call APIs from `loop()` with a `millis()` timer, never in a tight loop.
- The device joins 2.4 GHz networks with a password. Captive-portal networks (hotels, conference Wi-Fi with a sign-in page) and enterprise sign-in do not work; use a phone hotspot.
- Secrets live in constants at the top of `app.cpp`. Remind the person not to share or publish that file with real passwords or keys in it.
- To receive data (from a helper script on their computer, a webhook relay, a phone), run `WebServer` from `WebServer.h` on the device and print its IP address on screen.

## Memory (measured)

| Situation | Free RAM |
|---|---|
| After `board.begin()` | ~400 KB |
| Same, in an app that includes Wi-Fi | ~350 KB |
| After Wi-Fi is turned on | ~295 KB |
| Each HTTPS request, while it runs | roughly 40–50 KB more (typical for ESP32 HTTPS; not yet measured on this device) |

Budget canvases and buffers from these: for example, a 480 × 200 canvas (192 KB) plus a 2-second recording (64 KB) fits without Wi-Fi; with Wi-Fi and HTTPS, keep canvases to about 480 × 100 (96 KB). Allocate big buffers once in `setup()`. Print `ESP.getFreeHeap()` to the serial monitor when in doubt.

## Storage

The flash has a 9.9 MB storage area for files (FAT). `#include <FFat.h>`, `FFat.begin(true)`, then `FFat.open("/log.txt", FILE_APPEND)`. For settings that must survive restarts, `Preferences.h` is simpler (`prefs.begin("app"); prefs.putInt("count", n);`).

## Code rules

- Use the `board` API above for all hardware. Leave pins, I2C addresses and init sequences to `esp32c6_board.h`.
- Standard Arduino-ESP32 3.x APIs (`WiFi`, `HTTPClient`, `WebServer`, `Preferences`, `FFat`, `millis()`) are fine.
- Keep `loop()` non-blocking: time things with `millis()`; use `delay()` only for short pauses (≤ 50 ms) or deliberate moments like a 1-second celebration screen. Sounds block like `delay()`: play them at state changes (a chime or melody up to ~1 s is fine there), never on every pass of the loop.
- Keep everything drawn inside the 480 × 480 screen.
- Arrays over about 1 KB are globals or allocated once with `malloc`; `loop()` runs on an 8 KB stack.
- Print a few `Serial.printf` lines at key moments (start, Wi-Fi status, errors) so problems can be diagnosed over USB.

## Checklist (review your code against every item)

- [ ] `app.cpp` includes `"esp32c6_board.h"` (after any header comment); the `.ino` is only a comment; `esp32c6_board.h` is unchanged.
- [ ] `board.begin()` in `setup()`; `board.update()` first in `loop()`.
- [ ] No `delay()` over 50 ms and no sound in the normal loop path (sounds only at state changes); timers use `millis()`.
- [ ] Every function is declared above its first use.
- [ ] Every font name appears in the verified font list above.
- [ ] Text uses `board.text()` / `board.centerText()` with `bg` matching what is underneath; shorter replacement text clears the old area first.
- [ ] Canvases and bitmaps have even x, y, width and height.
- [ ] RAM use (canvases + buffers) fits the memory table, with Wi-Fi counted if used.
- [ ] Wi-Fi and fetch failures show a message and can be retried; secrets are placeholders or clearly marked.
- [ ] Every feature on the app card is implemented. Glue that makes them reliable (debouncing, clearing, error messages) is expected; any new visible feature goes on the version-2 list instead.
