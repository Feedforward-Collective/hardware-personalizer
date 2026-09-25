# Building and flashing from an agent with a shell

You run the tools; the person plugs in the device and watches it. Explain each step in a sentence before running it, and ask before installing anything.

## 1. Where the project lives

Ask where they want to keep their device apps: a folder on this computer (suggest `~/Documents/Arduino/<AppName>`), a git repository, or elsewhere. Create the sketch folder `<AppName>/` there with:

- `<AppName>.ino` containing only the placeholder comment from `starter/ESP32C6_App/ESP32C6_App.ino` (Arduino requires the `.ino` to match the folder name)
- `esp32c6_board.h` copied unchanged from `starter/ESP32C6_App/`
- your `app.cpp`

If the device is plugged into a different machine from the one you run on (a cloud VM or remote container), build here and use **Remote flashing** below.

## 2. Tools (once per computer)

Check first: `arduino-cli version`, `arduino-cli core list`, `arduino-cli lib list`. Install only what is missing.

- **arduino-cli**: macOS `brew install arduino-cli` (if Homebrew is present); Windows `winget install ArduinoSA.CLI`; Linux or anywhere else, the official script from arduino.github.io/arduino-cli (installs to `./bin`).
- **ESP32 core** (3.3.x):
  ```sh
  arduino-cli config init            # skip if a config already exists
  arduino-cli config add board_manager.additional_urls https://espressif.github.io/arduino-esp32/package_esp32_index.json
  arduino-cli core update-index
  arduino-cli core install esp32:esp32@3.3.12   # or the newest 3.3.x
  ```
  The core download is several hundred MB and can take minutes; run it in the background if your tool allows.
- **Libraries**: `arduino-cli lib install "GFX Library for Arduino" U8g2` (plus any library the app needs).

## 3. Find the device

`arduino-cli board list`. The device appears as **ESP32 Family Device** (USB VID 0x303A): `/dev/cu.usbmodem…` on macOS, `/dev/ttyACM…` on Linux, `COM…` on Windows. If nothing appears, ask them to check the cable carries data and try another port (see `troubleshooting.md`).

## 4. Compile and upload

```sh
FQBN="esp32:esp32:esp32c6:CDCOnBoot=cdc,FlashSize=16M,PartitionScheme=app3M_fat9M_16MB"
arduino-cli compile -b "$FQBN" <AppName>                          # compile only: fix errors here
arduino-cli compile -b "$FQBN" -u -p <PORT> <AppName>             # compile + upload
```

Success ends with `Hash of data verified.` and `Hard resetting via RTS pin...`. Fix compile errors yourself and recompile; tell the person only what matters ("fixed a typo, uploading now").

## 5. Watch it run

Read the device's serial output for a few seconds after upload, to confirm it started and to diagnose problems:

```sh
sleep 10 | arduino-cli monitor -p <PORT> --config 115200 --quiet   # listens for 10 s
```

The monitor exits as soon as its input closes, so the piped `sleep` sets how long it listens (in Windows PowerShell, run it in its own terminal and stop it with Ctrl+C). It does not restart the app: start listening right after an upload to catch the startup lines.

`[board] ready=1` means every part of the device started. Then ask the person to try each feature while you watch the output.

## Remote flashing (device not attached to your machine)

Compile with `--output-dir build`. Send the person `build/<AppName>.ino.merged.bin` and walk them through the browser flasher:

1. Open **https://espressif.github.io/esptool-js/** in **Chrome** or **Edge** (Safari and Firefox cannot talk to USB devices).
2. Plug in the device, click **Connect**, pick the device in the popup (*USB JTAG/serial debug unit*).
3. Set **Flash Address** to `0x0`, choose the `.merged.bin` file, click **Program**, and wait for it to finish.
4. Unplug and replug the device (or press **PWR**) to start the app.
