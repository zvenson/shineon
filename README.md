<p align="center"><img src="assets/logo.png" width="800" alt="Shine On: the name in glowing gold outlines, three bands of light through the lower half of the letters"></p>

# Shine On

**A string ensemble for the M-VAVE FM-1, on four tracks: each sound with its own ensemble and phaser, a mixer with delay, plate reverb and drive, a master, a step sequencer.**

Shine On turns the FM-1 into one of the string machines of the seventies: every key has its own keyer, the tone
comes from phase-locked footages (16', 8', 4' sawtooth and an 8' square, as a top-octave divider gives them), and
the six registers are fixed formant filters on the sum, not filters per voice: CONTRABASS and CELLO (the bass
section), VIOLA, VIOLIN, TRUMPET and HORN. Then TONE, WARMTH, BRIGHT and the part that makes the sound: three
bucket-brigade lines swayed by two LFOs 120 degrees apart, and a phaser. Since 3.0 it plays **four tracks** that
share the 16 keyers, each with its own sound (its own registers, ensemble and phaser), arpeggiator and sequence,
through a **mixer** with delay, plate reverb and drive as send effects and a **MASTER** (DJ filter, drive,
compressor); the tracks start and stop on the bar, every pattern keeps its sounds and its mix. 32 factory programs
(99 slots). The panel, the sequencer, the delay and the reverb are [OberVave](https://github.com/zvenson/obervave)'s.

<p align="center"><img src="docs/screen.png" width="240" alt="Shine On's screen: program 01 ENSEMBLE, the PERFORM page with four knobs, the scope, sixteen keyer LEDs"></p>

> **Released: 3.0** (`docs/firmware/shineon-3.0.fwsc`, identity `FM-1_960`; on the Firmware Hub, built and signed by
> this repository's workflow). 3.0 is OberVave 3.0's four
> tracks, mixer and sequencer with Shine On's engine: 1.x's settings and patterns come over on the first start (the old
> patterns stay in the flash). Four full string sounds at once cost about twice the CPU of one: watch the time on the
> GLOBAL page.
>
> Before 3.0: 1.2.3 showed the update loader's progress in the Firmware Hub's look (the idea from Skiff); 1.2.2
> opened the FM-1's **TRS MIDI IN** jack (through a TRS-to-DIN adapter, type A or B); the keys send on the MIDI
> channel set.

**Play it in your browser:** `docs/sim/` is the firmware itself (the same C sources, compiled to WebAssembly) with
the FM-1's screen, keys, buttons and knobs, your computer keyboard or a MIDI keyboard. Build it with
`sh web/sim/build.sh`, try it with `python3 -m http.server -d docs/sim`.

## The sound

| | |
| --- | --- |
| **Keyers** | 16, shared by the four tracks' sounds. Each key its own attack (CRESCENDO on the originals) and release (SUSTAIN LENGTH), analog RC curves 0.5 ms to 10 s, a little velocity if you want it (the originals had none), GLIDE. |
| **Footages** | Per keyer a 16', 8' and 4' sawtooth and an 8' square, phase-locked as a divider's (PolyBLEP anti-aliased). OCTAVE shifts the whole sound. |
| **Registers** | Fixed formant filters on the summed footages: CONTRABASS (16', round), CELLO (8', warm with a 400 Hz body), VIOLA (8', nasal, a 1.2 kHz formant), VIOLIN (4', bright and airy), TRUMPET (8' square, a 1.5 kHz band), HORN (8' square, hollow). SPLIT puts the bass section under a key of your choice, as the originals' lower manual; off, every register plays the whole keyboard. |
| **Tone** | TONE (a 12 dB low-pass over the lot, CC 74), WARMTH (a gentle tanh), BRIGHT (a presence lift above 3 kHz). VINTAGE: a few cents a keyer and a slow drift. |
| **Vibrato** | A triangle per sound with rate, depth and a delay; the mod wheel (CC 1) adds to it. |
| **Ensemble** | Each sound its own: three lines on one bucket brigade, swayed by a slow LFO (0.6 Hz, up to +-4 ms) and a fast one (6 Hz, a tenth of that), three phases 120 degrees apart, 4-point Hermite reads. MONO as the originals (one output), STEREO (line 1 left, 3 right, 2 in the middle), WIDE. RATE scales both LFOs, MIX down to the dry divider. A program brings its ensemble. |
| **Phaser** | Each sound its own, after its ensemble: six allpass stages a side, the right a quarter turn behind the left, 0.05 to 4 Hz, feedback: the pedal every string machine was played through. A program brings it (PHASED STR, PHASE BRASS, PHASE HORN). |
| **Send effects, master** | The song's (a program change never cuts a reverb tail), fed by each track's sends in the mixer: OberVave's ping-pong delay in note values of the tempo with a tape-like slew, Dattorro's plate at the full sample rate, a drive (OVERDRIVE, DIST, FUZZ, CRUSH). Then the MASTER: volume, a DJ filter (one knob: low-pass below the middle, high-pass above), drive, a compressor and a soft limiter. |
| **Arpeggiator** | UP, DOWN, UP/DN, RANDOM, AS PLAYED; 1-4 octaves; 1/4 to 1/32 with triplets; gate; latch (HOLD). One per sound. Follows MIDI clock. |
| **Sequencer** | OberVave 3.0's: four tracks of 256 steps, chords of four, velocity, gate and ties, ratchets, chance, a nudge, polymeters, swing, up to four recorded knobs a track; the tracks start and stop on the bar; REC quantized per track or as played; 8 patterns in flash, each with its four sounds and its mix, switched on the bar. |
| **MIDI** | USB-MIDI in and out; the FM-1's TRS MIDI IN jack (3.5 mm, through a TRS-to-DIN adapter, type A or B). Notes, velocity, pitch bend, program change, sustain, mod wheel, clock / start / stop (or ignored: SYNC INTERNAL). OMNI plays the chosen track; with a MIDI channel set, that channel is track A and the next three are B-D. CC 74 tone, 73 / 72 attack / release, 5 glide, 1 mod depth, 7 volume, 64 sustain, 120 / 123 all off; every other parameter from CC 20 up in the order of the parameter table (`firmware/src/shn_params.h`). |

Integers only: the chip has no FPU. The whole engine is fixed point, the tables are generated by
`tools/gen_tables.py` and committed.

## The pages

**HOME** PERFORM (TONE, ATTACK, RELEASE, ensemble DEPTH); again: the **MIXER**. **EDIT** STRINGS (the four string
registers) / BRASS + SPLIT / TONE (TONE, WARMTH, BRIGHT, VELOCITY). **ENV** KEYER (ATTACK, RELEASE, VELOCITY, GLIDE).
**LFO** VIBRATO. **ARP** the arpeggiator (on its page the ARP button is HOLD, each track its own). **FX** ENSEMBLE /
PHASER (the track's sound's) / DELAY / REVERB / DRIVE (the song's). **GLO** GLOBAL (TEMPO, CLICK, MIDI CH, TUNE; the
CPU time and the sync in its title) / OUTPUT (VINTAGE, OCTAVE, BEND, VOLUME). **PRESETS** the chosen track's program,
**SAVE** twice saves it (PRESETS in between: into another slot), SAVE held + SELECT pressed: a new sound at random.

## The panel

The panel works as sloopDX's: **tap** a function button (press and let go, touching nothing else) and its pages
open; **hold** it and it is a **layer**: the white keys and the knobs change job until you let go. **HOME** tapped
while a layer is held locks it open; its button lets it go.

| Hold | Keys | KNOB 1 · 2 · 3 · 4 | Tap |
| --- | --- | --- | --- |
| **GLO** | white 1-4 (F3 .. B3): start / stop track A-D on the bar (REC lit: record it); 5-8: mute A-D; 9-12: solo A-D; G5: tap tempo | - | GLOBAL / OUTPUT |
| **SEQ** | steps 1-16 of the page (tap: set with the last note or chord / clear; hold + a later step: one long note); black keys F#3 .. A#4: page 1-8 | NOTE · DIV · SWING · LENGTH; a step held: NOTE · LEVEL · RATCHET · GATE | the sequencer (full screen); again: its next view |
| **EDIT** | erase that note (playing: as the playhead passes) | SHIFT · LENGTH x2 / 1/2 · TRANSPOSE | STRINGS / BRASS + SPLIT / TONE; in STEPS: clear the step |
| **SAVE** | white 1-8: go to pattern 1-8; black 1-8 (F#3 .. A#4): copy this pattern there (with SELECT pressed: a random sound) | - | twice: save the track's program (PRESETS in between: into another slot); in the sequencer: SAVE PATTERN?, again: saved |
| **FX** | notes | ENSEMBLE · PHASER (the track's) · DELAY · REVERB mix | ENSEMBLE / PHASER / DELAY / REVERB / DRIVE |

- **ALGORITHM** picks the track (A-D, in its colour: amber, light blue, green, pink): the keys play its sound, every
  page edits it, **PRESETS** picks its program. **SELECT** turned: in STEPS a bar on, in the sequencer's other views
  and the MIXER the next pattern (on the bar), elsewhere the tempo; **SELECT held** in STEPS shows STEP+, pressed on
  GLOBAL it switches SYNC (AUTO: a MIDI clock leads; INTERNAL: ignored). **OCT- / OCT+** the octave (both: 0).
- **The MIXER** (HOME twice; HOME again: its next row): LEVEL, PAN, SEND DELAY, SEND REVERB, SEND DRIVE, MASTER. KNOB
  1-4 are tracks A-D in the row (MASTER: VOLUME, DJ FILTER, DRIVE, COMP); under them each track's level, a meter,
  PLAY / STOP and its mute and solo.
- **PLAY** starts the tracks that are on from step 1 / stops (the light flashes on the beat). **REC** playing:
  record the chosen track now / stop; stopped: armed, the first note starts (CLICK on COUNT-IN: PLAY counts in a bar
  first). **REC held 1.2 s** clears the track. Turning a knob while REC records writes its value onto the steps.
  **EDIT + OCT- / OCT+** undo / redo.
- The key LEDs show the notes that sound, the page buttons light for the page you are on, the sixteen round LEDs on
  the screen glow with each keyer.

## The sequencer

As [OberVave 3.0's](https://github.com/zvenson/obervave#the-sequencer). **SEQ tapped** gives it the whole screen and
steps through its views (**TRACKS**, **STEPS**, **TRACK**, **PATTERNS**), **HOME** leaves. TRACKS shows A-D one under
the other, each with its program, its state (PLAY, STOP, NEXT / ENDS while it waits for the bar, MUTE), its 16 steps
and, for a track longer than a bar, where it plays (`2/4` and a line a piece per bar).

| View | KNOB 1 | KNOB 2 | KNOB 3 | KNOB 4 |
| --- | --- | --- | --- | --- |
| **TRACKS** (PRESETS: the track's sound) | TONE | LEVEL (the mixer's) | LENGTH | QUANT (REC's grid: 1/32 .. 1/4, OFF) |
| **STEPS** (SELECT held: STEP+) | the step cursor | NOTE (the chord moves) | GATE (to TIE) | VELOCITY |
| **STEP+** | the step cursor | RATCHET 1-4 | CHANCE | the step ON / OFF |
| **TRACK** | LENGTH (fast: by 16; up to 256) | DIV | SWING | TRANSPOSE |
| **PATTERNS** | the slot | - | GO there | COPY this one there (twice on a used one) |

- **Step entry** (STEPS): a key or a chord goes into the cursor's step, the cursor moves on when you let go; a key
  held and KNOB 1 turned on makes a long note. **EDIT tapped** clears the step. MIDI notes write too.
- **Recording:** REC on every page, onto the chosen track, on its grid or as played; legato up to the next note of the
  take; with the arp on, the arp's notes, each round replacing the last.
- **Patterns:** SELECT in the sequencer or the MIXER goes to the next / previous one (stopped at once, playing on the
  next bar, its sounds and mix with it); SAVE held + a white key 1-8 goes there, + a black key copies this one there.
- **Saving:** SAVE in the sequencer asks `SAVE PATTERN?`, SAVE again saves (playing: at the stop); a pattern saves
  itself 3 s after a change once the sequencer stands.

## Build and run

Shine On is built on [Dinghy](https://github.com/zvenson/dinghy), whose platform (boot, USB, updates, audio, settings)
it keeps unchanged; the setup is Dinghy's (Linux x86-64, Python 3 with Pillow, the JieLi toolchain, three files of
the JieLi AC79 SDK: see Dinghy's README, "Set up once").

```sh
tests/run_host.sh                 # the synth on your computer: build/host/shineon.wav and screens as PNG
tests/run_host.sh tests/strings.txt   # a script of keys / buttons / knob turns / MIDI over time (tests/host.c explains)
sh web/sim/build.sh               # the browser simulator: docs/sim (zig cc + wasm-opt), node tests/sim_test.mjs
./build.sh                        # the device package: build/shineon.fwsc
./build.sh --release 1.2          # -> build/shineon-1.2.fwsc, identity FM-1_962
python3 tools/fm1_install.py build/shineon.fwsc   # onto the FM-1 over USB
```

The build refuses what would brick an FM-1 (a 64-bit division, a call into the chip ROM, too much RAM); the rules
are in `docs/RULES.md`, the platform calls in `docs/API.md`.

## Credits and licence

GPL-3.0-only, see `LICENSE` and `LICENSING.md` for who made what. Shine On is OberVave's panel, sequencer and
effects chain with a new engine; OberVave stands on [Dinghy](https://github.com/zvenson/dinghy) (zvenson), which
carries the FM-1 platform of [Felucca](https://github.com/hugelton/Felucca) by Leo Kuroshita (Hügelton Instruments)
with the changes of [SLOOP](https://github.com/isod89/sloop-fm1) (isod89). The filters follow Andrew Simper's
trapezoidal SVF, the reverb Jon Dattorro's 1997 plate.

Shine On is inspired by the string machines of the seventies (the ARP / Eminent Solina String Ensemble above all).
Those names are trademarks of their owners; Shine On is not made by or affiliated with them, and none of the marks
appear in the firmware. "M-VAVE" and "FM-1" belong to their owners too. The name is a wink at a certain
nine-part song that was played through one; no song or artist is named in the firmware.
