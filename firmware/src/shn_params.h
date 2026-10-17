/* SPDX-License-Identifier: GPL-3.0-only */
/* The parameter table: a patch is a byte per parameter plus a 10-character name. The UI, MIDI CC, the
 * randomizer and the storage all walk this table. (Shine On: a string ensemble, see shn_engine.h.) */
enum {
    P_CBASS, P_CELLO, P_VIOLA, P_VIOLIN,        /* the registers: 16' and 8' (the bass section), 8' and 4' (the upper) */
    P_TRUMPET, P_HORN,                          /* the brass registers: the 8' square through two formants */
    P_SPLIT,                                    /* OFF: every register over the whole keyboard; else the bass section below */
    P_TONE, P_WARMTH, P_BRIGHT,                 /* the output filter, a gentle saturation, a presence lift */
    P_ATTACK, P_RELEASE, P_VEL,                 /* the keyer: crescendo, sustain length, velocity to level */
    P_LFO_RATE, P_VIB, P_LFO_DELAY, P_MOD_DEPTH,   /* the vibrato (none on the originals: a little goes a long way) */
    P_GLIDE, P_VINTAGE, P_OCTAVE, P_BEND, P_VOLUME,
    P_ARP_MODE, P_ARP_DIV, P_ARP_OCT, P_ARP_GATE,
    P_ENS_MODE, P_ENS_DEPTH, P_ENS_RATE, P_ENS_MIX,   /* the effects: sound A's (shn_fx.h) */
    P_PHA_RATE, P_PHA_DEPTH, P_PHA_FB, P_PHA_MIX,
    P_DLY_TIME, P_DLY_FB, P_DLY_TONE, P_DLY_MIX,
    P_REV_SIZE, P_REV_DAMP, P_REV_PRE, P_REV_MIX,
    NPARAM
};
#define PATCH_NAME 10u
#define PATCH_BYTES 80u                         /* NPARAM values, the name at NPARAM, the rest spare */
#if NPARAM + PATCH_NAME > PATCH_BYTES
#error "the patch outgrew PATCH_BYTES"
#endif
#define NPROG 99u                               /* 1-32 factory sounds, 33-99 INIT; the user's saves over them */
#define NFACT 32u

enum { K_NUM, K_BIP, K_ONOFF, K_ARP, K_DIV, K_MS, K_HZ, K_PCT, K_CUT, K_OCT, K_DDIV, K_SPLIT, K_ENS, K_REG, K_OCTS };
enum { A_OFF, A_UP, A_DOWN, A_UPDN, A_RND, A_PLAY, NARPMODE };
enum { D_4, D_8, D_8T, D_16, D_16T, D_32, D_32T, NDIV };
enum { DD_16, DD_8T, DD_8, DD_8D, DD_4, DD_4D, DD_2, NDDIV };   /* the delay's note values */
enum { E_OFF, E_MONO, E_STEREO, E_WIDE, NENS };   /* the ensemble: as the originals (one output), or its three lines across the stereo field */

typedef struct {
    const char *name;                           /* 8 characters at most (the knob label) */
    uint8_t max, def, kind, cc;                 /* cc: a fixed MIDI CC, 0 = taken from 20 up in table order */
} pdef_t;
static const pdef_t PDEF[NPARAM] = {
    [P_CBASS] = {"C.BASS", 127, 0, K_REG, 0},
    [P_CELLO] = {"CELLO", 127, 0, K_REG, 0},
    [P_VIOLA] = {"VIOLA", 127, 100, K_REG, 0},
    [P_VIOLIN] = {"VIOLIN", 127, 100, K_REG, 0},
    [P_TRUMPET] = {"TRUMPET", 127, 0, K_REG, 0},
    [P_HORN] = {"HORN", 127, 0, K_REG, 0},
    [P_SPLIT] = {"SPLIT", 48, 0, K_SPLIT, 0},  /* 0 OFF, else the bass section ends below MIDI 36 + v (C2 .. C6) */
    [P_TONE] = {"TONE", 127, 100, K_CUT, 74},
    [P_WARMTH] = {"WARMTH", 127, 30, K_NUM, 0},
    [P_BRIGHT] = {"BRIGHT", 127, 40, K_NUM, 0},
    [P_ATTACK] = {"ATTACK", 127, 60, K_MS, 73},
    [P_RELEASE] = {"RELEASE", 127, 75, K_MS, 72},
    [P_VEL] = {"VELOCITY", 127, 20, K_NUM, 0},
    [P_LFO_RATE] = {"RATE", 127, 78, K_HZ, 0},
    [P_VIB] = {"VIBRATO", 127, 0, K_NUM, 0},
    [P_LFO_DELAY] = {"DELAY", 127, 40, K_NUM, 0},
    [P_MOD_DEPTH] = {"MOD DPT", 127, 0, K_NUM, 1},
    [P_GLIDE] = {"GLIDE", 127, 0, K_MS, 5},
    [P_VINTAGE] = {"VINTAGE", 127, 25, K_NUM, 0},
    [P_OCTAVE] = {"OCTAVE", 2, 1, K_OCTS, 0},   /* -1, 0, +1: the whole sound an octave down or up */
    [P_BEND] = {"BEND", 24, 2, K_NUM, 0},
    [P_VOLUME] = {"VOLUME", 127, 100, K_NUM, 7},
    [P_ARP_MODE] = {"ARP", 5, A_OFF, K_ARP, 0},
    [P_ARP_DIV] = {"DIV", 6, D_8, K_DIV, 0},
    [P_ARP_OCT] = {"OCTAVES", 4, 1, K_OCT, 0},
    [P_ARP_GATE] = {"GATE", 127, 100, K_PCT, 0},
    [P_ENS_MODE] = {"ENSEMBLE", NENS - 1, E_STEREO, K_ENS, 0},
    [P_ENS_DEPTH] = {"DEPTH", 127, 80, K_NUM, 0},
    [P_ENS_RATE] = {"RATE", 127, 64, K_NUM, 0},
    [P_ENS_MIX] = {"MIX", 127, 127, K_PCT, 0},
    [P_PHA_RATE] = {"RATE", 127, 30, K_NUM, 0},
    [P_PHA_DEPTH] = {"DEPTH", 127, 90, K_NUM, 0},
    [P_PHA_FB] = {"FEEDBK", 127, 40, K_NUM, 0},
    [P_PHA_MIX] = {"MIX", 127, 0, K_PCT, 0},
    [P_DLY_TIME] = {"TIME", NDDIV - 1, DD_8D, K_DDIV, 0},
    [P_DLY_FB] = {"FEEDBK", 127, 50, K_NUM, 0},
    [P_DLY_TONE] = {"TONE", 127, 70, K_NUM, 0},
    [P_DLY_MIX] = {"MIX", 127, 0, K_NUM, 0},
    [P_REV_SIZE] = {"SIZE", 127, 80, K_NUM, 0},
    [P_REV_DAMP] = {"DAMP", 127, 60, K_NUM, 0},
    [P_REV_PRE] = {"PRE-DLY", 127, 20, K_NUM, 0},
    [P_REV_MIX] = {"MIX", 127, 0, K_NUM, 0},
};

static const char *const ARP_NAME[NARPMODE] = {"OFF", "UP", "DOWN", "UP/DN", "RANDOM", "PLAYED"};
static const char *const DIV_NAME[NDIV] = {"1/4", "1/8", "1/8T", "1/16", "1/16T", "1/32", "1/32T"};
static const uint8_t DIV_PER_QUARTER[NDIV] = {1, 2, 3, 4, 6, 8, 12};
static const char *const DDIV_NAME[NDDIV] = {"1/16", "1/8T", "1/8", "1/8.", "1/4", "1/4.", "1/2"};
static const uint8_t DDIV_NUM[NDDIV] = {1, 1, 1, 3, 1, 3, 2}, DDIV_DEN[NDDIV] = {4, 3, 2, 4, 1, 2, 1};   /* of a quarter */
static const char *const ENS_NAME[NENS] = {"OFF", "MONO", "STEREO", "WIDE"};

/* the pages: four knobs each (-1 = no knob) */
typedef struct { const char *title; int8_t p[4]; } page_t;
enum { PG_PERFORM, PG_REG1, PG_REG2, PG_TONE, PG_ENV, PG_VIB, PG_ARP, PG_ENS, PG_PHASER, PG_DELAY, PG_REVERB,
       PG_OUTPUT, PG_GLOBAL, PG_SEQ, NPAGE };   /* the sequencer's step view (not patch parameters: drawn by hand) */
static const page_t PAGE[NPAGE] = {
    [PG_PERFORM] = {"PERFORM", {P_TONE, P_ATTACK, P_RELEASE, P_ENS_DEPTH}},
    [PG_REG1] = {"STRINGS", {P_CBASS, P_CELLO, P_VIOLA, P_VIOLIN}},
    [PG_REG2] = {"BRASS + SPLIT", {P_TRUMPET, P_HORN, P_SPLIT, P_TONE}},
    [PG_TONE] = {"TONE", {P_TONE, P_WARMTH, P_BRIGHT, P_VEL}},
    [PG_ENV] = {"KEYER", {P_ATTACK, P_RELEASE, P_VEL, P_GLIDE}},
    [PG_VIB] = {"VIBRATO", {P_LFO_RATE, P_VIB, P_LFO_DELAY, P_MOD_DEPTH}},
    [PG_OUTPUT] = {"OUTPUT", {P_VINTAGE, P_OCTAVE, P_BEND, P_VOLUME}},
    [PG_ARP] = {"ARP", {P_ARP_MODE, P_ARP_DIV, P_ARP_OCT, P_ARP_GATE}},
    [PG_ENS] = {"ENSEMBLE", {P_ENS_MODE, P_ENS_DEPTH, P_ENS_RATE, P_ENS_MIX}},
    [PG_PHASER] = {"PHASER", {P_PHA_RATE, P_PHA_DEPTH, P_PHA_FB, P_PHA_MIX}},
    [PG_DELAY] = {"DELAY", {P_DLY_TIME, P_DLY_FB, P_DLY_TONE, P_DLY_MIX}},
    [PG_REVERB] = {"REVERB", {P_REV_SIZE, P_REV_DAMP, P_REV_PRE, P_REV_MIX}},
    [PG_GLOBAL] = {"GLOBAL", {-1, -1, -1, -1}},   /* tempo, click, MIDI channel, tune: globals, drawn by hand */
    [PG_SEQ] = {"SEQUENCER", {-1, -1, -1, -1}},   /* length, note value, swing, transpose; a held step: its own four */
};

typedef struct { uint8_t v[PATCH_BYTES]; } patch_t;   /* v[0..NPARAM) the parameters, v[NPARAM..] the name */

static void patch_defaults(patch_t *p)
{
    uint32_t i;
    for (i = 0; i < NPARAM; i++) p->v[i] = PDEF[i].def;
    for (; i < PATCH_BYTES; i++) p->v[i] = ' ';
}

/* a factory patch: a name and a list of (parameter, value) pairs over the defaults, ended by 255 */
typedef struct { const char *name; const uint8_t *set; } factory_t;
static void patch_from_factory(patch_t *p, const factory_t *f)
{
    const uint8_t *s = f->set;
    uint32_t i;
    patch_defaults(p);
    while (s[0] != 255u) { p->v[s[0]] = s[1]; s += 2; }
    for (i = 0; i < PATCH_NAME && f->name[i]; i++) p->v[NPARAM + i] = (uint8_t)f->name[i];
}
