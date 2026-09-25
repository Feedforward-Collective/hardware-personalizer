#!/usr/bin/env python3
"""Build the hardware-personalizer distributables.

1. Copies starter/ESP32C6_App/esp32c6_board.h (the single source) into every
   other sketch folder.
2. Writes dist/hardware-personalizer.zip for claude.ai, the Claude app and
   ChatGPT skill uploads.
3. Writes dist/PROMPT.md: the same skill as one self-contained prompt for
   people who don't use skills.

Run: python3 build.py
"""
import re
import shutil
import zipfile
from pathlib import Path

ROOT = Path(__file__).resolve().parent
SKILL = ROOT / "hardware-personalizer"
STARTER = SKILL / "starter"
DIST = ROOT / "dist"

REFERENCES = [  # (file, section title in PROMPT.md)
    ("ideas.md", "Reference: ideas and limits"),
    ("device.md", "Reference: device and code"),
    ("flash-agent.md", "Reference: building and flashing from an agent"),
    ("flash-by-hand.md", "Reference: flashing by hand with the Arduino IDE"),
    ("troubleshooting.md", "Reference: troubleshooting"),
]
STARTER_FILES = ["ESP32C6_App/ESP32C6_App.ino", "ESP32C6_App/app.cpp",
                 "ESP32C6_App/esp32c6_board.h"]
TOUR_FILE = "Tour/app.cpp"


def sync_board_file():
    source = STARTER / "ESP32C6_App" / "esp32c6_board.h"
    for sketch in STARTER.iterdir():
        target = sketch / "esp32c6_board.h"
        if sketch.is_dir() and sketch.name != "ESP32C6_App":
            shutil.copyfile(source, target)


def build_zip():
    out = DIST / "hardware-personalizer.zip"
    with zipfile.ZipFile(out, "w", zipfile.ZIP_DEFLATED) as z:
        for path in sorted(SKILL.rglob("*")):
            if path.is_file() and path.name != ".DS_Store" and "build" not in path.relative_to(SKILL).parts:
                z.write(path, Path("hardware-personalizer") / path.relative_to(SKILL))
    return out


def body(md: str) -> str:
    """Strip YAML frontmatter and the first H1."""
    md = re.sub(r"\A---\n.*?\n---\n", "", md, flags=re.S)
    return re.sub(r"\A\s*# .*\n", "", md).strip()


def demote(md: str) -> str:
    """Push headings down one level so they nest under a section title."""
    return re.sub(r"^(#+) ", r"#\1 ", md, flags=re.M)


def retarget(md: str) -> str:
    """Rewrite skill-folder paths into references to sections of this prompt."""
    for name, title in REFERENCES:
        md = re.sub(r"\[`references/%s`\]\(references/%s\)" % (re.escape(name), re.escape(name)),
                    f"**{title}** (below)", md)
        md = md.replace(f"`references/{name}`", f"**{title}**").replace(f"`{name}`", f"**{title}**")
    md = md.replace("this skill's `starter/ESP32C6_App/`", "**Starter files** at the end of this prompt")
    md = md.replace("`starter/ESP32C6_App/`", "**Starter files** (end of this prompt)")
    md = md.replace("`starter/ESP32C6_App/app.cpp`", "the starter `app.cpp` (end of this prompt)")
    md = md.replace("the placeholder comment from `starter/ESP32C6_App/ESP32C6_App.ino`", "the placeholder comment from the starter `ESP32C6_App.ino`")
    return md


PREAMBLE = """# Hardware personalizer: build an app for your ESP32-C6 device

> **How to use this prompt**: start a new chat in Claude, ChatGPT or any capable AI, attach this file (or paste all of it), and say "Let's start." The AI will ask about your idea, help you shape it, write the code, and walk you through putting it on your device.

---

**Instructions for the AI.** Everything below is your operating guide for this conversation. Follow the four phases in order. The reference sections and starter files further down replace the skill folder mentioned in them: where a phase says to read a reference, use that section of this prompt. Unless you can run shell commands on the person's computer, use the Arduino IDE flashing path. The starter files are verified on the device; give them to the person exactly as written.

"""


def build_prompt():
    skill = body((SKILL / "SKILL.md").read_text())
    skill = skill.replace("Everything this skill refers to lives in this skill's folder: `references/` for knowledge, `starter/` for code that is already proven on the device.",
                          "Everything referred to below is included later in this prompt: reference sections for knowledge, and starter files with code that is already proven on the device.")
    skill = skill.replace("Read `starter/ESP32C6_App/app.cpp` for the shape of an app. Reuse proven patterns: the demo sections of `starter/Tour/app.cpp` (shake reveal, status colours, flip timer, noise meter, tilt game, arc gauge, reactive face, touch poll, word-wrapped text, rings), and `starter/Demo/app.cpp` for smooth canvas animation, Wi-Fi scanning and sound recording.",
                          "Read the starter `app.cpp` (end of this prompt) for the shape of an app, and reuse the proven patterns in the demo sections of the **Tour app** (end of this prompt).")
    skill = skill.replace("A swipeable tour of mini demos lives in `starter/Tour/app.cpp`.", "A swipeable tour of mini demos is the **Tour app** at the end of this prompt.")
    parts = [PREAMBLE, "## Your role and the four phases\n\n" + demote(retarget(skill))]
    for name, title in REFERENCES:
        text = body((SKILL / "references" / name).read_text())
        text = text.replace(
            "Upload the self-test: the `starter/SelfTest` sketch (same three-file layout). It checks every part of the device on screen and over serial.",
            "Write a small self-test `app.cpp` that prints `ok` or `MISSING` over serial for each part (`c6i2c::present()` on addresses 0x18, 0x34, 0x40, 0x51, 0x5A, 0x6B) and shows touch, button and motion events on screen.")
        parts.append(f"## {title}\n\n" + demote(retarget(text)))
    starter = ["## Starter files\n\nThe sketch folder is named `ESP32C6_App` and holds exactly these three files."]
    for rel in STARTER_FILES:
        lang = "cpp"
        starter.append(f"### `{Path(rel).name}`\n\n```{lang}\n{(STARTER / rel).read_text().rstrip()}\n```")
    parts.append("\n\n".join(starter))
    parts.append("## Tour app\n\nThe on-device tour: an `app.cpp` that replaces the starter's `app.cpp` (same folder, same other two files). Its demo sections are also proven patterns for Phase 3.\n\n"
                 f"```cpp\n{(STARTER / TOUR_FILE).read_text().rstrip()}\n```")
    out = DIST / "PROMPT.md"
    text = "\n\n".join(parts) + "\n"
    leftover = re.findall(r"references/|starter/", text)
    if leftover:
        raise SystemExit(f"PROMPT.md still points into the skill folder: {sorted(set(leftover))}")
    out.write_text(text)
    return out


if __name__ == "__main__":
    DIST.mkdir(exist_ok=True)
    sync_board_file()
    for built in (build_zip(), build_prompt()):
        print(f"wrote {built.relative_to(ROOT)} ({built.stat().st_size // 1024} KB)")
