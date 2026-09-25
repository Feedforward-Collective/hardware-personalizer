# Hardware personalizer

Turn an idea into a working app on the **ESP32-C6 device** (Waveshare ESP32-C6-Touch-AMOLED-2.16), with an AI as your guide. The AI talks through what you want, helps you shape it, writes the code, and walks you through putting it on the device. No programming experience needed.

You need: the device, a USB-C **data** cable, a Mac or Windows computer, and one of the AI apps below.

## Pick how you want to use it

| You use | Get | Install |
|---|---|---|
| **Claude Code** | the `hardware-personalizer` folder | Copy it to `~/.claude/skills/hardware-personalizer/` |
| **Codex** | the `hardware-personalizer` folder | Copy it to `~/.agents/skills/hardware-personalizer/` |
| **claude.ai** or the **Claude app** | `dist/hardware-personalizer.zip` | Turn on code execution (**Settings → Capabilities**), then **Settings → Customize → Skills → Upload** and choose the ZIP (paid plans) |
| **ChatGPT** (web or app) | `dist/hardware-personalizer.zip` | **Skills → Create → Upload from your computer**, choose the ZIP |
| **Any AI, no skills** | `dist/PROMPT.md` | Start a new chat, attach `PROMPT.md` (or paste all of it) |

Then just say what you want, for example: *"I want to make something with my ESP32-C6 device"* or *"Help me build a focus timer for my desk gadget."*

- **Claude Code and Codex** can install the tools and put the app on the device for you; you plug it in and watch.
- **Chat apps** walk you through the free Arduino IDE, one step at a time. The first setup takes about 15 minutes; after that, each new app is copy, paste, upload.

## What's inside

```
hardware-personalizer/            the skill
  SKILL.md                        the AI's playbook: discover, refine, build, flash
  references/ideas.md             ideas, real projects, and what the device can't do
  references/device.md            the device and its code API (the AI's coding contract)
  references/flash-agent.md       set up and flash from Claude Code or Codex
  references/flash-by-hand.md     set up and flash with the Arduino IDE
  references/troubleshooting.md   fixes for common problems
  starter/ESP32C6_App/            the starter app (every new app starts here)
  starter/SelfTest/               checks every part of the device
  starter/Demo/                   animation, Wi-Fi, recording and tilt examples
  starter/Tour/                   swipeable on-device tour of 8 mini demos (built on request)
dist/                             built files: the ZIP and PROMPT.md
build.py                          rebuilds dist/ after any change
```

## For maintainers

- `starter/ESP32C6_App/esp32c6_board.h` is the single source for the board support file. After editing it or any reference, run `python3 build.py` to copy it into the other sketches and rebuild the ZIP and `PROMPT.md`.
- Everything in `esp32c6_board.h` and `device.md` was checked on a real device (September 2026, Arduino-ESP32 core 3.3.12, GFX Library for Arduino 1.6.8, U8g2 2.36). Not yet tested: Windows flashing, HTTPS memory use, battery life, the SD card slot.
- Keep all code out of the `.ino` files. Arduino only runs its Intel-only `ctags` tool on `.ino` code, so this keeps builds working on Apple-silicon Macs without Rosetta.

## License

MIT; see [LICENSE](LICENSE) and [NOTICE](NOTICE).
