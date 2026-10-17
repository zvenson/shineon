/* SPDX-License-Identifier: GPL-3.0-only */
/* The factory programs: each a name and the parameters that differ from the defaults (shn_params.h). The defaults
 * are the classic upper section (VIOLA + VIOLIN, the ensemble in stereo): most programs only move a few knobs. */
#define X 255
/* 01: the sound everyone means: 8' + 4' strings, slow swell, the ensemble in stereo, a little plate */
static const uint8_t PF_ENSEMBLE[] = {P_ATTACK, 70, P_RELEASE, 85, P_REV_MIX, 30, P_REV_SIZE, 90, X};
/* 02: the full orchestra: every string register, split so the bass has the low keys */
static const uint8_t PF_FULL_STR[] = {P_CBASS, 90, P_CELLO, 100, P_VIOLA, 100, P_VIOLIN, 90, P_SPLIT, 24, P_ATTACK, 72, P_RELEASE, 90,
    P_REV_MIX, 35, P_REV_SIZE, 100, X};
/* 03: the dark one: a long slow pad through a wide ensemble and a big hall; the phaser turns slowly underneath */
static const uint8_t PF_SHINE_PAD[] = {P_CELLO, 80, P_VIOLA, 110, P_VIOLIN, 60, P_TONE, 78, P_ATTACK, 92, P_RELEASE, 105,
    P_ENS_MODE, E_WIDE, P_ENS_DEPTH, 95, P_PHA_MIX, 70, P_PHA_RATE, 18, P_PHA_DEPTH, 100, P_PHA_FB, 55,
    P_REV_MIX, 55, P_REV_SIZE, 118, P_REV_DAMP, 70, P_REV_PRE, 40, P_DLY_TIME, DD_4D, P_DLY_FB, 35, P_DLY_TONE, 35, P_DLY_MIX, 22, X};
/* 04: violins only, bright and fast */
static const uint8_t PF_VIOLINS[] = {P_VIOLA, 0, P_VIOLIN, 120, P_ATTACK, 40, P_RELEASE, 60, P_BRIGHT, 70, P_REV_MIX, 25, X};
/* 05: violas, round */
static const uint8_t PF_VIOLAS[] = {P_VIOLA, 120, P_VIOLIN, 0, P_TONE, 90, P_ATTACK, 65, P_RELEASE, 80, P_REV_MIX, 28, X};
/* 06: cellos, warm and low */
static const uint8_t PF_CELLOS[] = {P_CELLO, 120, P_VIOLA, 0, P_VIOLIN, 0, P_ATTACK, 60, P_RELEASE, 85, P_WARMTH, 50, P_REV_MIX, 30, X};
/* 07: the bass section alone */
static const uint8_t PF_CONTRABASS[] = {P_CBASS, 110, P_CELLO, 70, P_VIOLA, 0, P_VIOLIN, 0, P_ATTACK, 30, P_RELEASE, 50,
    P_ENS_MODE, E_MONO, P_ENS_DEPTH, 50, P_WARMTH, 60, X};
/* 08: cello and violin, an octave apart, hollow in the middle */
static const uint8_t PF_OCTAVES[] = {P_CELLO, 100, P_VIOLA, 0, P_VIOLIN, 100, P_ATTACK, 68, P_RELEASE, 90, P_REV_MIX, 30, X};
/* 09: the brass: trumpet and horn through the ensemble (the originals' second mood) */
static const uint8_t PF_BRASS[] = {P_VIOLA, 0, P_VIOLIN, 0, P_TRUMPET, 110, P_HORN, 60, P_ATTACK, 25, P_RELEASE, 45, P_ENS_DEPTH, 60,
    P_WARMTH, 55, X};
/* 10: horns alone, soft */
static const uint8_t PF_HORNS[] = {P_VIOLA, 0, P_VIOLIN, 0, P_HORN, 120, P_ATTACK, 45, P_RELEASE, 60, P_TONE, 85, P_ENS_DEPTH, 55,
    P_REV_MIX, 35, P_REV_SIZE, 100, X};
/* 11: strings and brass together: the big tutti */
static const uint8_t PF_TUTTI[] = {P_CBASS, 70, P_CELLO, 80, P_VIOLA, 90, P_VIOLIN, 80, P_TRUMPET, 60, P_HORN, 50, P_SPLIT, 24,
    P_ATTACK, 55, P_RELEASE, 75, P_REV_MIX, 32, P_REV_SIZE, 105, X};
/* 12: a phased string pad: the classic pedal in front of the machine */
static const uint8_t PF_PHASED[] = {P_ATTACK, 65, P_RELEASE, 85, P_PHA_MIX, 100, P_PHA_RATE, 35, P_PHA_DEPTH, 110, P_PHA_FB, 60,
    P_REV_MIX, 25, X};
/* 13: the mono ensemble, as the originals came out of the back */
static const uint8_t PF_VINTAGE_MONO[] = {P_ENS_MODE, E_MONO, P_ENS_DEPTH, 85, P_VINTAGE, 70, P_TONE, 88, P_ATTACK, 62, P_RELEASE, 80, X};
/* 14: dry, no ensemble: the raw divider, a honest organ-like string */
static const uint8_t PF_DRY_STRINGS[] = {P_ENS_MODE, E_OFF, P_ATTACK, 50, P_RELEASE, 70, P_VINTAGE, 10, X};
/* 15: fast attack for staccato chords */
static const uint8_t PF_STACCATO[] = {P_ATTACK, 8, P_RELEASE, 40, P_VEL, 60, P_VIOLIN, 110, P_VIOLA, 80, X};
/* 16: a swell that takes its time */
static const uint8_t PF_SLOW_SWELL[] = {P_ATTACK, 105, P_RELEASE, 110, P_CELLO, 60, P_VIOLA, 100, P_VIOLIN, 80, P_ENS_DEPTH, 90,
    P_REV_MIX, 45, P_REV_SIZE, 115, P_REV_PRE, 50, X};
/* 17: a string machine with a vibrato (the originals had none; a touch is lovely) */
static const uint8_t PF_VIBRATO_STR[] = {P_VIB, 28, P_LFO_RATE, 82, P_LFO_DELAY, 60, P_ATTACK, 60, P_RELEASE, 80, P_REV_MIX, 28, X};
/* 18: an arpeggio in eighths through the ensemble and a dotted delay */
static const uint8_t PF_ARP_STRINGS[] = {P_ARP_MODE, A_UP, P_ARP_DIV, D_8, P_ARP_OCT, 2, P_ARP_GATE, 90, P_ATTACK, 20, P_RELEASE, 70,
    P_DLY_TIME, DD_8D, P_DLY_FB, 45, P_DLY_TONE, 50, P_DLY_MIX, 40, P_REV_MIX, 30, X};
/* 19: a sixteenth arpeggio on violins, short */
static const uint8_t PF_ARP_PIZZ[] = {P_ARP_MODE, A_UPDN, P_ARP_DIV, D_16, P_ARP_OCT, 2, P_ARP_GATE, 40, P_VIOLA, 0, P_VIOLIN, 120,
    P_ATTACK, 3, P_RELEASE, 35, P_DLY_TIME, DD_8, P_DLY_FB, 40, P_DLY_MIX, 35, P_REV_MIX, 25, X};
/* 20: a bass line machine: contrabass, mono ensemble, a little glide */
static const uint8_t PF_BASS_LINE[] = {P_CBASS, 120, P_CELLO, 40, P_VIOLA, 0, P_VIOLIN, 0, P_ATTACK, 10, P_RELEASE, 35, P_GLIDE, 20,
    P_ENS_MODE, E_MONO, P_ENS_DEPTH, 40, P_WARMTH, 70, P_TONE, 80, X};
/* 21: the whole thing an octave down: dark cellos */
static const uint8_t PF_LOW_STRINGS[] = {P_OCTAVE, 0, P_CELLO, 70, P_VIOLA, 110, P_VIOLIN, 50, P_TONE, 84, P_ATTACK, 75, P_RELEASE, 95,
    P_REV_MIX, 38, P_REV_SIZE, 110, X};
/* 22: an octave up: a shimmer of high strings */
static const uint8_t PF_HIGH_STRINGS[] = {P_OCTAVE, 2, P_VIOLA, 60, P_VIOLIN, 110, P_TONE, 100, P_ATTACK, 70, P_RELEASE, 90,
    P_ENS_DEPTH, 90, P_REV_MIX, 40, P_REV_SIZE, 115, X};
/* 23: a brass stab with a phaser */
static const uint8_t PF_PHASE_BRASS[] = {P_VIOLA, 0, P_VIOLIN, 0, P_TRUMPET, 120, P_HORN, 40, P_ATTACK, 12, P_RELEASE, 40,
    P_PHA_MIX, 90, P_PHA_RATE, 45, P_PHA_DEPTH, 100, P_PHA_FB, 70, P_ENS_DEPTH, 50, X};
/* 24: a wide, deep ensemble: the biggest sway */
static const uint8_t PF_DEEP_WIDE[] = {P_ENS_MODE, E_WIDE, P_ENS_DEPTH, 127, P_ENS_RATE, 40, P_ATTACK, 75, P_RELEASE, 95,
    P_CELLO, 60, P_REV_MIX, 35, X};
/* 25: a quick shimmer: fast ensemble, violins */
static const uint8_t PF_SHIMMER[] = {P_ENS_RATE, 110, P_ENS_DEPTH, 60, P_VIOLA, 40, P_VIOLIN, 120, P_BRIGHT, 80, P_ATTACK, 55,
    P_RELEASE, 80, P_REV_MIX, 35, X};
/* 26: an organ-like keyboard: all four footages, no ensemble sway, fast keyer */
static const uint8_t PF_STRING_ORG[] = {P_CBASS, 60, P_CELLO, 80, P_VIOLA, 80, P_VIOLIN, 80, P_ATTACK, 5, P_RELEASE, 20,
    P_ENS_MODE, E_OFF, P_WARMTH, 40, P_REV_MIX, 20, X};
/* 27: a dark hall of cellos, long release, delay in quarters */
static const uint8_t PF_CATHEDRAL[] = {P_CELLO, 110, P_VIOLA, 50, P_VIOLIN, 0, P_TONE, 76, P_ATTACK, 85, P_RELEASE, 115,
    P_REV_MIX, 65, P_REV_SIZE, 125, P_REV_DAMP, 80, P_REV_PRE, 60, P_DLY_TIME, DD_4, P_DLY_FB, 45, P_DLY_TONE, 30, P_DLY_MIX, 30, X};
/* 28: a warm overdriven ensemble, as through a tube amp */
static const uint8_t PF_WARM_TUBE[] = {P_WARMTH, 110, P_TONE, 82, P_CELLO, 60, P_VIOLA, 100, P_VIOLIN, 70, P_ATTACK, 60,
    P_RELEASE, 85, P_REV_MIX, 25, X};
/* 29: a split: contrabass under the keys, bright violins above */
static const uint8_t PF_SPLIT_DUO[] = {P_CBASS, 120, P_CELLO, 50, P_VIOLA, 50, P_VIOLIN, 120, P_SPLIT, 24, P_ATTACK, 30,
    P_RELEASE, 60, P_REV_MIX, 28, X};
/* 30: a slow phaser on a horn section */
static const uint8_t PF_PHASE_HORNS[] = {P_VIOLA, 0, P_VIOLIN, 0, P_TRUMPET, 50, P_HORN, 110, P_ATTACK, 50, P_RELEASE, 70,
    P_PHA_MIX, 100, P_PHA_RATE, 15, P_PHA_DEPTH, 127, P_PHA_FB, 50, P_TONE, 90, P_REV_MIX, 30, X};
/* 31: a random arpeggio in triplets, a shimmering texture */
static const uint8_t PF_ARP_TEXTURE[] = {P_ARP_MODE, A_RND, P_ARP_DIV, D_16T, P_ARP_OCT, 3, P_ARP_GATE, 70, P_ATTACK, 30, P_RELEASE, 90,
    P_VIOLA, 50, P_VIOLIN, 110, P_DLY_TIME, DD_8T, P_DLY_FB, 55, P_DLY_TONE, 45, P_DLY_MIX, 45, P_REV_MIX, 40, P_REV_SIZE, 110, X};
/* 32: the vibrato cello solo line, glide */
static const uint8_t PF_SOLO_CELLO[] = {P_CELLO, 120, P_VIOLA, 30, P_VIOLIN, 0, P_VIB, 40, P_LFO_RATE, 80, P_LFO_DELAY, 45,
    P_GLIDE, 25, P_ATTACK, 40, P_RELEASE, 60, P_ENS_DEPTH, 60, P_REV_MIX, 35, X};

static const factory_t FACTORY[NFACT] = {
    {"ENSEMBLE", PF_ENSEMBLE}, {"FULL STR", PF_FULL_STR}, {"SHINE PAD", PF_SHINE_PAD}, {"VIOLINS", PF_VIOLINS},
    {"VIOLAS", PF_VIOLAS}, {"CELLOS", PF_CELLOS}, {"CONTRABASS", PF_CONTRABASS}, {"OCTAVES", PF_OCTAVES},
    {"BRASS", PF_BRASS}, {"HORNS", PF_HORNS}, {"TUTTI", PF_TUTTI}, {"PHASED STR", PF_PHASED},
    {"VINTAGE MO", PF_VINTAGE_MONO}, {"DRY STRING", PF_DRY_STRINGS}, {"STACCATO", PF_STACCATO}, {"SLOW SWELL", PF_SLOW_SWELL},
    {"VIBRATO ST", PF_VIBRATO_STR}, {"ARP STRING", PF_ARP_STRINGS}, {"ARP PIZZ", PF_ARP_PIZZ}, {"BASS LINE", PF_BASS_LINE},
    {"LOW STRING", PF_LOW_STRINGS}, {"HIGH STRIN", PF_HIGH_STRINGS}, {"PHASE BRAS", PF_PHASE_BRASS}, {"DEEP WIDE", PF_DEEP_WIDE},
    {"SHIMMER", PF_SHIMMER}, {"STRING ORG", PF_STRING_ORG}, {"CATHEDRAL", PF_CATHEDRAL}, {"WARM TUBE", PF_WARM_TUBE},
    {"SPLIT DUO", PF_SPLIT_DUO}, {"PHASE HORN", PF_PHASE_HORNS}, {"ARP TEXTUR", PF_ARP_TEXTURE}, {"SOLO CELLO", PF_SOLO_CELLO},
};
static const uint8_t PF_INIT[] = {X};
static const factory_t FACT_INIT = {"INIT", PF_INIT};
#undef X
