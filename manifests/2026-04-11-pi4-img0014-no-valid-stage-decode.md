# Pi 4 `IMG_0014.mov` Analysis

Date:

- `2026-04-11`

Input clip:

- `/Users/witoldbolt/Downloads/IMG_0014.mov`

Tooling:

- [analyze-rpi4-actled-video.py](/Users/witoldbolt/phoenix-rpi/scripts/analyze-rpi4-actled-video.py)
- [interpret-rpi4-actled-analysis.py](/Users/witoldbolt/phoenix-rpi/scripts/interpret-rpi4-actled-analysis.py)
- current layout:
  - `pi4_dense_firstread_focus_map_2026_04_10`

Raw video facts:

- width:
  - `510`
- height:
  - `238`
- effective frame rate:
  - about `59.92 fps`
- duration:
  - about `94.83 s`

Decoder result:

- no valid Phoenix stage burst decoded
- interpreter result:
  - `highest_completed=none`
  - `next_expected=1`
- auto-detected ACT LED ROI:
  - `244,130,278,163`

Important raw LED observations:

- the analyzer does see many green-LED on-segments
- but they do not form a valid sync-plus-5-bit Phoenix stage burst
- the early cluster around `0.92s .. 2.05s` is malformed and does not decode
- later isolated single-pulse or malformed groups continue across the clip

Current interpretation:

- this clip does **not** provide a reliable Phoenix stage boundary
- current possibilities are:
  - only firmware / retry chatter is visible
  - the board resets or re-enters before a full Phoenix burst completes
  - the LED lane is now too ambiguous for efficient progress on this image

Decision:

- do not widen the LED-only debug loop based on this clip
- treat UART as the next authoritative hardware-debug lane
- keep LED video in parallel as a secondary signal

Implication for next retry:

- when the USB-TTL cable is available, run:
  - host UART capture
  - LED video in parallel
- prioritize UART-derived boundary classification over LED-only inference
