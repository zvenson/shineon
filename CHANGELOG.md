# Changelog

Shine On for the M-VAVE FM-1 (and, as a beta, the M-Vave SMK-37). Newest first. Each release is on
[GitHub](https://github.com/zvenson/shineon/releases) and on [the Firmware Hub](https://thefirmwarehub.com/firmware/shineon/).

## 3.5 – 2026-11-12

- **Saving made simple: MEMORY** (as OberVave 3.5). SAVE opens one page with the 8 memory slots. A slot holds
  everything: the steps of all four tracks, their four sounds (registers, ensemble, phaser), the mixer, the effects
  and MASTER. KNOB 1 (or PRESETS) picks the slot, KNOB 2 (or SELECT) the action, SAVE does it: **SAVE**, **LOAD**,
  **COPY**, **PASTE**, **INIT** and **SOUND** (the chosen track's sound into one of the 99 programs). SAVE, SAVE saves
  what you hear into its slot.
- **No auto-save any more.** What you play is a working copy of the slot loaded last; a `*` shows changes not saved
  yet. LOAD the same slot again and you are back to how it was saved. Anything that would throw changes away, or
  overwrite another slot, asks once more; changing patterns with unsaved changes warns first. (Global settings such
  as tempo and MIDI channel still keep themselves.)
- Gone: SAVE held + keys 1-8 (go / copy), SAVE PATTERN?, the PATTERNS view, the program "save as". SAVE held + SELECT
  still dices a new sound.
- **Its own store.** Up to 3.0 Shine On kept its programs and patterns where OberVave keeps them, so after switching
  between the two each read the other's patterns as its own, and a save overwrote them. On the FM-1 Shine On now has
  its own place in the flash; on the first start of 3.5 its programs come over, and its patterns too when Shine On was
  the last to run (OberVave's stay where they are, untouched). On the SMK-37, where there is no room for a second
  store, Shine On marks the store as its own: data of the other firmware is cleared instead of misread (3.0's own data
  stays).
- **VIBE LINK:** the FM-1 as a controller for the VIBE FM from inside Shine On (OUTPUT page, SELECT pressed: OFF / ON /
  CONTROL).
- **Backup** of programs, patterns and settings over USB (the same protocol as OberVave's backup page).
- **FM-1 Mirror:** the screen, live and big, on a computer or phone with
  [FM-1 Mirror](https://github.com/pfkellogg/fm1-mirror) by Patrick F. Kellogg.
- Newer FM-1s with the UC25HQ80 flash chip save and update (they saved to RAM only before).
- The sound: soft knees instead of the last hard clamps after the registers and in the effect sends (as OberVave 3.3);
  at normal levels nothing changes, a full sixteen-key chord on every register stays round.
- Identity `FM-1_9635`.

## 3.0.1 – 2026-10-23

- Its own identity `FM-1_9630` (3.0 shared one with another firmware, so the hub could not tell them apart).

## 3.0 – 2026-10-23

- OberVave 3.0's **four tracks**, mixer, MASTER and sequencer with Shine On's engine: the 16 keyers shared by four
  sounds, each with its own registers, ensemble and phaser; DELAY, REVERB and DRIVE as the mixer's sends.
- Tracks start and stop on the bar; every pattern keeps its sounds and its mix; recording with a quantise per track.
- 1.x's settings and patterns come over on the first start.

## 1.2.3 – 2026-10-19

- The update loader shows what it does, in the Firmware Hub's look.

## 1.2.2 – 2026-10-19

- The FM-1's **TRS MIDI IN** jack works; the keys send on the set MIDI channel.

## 1.2.1 – 2026-10-18

- 1.2, built and signed by GitHub Actions.

## 1.2 – 2026-10-18

- 1.0's sound fixes in a package (there is no 1.1: that identity belongs to another firmware).

## 1.0 – 2026-10-17

- The first release: a string ensemble in the spirit of the divide-down string machines of the seventies – keyers,
  phase-locked footages, six register filters, TONE / WARMTH / BRIGHT, the three-line ensemble and a phaser, delay and
  plate reverb, the sequencer of OberVave 1.5.
