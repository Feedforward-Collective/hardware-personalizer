# Troubleshooting

Find the symptom, try the fix, and ask the person what they see now. Ask for exact error text (a photo or copy-paste of the output panel) rather than a summary.

## Connecting and uploading

| Symptom | Fix |
|---|---|
| No port appears, or `No serial data received` / `Failed to connect` | 1) Use a USB-C **data** cable; many cables only charge. 2) Try another USB port, directly on the computer rather than a hub. 3) Run the upload again: uploading resets the chip over USB and recovers a device that looks dead or frozen. |
| Several ports listed | Unplug the device, note which port disappears, plug back in and choose that one. |
| Screen black or frozen, device seems dead | It is almost always still alive. Run the upload again: it resets the chip, and `board.begin()` restores the screen's power settings (a long PWR press resets the power chip and leaves the screen dark until the app restarts). |
| Upload works but "Hello!" (or the app) never appears | Open the serial monitor at 115200 baud and look for `[board] ready=1`. If it reads `ready=0`, the line before it names the part that failed; unplug for 10 seconds and retry. |
| Windows: no COM port | Windows 10/11 normally installs the driver itself. Check Device Manager under **Ports (COM & LPT)** with the device plugged in; if it shows under "Other devices" with a warning, try another cable or port. (Windows is less tested than Mac with this device.) |

## Compile errors

| Error text contains | Cause and fix |
|---|---|
| `bad CPU type in executable` (Mac) | Code was put in the `.ino` file. Move it all into `app.cpp`, leaving the `.ino` as a comment. |
| `No such file or directory` for `Arduino_GFX_Library.h` or `U8g2lib.h` | Install **GFX Library for Arduino** or **U8g2** in the Library Manager. |
| `esp32c6_board.h: No such file` | The board file is missing from the sketch folder or has a different name. It must sit next to `app.cpp`. |
| `'u8g2_font_…' was not declared` | That font name doesn't exist. Use a name from the font list in `device.md`. |
| `'…' was not declared in this scope` for your own function | Move the function above the code that calls it (or declare it at the top). |
| `Sketch too big` | Choose **Partition Scheme → 16M Flash (3MB APP/9.9MB FATFS)** (and Flash Size 16MB) in the Tools menu; shrink embedded sounds or images. |
| Board or menu items missing | The ESP32 board package isn't installed or isn't version 3.3.x; redo setup step 3. |

## The app misbehaves

| Symptom | Cause and fix |
|---|---|
| Taps, buttons or shakes are ignored or only sometimes work | `board.update()` isn't first in `loop()`, or the loop is blocked by long `delay()`s, sound or network calls. Time things with `millis()`. |
| Device restarts over and over; serial shows `Guru Meditation` or `Stack canary` | Usually memory: a big array declared inside a function (move it to a global or `malloc` it once), or canvases and buffers exceeding the memory table in `device.md`. Print `ESP.getFreeHeap()`. |
| Smeared or overlapping text | Clear the area with `fillRect` in the background colour before redrawing. |
| Text or thin shapes missing, or a canvas/image garbled | Draw text with `board.text()` / `board.centerText()`; give canvases and bitmaps an even x, y, width and height (the panel updates on a 2-pixel grid). |
| Flicker during animation | Draw into an `Arduino_Canvas` and `flush()` once per frame (see `device.md`). |
| No sound | `board.begin()` was called with `false`, volume is 0, or `board.audio.ready()` is false (check serial for `[board] audio failed`). |
| Microphone thresholds wrong | Print `board.audio.level(50)` readings for a quiet room and for the target sound, then set the threshold between them. |
| Wi-Fi never connects | Check the name and password (case-sensitive); the device needs **2.4 GHz** Wi-Fi; networks with a sign-in web page or enterprise login don't work, so try a phone hotspot. |
| Web requests fail | Print the HTTP code. `-1` is a connection problem (Wi-Fi, URL, HTTPS); 401/403 means a wrong API key; 429 means polling too often. |
| Time is wrong | Call `board.clock.setTimezone(...)` with the right zone and `board.clock.syncFromInternet()` once Wi-Fi is up. |
| Battery drains fast | Lower `board.setBrightness()`, turn the screen off when idle, and turn Wi-Fi off between fetches. |

## Still stuck

Upload the self-test: the `starter/SelfTest` sketch (same three-file layout). It checks every part of the device on screen and over serial. Every line reading `ok` means the hardware is fine and the problem is in the app code; go back to the code with the serial output in hand.
