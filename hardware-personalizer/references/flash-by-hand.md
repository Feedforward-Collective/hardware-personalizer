# Flashing by hand with the Arduino IDE (chat apps)

Guide the person through these steps **one at a time**, waiting for them to confirm each before giving the next. Name every button and menu exactly as written. If a step fails, switch to `troubleshooting.md`.

Part A happens once per computer. Part B happens for every app.

## Part A: one-time setup (about 15 minutes)

1. **Install the Arduino IDE.** Go to **arduino.cc/en/software** and download **Arduino IDE 2** for their computer.
   - Mac: open the downloaded file and drag **Arduino IDE** into **Applications**, then open it from Applications. If macOS asks whether to open an app from the internet, click **Open**.
   - Windows: run the installer and accept the defaults, including any driver prompts.
2. **Add the ESP32 boards.** Open **Settings** (Mac: **Arduino IDE → Settings…**; Windows: **File → Preferences…**). In **Additional boards manager URLs**, paste:
   `https://espressif.github.io/arduino-esp32/package_esp32_index.json`
   Click **OK**.
3. **Install the board package.** Click the **Boards Manager** icon in the left sidebar (the second icon, a circuit board). Search **esp32**, find **esp32 by Espressif Systems**, choose version **3.3.x** (the newest 3.3 release) and click **Install**. It is a large download; wait until it says **INSTALLED**.
4. **Install the two libraries.** Click the **Library Manager** icon (the third icon, books). Search and install:
   - **GFX Library for Arduino** by *Moon On Our Nation*
   - **U8g2** by *oliver*
   If asked to install dependencies, click **Install All**.
5. **Get the starter folder.** The person needs the folder `ESP32C6_App` containing `ESP32C6_App.ino`, `app.cpp` and `esp32c6_board.h` (the originals are in this skill's `starter/ESP32C6_App/`).
   - If you can create downloadable files, give them the three files (or a ZIP of the folder) to download.
   - Otherwise, paste each file in its own code block, and have them create a folder named `ESP32C6_App` in **Documents/Arduino** containing the three files with exactly those names. (Text editors: Mac **TextEdit** → **Format → Make Plain Text** before saving; Windows **Notepad**, "Save as type: All files".)
6. **Open the sketch.** In the Arduino IDE: **File → Open…**, choose `ESP32C6_App.ino`. Three tabs appear across the top: `ESP32C6_App.ino`, `app.cpp`, `esp32c6_board.h`.
7. **Plug in the device** with a USB-C **data** cable (a charge-only cable will not work).
8. **Choose the board and settings** in the **Tools** menu:

   | Tools menu item | Choose |
   |---|---|
   | **Board** | **esp32 → ESP32C6 Dev Module** |
   | **USB CDC On Boot** | **Enabled** |
   | **Flash Size** | **16MB (128Mb)** |
   | **Partition Scheme** | **16M Flash (3MB APP/9.9MB FATFS)** |
   | **Port** | Mac: **/dev/cu.usbmodem…**; Windows: a **COM** port, often labelled *USB JTAG/serial debug unit*. If several appear, unplug the device, see which disappears, plug it back in and choose that one. |

   The IDE remembers these settings for next time.
9. **Test upload.** Click the **Upload** button (the right-pointing arrow at top left). The first build takes a few minutes. Success looks like **"Hard resetting via RTS pin..."** at the end of the output panel, and the device shows **"Hello!"**. Tap it, shake it, press KEY to see it respond.

## Part B: putting a new app on the device

1. Click the **`app.cpp`** tab.
2. Select everything in it (**Cmd+A** on Mac, **Ctrl+A** on Windows) and delete it.
3. Paste the new `app.cpp` code you wrote.
4. If the app has extra files (for example `sound.h`), use the **⋯** menu at the right end of the tab bar → **New Tab**, give it the exact name, and paste its contents.
5. Fill in any placeholders you marked (Wi-Fi name and password, API keys).
6. Click **Upload** and wait for **"Hard resetting via RTS pin..."**.
7. Try the app on the device.

**To keep an app**: **File → Save As…** under a new name (for example `FocusFlip`). The IDE saves a copy of the whole folder, so each app keeps its own three files.

**To see debug messages**: **Tools → Serial Monitor**, set the speed at its top right to **115200 baud**. Messages from `Serial.printf` appear there; copying them back to you helps with diagnosis.

**To go back to Magic 8** (for devices that shipped with it): run its original installer again.
