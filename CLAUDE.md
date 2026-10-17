# Shine On

A firmware for the M-VAVE FM-1: a string ensemble in the spirit of the divide-down string machines of the
seventies (Solina / Eminent era). Started 2026-10-17 as a copy of OberVave 1.5 (`../obervave`, zvenson/obervave,
commit 6194c3b): the panel, the sequencer (two tracks, two sounds), the delay and the reverb are OberVave's; the
engine and the ensemble are new. GPL-3.0. Identity `FM-1_96N`. Name by the user ("Shine on you crazy synth").

## Hard rules

- As OberVave's: Dinghy's platform (`platform.c`, `usb.c`, `ota.c`, hal/) stays as it is. Keep the Felucca / SLOOP /
  Dinghy / OberVave credits (LICENSING.md, README).
- No trademarks in the product: never "ARP", "Solina", "Eminent", "Pink Floyd" or a song title on screen, in patch
  names or in the release name. In docs only as "inspired by". No artist or song names in patch names.
- Target: JieLi AC791N, no FPU, no libgcc: integers only, **no 64-bit division**, no constant in
  0xFFC00000-0xFFD00000 (`tools/build.py` checks). RAM <= 96 KB, big buffers in `.pool`.
- Audio ISR: 256 frames per call at 44.1 kHz (5.8 ms). The user's priority is **sound quality over CPU**; measure
  on the device (GLOBAL page title) before trading any of it away.
- The identity prefix is `FM-1_96`: release X.Y gives FM-1_96Y. **Never release a 1.1 or a 1.6**: FM-1_961 is FOMAJAM's, FM-1_966 GR!MM's (both on the hub).

## Where things are

- `firmware/src/shineon.c`: the app (OberVave's, with the new parameter set). Includes `platform.c` first, then
  `shn_dsp.h` (oscillators, SVF, tanh: OberVave's), `shn_params.h` (the parameter table, pages), `shn_fx.h`
  (ensemble, phaser, delay, reverb), `shn_engine.h` (keyers, footages, registers, arp: the ISR), `shn_seq.h`
  (the sequencer, OberVave's unchanged), `shn_patches.h` (32 factory programs), `shn_ui.h` (the drawing, OberVave's
  in a warm palette, 16 LEDs).
- Engine (`shn_engine.h`): NV 16 keyers, POLY only (the same key again restarts its keyer). A keyer renders its
  footages (16' / 8' / 4' saw, 8' square, phase-locked at the note on) into the sound's five busses
  `fbus[part][5][256]` (.pool): 16', 8' for the bass section, 8', 4', square for the upper. `v->w` says which
  (SPLIT: a low key feeds the bass busses, a high one the upper; no SPLIT: all). `regs_render` then runs the six
  fixed register filters (`coef_fixed`, corners from gen_tables' G_* defines) on the busses, x 0.086, TONE (an SVF
  over the lot, slewed), WARMTH (tanh), BRIGHT (one-pole presence), mono into `out` (A) or `bus_b` (B). The keyer
  is OberVave's env_tick without decay/sustain. Vibrato: a triangle per sound, delay / fade per keyer.
- Effects (`shn_fx.h`): the ensemble = one 1024-sample line, three Hermite taps at ENS_BASE (6 ms) +- the slow
  sine (0.6 Hz x RATE, up to +-3.9 ms) + the fast one (6 Hz, a ninth of the depth), phases 0 / 120 / 240 degrees
  (SIN_Q15 index + 85 / + 171); MONO sums the three, STEREO 1 L / 3 R / 2 both, WIDE 1+2 L, 3+2-1/2 R. MIX up to
  1.0 = lines only (the originals had no dry path). Phaser: 6 first-order allpasses a side, coefficient from
  PHA_A_Q15 (120 Hz .. 6 kHz) by a triangle, the right a quarter turn on, feedback. Delay and reverb are
  OberVave's. The FX layer's knobs: ENS MIX, PHASER MIX, DELAY MIX, REVERB MIX.
- Levels: a five-note chord on VIOLA + VIOLIN peaks ~-5 dBFS, RMS ~-19 dBFS (tests/strings.txt); the keyers go into
  the busses at a quarter, the register sum is x 0.28, so 16 keyers stay within the filters' 32-bit products and
  the output tanh is driven to ~0.7 at most. Host cost ~1720 instr/sample on that script (OberVave's worst ~1900).
- Lessons (17.10., the user heard crackle on the VIBE): the ensemble's LFO sine MUST be interpolated (sin_q15; the
  plain 256-step table jumped the lines by samples); the registers use a linear SVF (svf_out), not shn_dsp.h's
  saturating one; a keyer still sounding keeps its phases on a retrigger / steal; DEPTH glides (ens_cur_s/f).
  Measure cleanliness as HF energy > 10 kHz per 10 ms window over the WHOLE file (filter first, then slice).
- Settings: `SET_VERSION 0x53` and `PS_TAG "SHNP"`, so an OberVave store on the same FM-1 is not read as ours.
  The 8 pattern sectors are shared with OberVave's layout (same `pattern_t`): a pattern made in OberVave loads.
- `tools/gen_tables.py` -> `firmware/gen/shn_tables.h` (committed): OberVave's tables plus SIN_Q15, PHA_A_Q15,
  G_* register corners. `tools/make_logo.py` -> `assets/logo*`, `--splash` -> `firmware/gen/shn_splash.h`.
- `tests/run_host.sh [script]`, `tests/strings.txt` (four programs, chords), `OBV_PERF=1` for instr/sample.
  `sh web/sim/build.sh` -> `docs/sim/` (zig + wasm-opt in ~/.local/share/obervave-wasm, shared with OberVave).
- `tools/build.py`: `./build.sh [--release X.Y]` -> `build/shineon[-X.Y].fwsc`. Releases go to `docs/firmware/`
  (the hub reads the newest package there).
- On VIBE FM Mk1: `~/VibeFMmk1/vibe-sdk/firmware/shineon/` (upstream copy + `shineon_vibe.c`, OberVave's adapter).

## Status

1.0 (2026-10-17): host-tested, device build passes (image 123 KB, RAM 41 KB, pool 284 KB).
1.2 (2026-10-18, FM-1_962): 1.0's sound fixes released (LFO interpolation, clean register filters, phases kept on a
retrigger, two-pole BBD filter); on the hub. Host, OTA and loader tests pass; **not yet tried on an FM-1**. The SMK-37 build compiles with OberVave's layer but is untested.
