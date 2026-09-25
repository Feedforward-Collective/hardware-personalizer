# Ideas for the ESP32-C6 device

Use this to spark and sanity-check ideas. Tailor, combine and remix; the best idea is usually a small twist on the person's own routine.

## What the device can sense and do

| Inputs | Outputs |
|---|---|
| Touch screen: tap, drag, swipe left/right/up/down | 480 × 480 colour AMOLED screen, 2.16 inches, deep true blacks, bright |
| Motion: shake, tilt (pitch and roll), face-up/face-down, spinning | Speaker: beeps, chimes, melodies, short sound clips |
| Buttons: KEY and BOOT (free to use), PWR (short press) | Wi-Fi: call web APIs, show live data, send messages |
| Two microphones: loudness, claps, short recordings | Bluetooth LE (advanced) |
| Clock that keeps time on battery | Battery power: portable for several hours |
| Wi-Fi: data from anywhere on the internet | |

**Mixing recipe**: pick one input, one output and one moment in their day. "When I **flip it face-down**, it **starts a 25-minute focus timer** and **chimes** when done."

## Limits (check every wish against these)

| Wish | Reality on this device | Workaround |
|---|---|---|
| Understand speech, chat by voice, translate | No on-device speech recognition or AI model: too little memory | Record a few seconds, send to a cloud API (speech-to-text, an LLM, translation), show or speak the answer. Needs Wi-Fi and an API key. Advanced. |
| Play music or long audio | Sound clips must fit in memory or flash storage (about 9 MB) | Short clips (a few seconds) are easy; longer audio is a stretch goal |
| Photos, video, camera | No camera. Full-screen images must be stored in flash and drawn in pieces | Simple drawings, icons, text and small images work well |
| Read the user's calendar, Slack, Teams, email | Possible through each service's API, but sign-in (OAuth) on the device is hard | A published calendar (iCal) link, a webhook, or a small helper script on their computer that pushes data to the device |
| Know what the user's computer is doing (on a call, busy) | The device cannot see the computer | A small script on the computer sends status over Wi-Fi |
| Run for days on battery | Screen-on battery life is several hours (not yet measured) | Keep it on USB power, dim the screen, or turn the screen off when idle |
| Rotate the picture | The screen always draws upright | Design the layout for how the device will sit |
| Multi-finger gestures, precise drawing | One touch point, finger-sized accuracy | Big touch targets (60 px or more) |
| SD card | The slot exists but the starter kit does not support it yet | Use the 9 MB of built-in flash storage |
| Talk to other devices directly | Bluetooth LE and ESP-NOW are possible but advanced | Use Wi-Fi and a web service first |

## Things people have built with devices like this

Most of these were built on similar ESP32 boards; each note says how it maps onto this one.

- **Token meter for AI coding** ([Clawdmeter](https://www.cnx-software.com/2026/05/14/clawdmeter-a-diy-esp32-s3-desk-dashboard-for-claude-code-token-usage-monitoring/)): a desk gauge of Claude Code usage. Here: a helper script on the computer pushes usage numbers over Wi-Fi; the device draws a live gauge.
- **Busy light** ([Smart Busy Light](https://www.instructables.com/Smart-Busy-Light-for-Modern-Workspaces/)): a glowing "busy / free" sign outside an office. Here: full-screen colour plus status text; tap or press KEY to toggle, or drive it from a calendar link.
- **On-air sign** ([XDA on-air screen](https://www.xda-developers.com/esp32-on-air-screen-home-office-track-calls/)): lights up whenever you are on a call. Here: a helper script on the computer detects the camera or microphone in use and tells the device.
- **Desk pet** ([PixelCat](https://github.com/toddsherman/pixelcat)): a pixel-art cat that purrs when petted, hisses when shaken, and startles at loud noises. Here: a perfect fit for touch, motion, microphone and speaker.
- **Desk companion** (Deskmate): clock, weather and cute animations on a desk. Here: time from the internet, weather from a free API, idle animations.
- **Tilt games** ([Tilt-a-tron](https://github.com/PartiallyFrozen/tilt-a-tron), [Marble Roller](https://github.com/OzInFl/Waveshare-ESP32-S3-2inch-Capacitive-Touch-Display-Marble-Roller-Game)): steer a marble through a maze by tilting. Here: tilt drives the game, with smooth animation.
- **Flip timer** ([Pomodoro Cube](https://github.com/shantanugoel/pomodoro_cube)): flip the device to start focus or break timers. Here: face-down starts focus, face-up shows time left, chime at the end.
- **Voice assistant** ([Voice-Assistant-5](https://github.com/iollama/Voice-Assistant-5), [local voice assistant](https://www.matt-adams.co.uk/ai/esp32/2026/07/09/local-voice-assistant-esp32-s3-audio-board.html)): push-to-talk AI conversation with an animated face. Here: hold KEY to record, send to a cloud speech/LLM service, show the reply. Advanced; memory is tight.
- **Pocket translator** (r/esp32): speak, see the translation. Here: same record-and-send pattern as the voice assistant.
- **Meeting-room sign** ([DRCV](https://github.com/epfl-si/drcv-esp32)): shows a room's upcoming bookings from the calendar server. Here: fetch a calendar feed; show "free until 3pm" or "in use".
- **Offline maps** (r/esp32): map tiles stored on the device. Here: small maps or floor plans stored in flash storage.

## Idea bank

Difficulty: ★ an afternoon · ★★ a weekend · ★★★ a project. "Wi-Fi" means it needs a network (and maybe an API key).

### At the desk and in the office
- **Focus flip** ★: face-down starts a focus block; the screen glows a calm colour; chime when done; shake to cancel.
- **Status badge** ★: swipe between "Heads down", "Ask me anything", "On a call", "Out to lunch", in bold colours for a desk or door.
- **Standup timer** ★: each speaker taps to pass the turn; a ring counts down 60 seconds; beeps at the end.
- **Meeting cost ticker** ★: enter the number of people; watch the cost of the meeting climb in real time.
- **Decision spinner** ★: shake to pick who takes notes, where to have lunch, or which task to do next.
- **Posture and break nudge** ★: notices it hasn't moved for 50 minutes and nudges you to stand.
- **Noise meter** ★★: shows open-office loudness as a face that turns from calm to frazzled.
- **Live KPI gauge** ★★ Wi-Fi: one number that matters (sales today, sign-ups, open tickets, uptime) as a big dial, with a chime on each milestone.
- **Next-meeting countdown** ★★ Wi-Fi: reads a published calendar link and shows "Board review in 12 min".
- **AI usage meter** ★★ Wi-Fi: tokens or spend for the team's AI tools, fed by a small script.
- **Slack/Teams status mirror** ★★★ Wi-Fi: mirrors your presence from a webhook or helper script.

### Customers, events and retail
- **Trade-show magnet** ★: a shake-to-reveal fortune, tip or prize; the booth's "shake me" moment (the Magic 8 idea).
- **Quick poll** ★: visitors tap one of three big answers; tallies show live; a chime on each vote.
- **Queue caller** ★★: staff press KEY to call the next number with a chime; big friendly display.
- **Table-service button** ★★ Wi-Fi: a guest taps "Ready to order" and staff get a message.
- **Feedback smiley** ★★ Wi-Fi: four faces to tap at a checkout; results post to a spreadsheet.
- **Product demo loop** ★: an attract-mode animation that wakes on pickup (motion) and walks through features.
- **Scavenger clue** ★: tilt to unlock a riddle, shake to get a hint; for workshops and offsites.

### Teams, training and workshops
- **Icebreaker deck** ★: shake for a question; tap for the next person.
- **Quiz buzzer** ★★: first to shake or tap wins, with a sound and a colour per team.
- **Energy check-in** ★: tap your mood at the start of a session; the device shows the room's average.
- **Timer for facilitators** ★: time-boxed agenda items, gentle warning chime, loud final bell.
- **Signal for help** ★: tilt the device to flag "stuck" in a hands-on training.

### Home and personal
- **Desk pet** ★★: a creature that reacts to petting, shaking, tilting and noise.
- **Kitchen timer and converter** ★: spin to set minutes, swipe for cup/gram conversions.
- **Kid's chore chart** ★: tap each chore done for a cheer and a star.
- **Plant or medicine reminder** ★: a daily chime and a "done" tap; nags gently if missed.
- **Weather and clock** ★★ Wi-Fi: a beautiful full-screen clock with today's forecast.
- **Tilt maze or bubble level** ★★: game or tool from the motion sensor.
- **Baby sleep noise monitor** ★★: shows and logs noise levels through the night.
- **Family message board** ★★★ Wi-Fi: short messages sent from a phone appear with a chime.

## Coaching tips

- The strongest first projects connect to one real moment in the person's week.
- Physical interactions (shake, flip, tilt) feel magical; lead with them when you can.
- A black background with one bold element looks great on this screen.
- Offer a stretch path ("version 2 could pull this from your CRM") without building it first.
