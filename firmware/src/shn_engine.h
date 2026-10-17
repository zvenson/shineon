/* SPDX-License-Identifier: GPL-3.0-only */
/* The engine: a string ensemble in the audio ISR, in the spirit of the Italian / Dutch divide-down string machines
 * of 1974-80. Every key has its own keyer (an attack / release envelope), the tone comes from phase-locked
 * footages (16', 8', 4' sawtooth and an 8' square, as a top-octave divider gives them), and the six registers are
 * fixed formant filters on the summed footages, not filters per voice: CONTRABASS and CELLO (the bass section),
 * VIOLA, VIOLIN, TRUMPET and HORN (the upper). Then the tone control, a gentle saturation and the ensemble
 * (shn_fx.h). 16 voices. The main loop sends note events through a ring and writes the live patch and the
 * controllers; the ISR owns the voices, the held-note list, the arpeggiator and the sustain. */
#ifndef RING_PUBLISH
#define RING_PUBLISH() __asm__ volatile("" ::: "memory")
#endif
#define NV 16u
#define NHELD 16u
#define ENV_ONE (1 << 24)
#define LEVEL_FLOOR 2048                        /* -78 dB: a release below this frees the voice */
#define PW50 0x80000000u

enum { E_IDLE, E_ATT, E_DEC, E_SUS, E_REL };
typedef struct { uint32_t stage; int32_t level; } env_t;   /* level Q24 */

typedef struct {
    osc_t o16, o8, o4, osq;                     /* the footages, phase-locked as a divider's */
    uint32_t noise;
    env_t ae;
    uint32_t inc8;                              /* this block's 8' step (16' = half, 4' = double) */
    int32_t amp0, amp1;                         /* Q15 at the block's start and end */
    int32_t pitch16, target16;                  /* Q16 semitones: the glide */
    int32_t drift;                              /* the vintage random walk */
    uint32_t lfo_blk;                           /* blocks since the note on (vibrato delay / fade) */
    int32_t lfo_fade;                           /* Q15 */
    uint32_t seq;                               /* allocation order */
    uint8_t note, vel, gate;                    /* gate: the key (or the arp) holds it */
    uint8_t part;                               /* 0: sound A, 1: track B's own sound */
    uint8_t low;                                /* in the bass section (SPLIT) */
    uint8_t w;                                  /* this block: which footages to render (W_*) */
} voice_t;
#define W_16 1u
#define W_8LO 2u
#define W_8 4u
#define W_4 8u
#define W_SQ 16u

/* two sounds: A (the keys, the arp, the effects) and track B's own. Track B has one when its pattern names a
 * program (pat.trk[1].pad[0] = program + 1); else B plays A as part 0. The main loop writes, the ISR reads. */
static patch_t live, live_b, live_bq;           /* live_bq: the next pattern's B, swapped in with it */
static volatile uint32_t ctl_bq_swap;           /* the next pattern brings live_bq along */
static uint32_t b_own;                          /* (ISR) track B has its own sound this block */
static voice_t vc[NV];
static uint32_t vseq, rr;                       /* allocation counter, round-robin pointer */

/* main loop -> ISR: note events */
typedef struct { uint8_t type, note, vel, part; } nev_t;   /* type 1 on, 0 off; part 1: track B's sound */
#define NQ 64u
static nev_t nq[NQ];
static volatile uint32_t nq_w, nq_r;
static void note_event_p(uint32_t part, uint32_t on, uint32_t note, uint32_t vel)
{
    if (nq_w - nq_r >= NQ) return;              /* a flood: drop (the ISR drains 64 per block) */
    nq[nq_w % NQ].part = (uint8_t)part;
    nq[nq_w % NQ].type = (uint8_t)on;
    nq[nq_w % NQ].note = (uint8_t)note;
    nq[nq_w % NQ].vel = (uint8_t)vel;
    RING_PUBLISH();
    nq_w++;
}
static void note_event(uint32_t on, uint32_t note, uint32_t vel) { note_event_p(0, on, note, vel); }

/* the controllers (main loop writes) */
static volatile int32_t ctl_bend;               /* -8192..8191 */
static volatile uint32_t ctl_sustain, ctl_hold, ctl_cc7 = 127;   /* ctl_hold: bit p = sound p's HOLD */
#if BOARD_SMK37
static volatile int32_t ctl_vib;                /* the SMK-37's mod wheel 0..127: a vibrato, up to +-1/2 semitone */
#endif
static volatile int32_t ctl_tune8;              /* global tune, Q8 semitones */
static volatile uint32_t ctl_tempo = 120, ctl_ext_quarter;   /* MIDI clock: frames per quarter, 0 = none */
static volatile uint32_t ctl_arp_restart, ctl_panic;

/* ISR -> main loop */
static volatile int32_t ui_vlevel[NV];          /* each voice's keyer, Q15 */
static volatile uint8_t ui_vnote[NV];
static volatile uint32_t ui_render_us, ui_arp_step;
#define SCOPE_N 240u
static int16_t scope[SCOPE_N];
static volatile uint32_t scope_state, scope_stride = 4;   /* 0 idle, 1 armed, 2 running, 3 done */

/* ---- the held notes (ISR): in order of pressing, physically down or held by the pedal / latch */
static struct { uint8_t note, vel, phys; } held[2][NHELD];   /* per sound: A's keys, B's keys */
static uint32_t nheld[2];
static uint32_t rnd_state = 0x9E3779B9u;

/* ---- the register filters: fixed formants on the summed footages, one set per sound */
enum { R_CBASS, R_CELLO, R_VIOLA, R_VIOLIN, R_TRUMPET, R_HORN, NREG };
typedef struct {
    svf_state_t cb, ce, ce_f, va, va_f, va_h, vi_h, vi, tr, tr_l, ho;
    svf_state_t tone;
    int32_t br_lp;                              /* the presence lift's one-pole */
    int32_t gain[NREG];                         /* Q12, slewed per sample towards the patch's */
} regs_t;
static svf_coef_t C_CB, C_CE, C_CEF, C_VA, C_VAF, C_VAH, C_VIH, C_VI, C_TR, C_TRL, C_HO;   /* (set once) */

/* ---- the two sounds as the ISR sees them: the patch, its vibrato, the numbers derived from the patch once per block */
typedef struct {
    const patch_t *p;
    uint32_t lfo_ph;
    int32_t lfo_val;                            /* this block, Q15 triangle */
    int32_t last_pitch16;                       /* where a glide starts from */
    int32_t vib_q, wheel_q, bend8, vint, velamp, gain_q12, oct8, split;
    int32_t reg_q12[NREG], warmth_q12, bright_q, tone_sm;
    uint32_t glide_k, w_lo, w_up;               /* which footages the bass / upper section needs this block */
    svf_coef_t tone_c;
    regs_t r;
} part_t;
static part_t pt[2] = {{&live, 0, 0, 60 << 16}, {&live, 0, 0, 60 << 16}};
static uint32_t seq_b_own(void);                /* (shn_seq.h) the pattern gives track B its own sound */
static void parts_point(void)                   /* which patch each part plays (the pattern may have changed) */
{
    uint32_t was = b_own;
    b_own = seq_b_own();
    pt[1].p = b_own ? &live_b : &live;
    if (b_own && !was) pt[1].tone_sm = live_b.v[P_TONE] << 8;   /* B's own from now: its tone from where it is */
}

static void held_remove(uint32_t p, uint32_t note)
{
    uint32_t i, j;
    for (i = j = 0; i < nheld[p]; i++)
        if (held[p][i].note != note) held[p][j++] = held[p][i];
    nheld[p] = j;
}

/* ---- vintage: each keyer has its own small offsets, scaled by the knob (a divider is in tune by nature: a little) */
static const int8_t VINT_TUNE8[NV] = {-6, 4, 2, -5, 6, -3, 1, -5, 3, -4, 5, -2, 2, -6, 4, -1};   /* +-2.5 cents */
static const int8_t VINT_ENV[NV] = {10, -6, 14, -12, 3, -16, 8, -4, 12, -9, 5, -14, 7, -2, 15, -11};   /* /128 on the times */

static void voice_start(voice_t *v, uint32_t part, uint32_t note, uint32_t vel)
{
    const uint8_t *pv = pt[part].p->v;
    v->part = (uint8_t)part;
    v->note = (uint8_t)note;
    v->vel = (uint8_t)vel;
    v->gate = 1;
    v->seq = ++vseq;
    v->low = (uint8_t)(pv[P_SPLIT] && note < 36u + pv[P_SPLIT]);
    v->target16 = (int32_t)note << 16;
    v->pitch16 = pv[P_GLIDE] ? pt[part].last_pitch16 : v->target16;
    v->ae.stage = E_ATT;
    v->lfo_blk = 0;
    v->lfo_fade = pv[P_LFO_DELAY] ? 0 : 32767;
    /* the footages start locked, as a divider's: 16' at half the 8' phase, 4' at twice, the square with the 8' */
    v->o8.ph = xorshift(&v->noise);
    v->o16.ph = v->o8.ph >> 1;
    v->o4.ph = v->o8.ph << 1;
    v->osq.ph = v->o8.ph;
    v->o16.prev = v->o8.prev = v->o4.prev = v->osq.prev = 0;
}

/* round robin over the 16 keyers; steal the oldest released, then the oldest */
static uint32_t voice_alloc(void)
{
    uint32_t i, v, pick = 0, best = 0xFFFFFFFFu;
    for (i = 0; i < NV; i++) {
        v = (rr + i) % NV;
        if (vc[v].ae.stage == E_IDLE) { rr = v + 1u; return v; }
    }
    for (v = 0; v < NV; v++)
        if (!vc[v].gate && vc[v].seq < best) { best = vc[v].seq; pick = v; }
    if (best == 0xFFFFFFFFu)
        for (v = 0; v < NV; v++)
            if (vc[v].seq < best) { best = vc[v].seq; pick = v; }
    rr = pick + 1u;
    return pick;
}

static void voice_trigger(uint32_t part, uint32_t note, uint32_t vel)
{
    uint32_t v;
    for (v = 0; v < NV; v++)                    /* the same key again: its keyer restarts (a divider has one per key) */
        if (vc[v].part == part && vc[v].note == note && vc[v].ae.stage != E_IDLE) break;
    if (v == NV) v = voice_alloc();
    voice_start(&vc[v], part, note, vel);
    pt[part].last_pitch16 = (int32_t)note << 16;
}

/* the key (or the arp, the sequencer) let go of a note */
static void voice_release(uint32_t part, uint32_t note, uint32_t from_arp)
{
    uint32_t v;
    (void)from_arp;
    for (v = 0; v < NV; v++)
        if (vc[v].gate && vc[v].note == note && vc[v].part == part) vc[v].gate = 0;
}

static void voices_release_all(void)
{
    uint32_t v;
    for (v = 0; v < NV; v++) vc[v].gate = 0;
}
static void voices_release_part(uint32_t part)
{
    uint32_t v;
    for (v = 0; v < NV; v++)
        if (vc[v].part == part) vc[v].gate = 0;
}

static void seq_tick(void);                     /* the step sequencer (shn_seq.h): per block, after the arp */

/* the click (main: ctl_click on / off) and a one-bar count-in before a recording (main: ctl_count) */
static volatile uint32_t ctl_click, ctl_count, ui_seq_count;
static int32_t click_amp;
static uint32_t click_ph, click_inc;
static void click_start(uint32_t accent)        /* beat 1 higher and louder */
{
    click_amp = accent ? 9000 : 6000;
    click_ph = 0;
    click_inc = accent ? 233739600u : 155826400u;   /* 2400 / 1600 Hz in 2^32 / 44100 steps */
}

/* ---- the arpeggiators (per block, 0.36 ms steps): one per sound, each on its own keys and its own ARP settings */
typedef struct { uint32_t cnt, period, gate_at, pos, dir, sounding, note, was_on; } arp_t;
static arp_t arp[2];

/* ISR -> main loop: the notes the arps play (REC records them, not the keys that feed them): on, note, part */
#define AQ 32u
static uint8_t arp_q[AQ][3];
static volatile uint32_t arp_qw, arp_qr;
static void arp_post(uint32_t p, uint32_t on, uint32_t note)
{
    if (arp_qw - arp_qr >= AQ) return;
    arp_q[arp_qw % AQ][0] = (uint8_t)on;
    arp_q[arp_qw % AQ][1] = (uint8_t)note;
    arp_q[arp_qw % AQ][2] = (uint8_t)p;
    RING_PUBLISH();
    arp_qw++;
}

static void arp_stop_note(uint32_t p)
{
    arp_t *a = &arp[p];
    if (a->sounding) { voice_release(p, a->note, 1); a->sounding = 0; arp_post(p, 0, a->note); }
}

static void arp_step(uint32_t p)
{
    const uint8_t *pv = pt[p].p->v;
    arp_t *a = &arp[p];
    uint8_t sorted[NHELD];
    uint32_t i, j, n = nheld[p], oct = clampi(pv[P_ARP_OCT], 1, 4), len, k, mode = pv[P_ARP_MODE], idx;
    for (i = 0; i < n; i++) sorted[i] = held[p][i].note;
    if (mode != A_PLAY)
        for (i = 1; i < n; i++) {               /* insertion sort: 16 at most */
            uint8_t t = sorted[i];
            for (j = i; j && sorted[j - 1u] > t; j--) sorted[j] = sorted[j - 1u];
            sorted[j] = t;
        }
    len = n * oct;
    if (mode == A_RND)
        k = xorshift(&rnd_state) % len;
    else if (mode == A_UPDN && len > 2u) {
        if (a->pos >= 2u * len - 2u) a->pos = 0;
        k = a->pos < len ? a->pos : 2u * len - 2u - a->pos;
        a->pos++;
    } else {
        if (a->pos >= len) a->pos = 0;
        k = mode == A_DOWN ? len - 1u - a->pos : a->pos;
        a->pos++;
    }
    idx = k % n;
    a->note = (uint32_t)clampi((int32_t)sorted[idx] + 12 * (int32_t)(k / n), 0, 127);
    for (i = 0; i < n; i++)                     /* the velocity of that key */
        if (held[p][i].note == sorted[idx]) break;
    voice_trigger(p, a->note, i < n ? held[p][i].vel : 100u);
    arp_post(p, 1, a->note);
    a->sounding = 1;
    ui_arp_step++;
}

static void arp_tick(uint32_t p)
{
    const uint8_t *pv = pt[p].p->v;
    arp_t *a = &arp[p];
    uint32_t mode = pv[P_ARP_MODE], div = DIV_PER_QUARTER[pv[P_ARP_DIV] % NDIV], q;
    if (mode == A_OFF) {
        if (a->was_on) { arp_stop_note(p); a->was_on = 0; }
        return;
    }
    if (!a->was_on) { voices_release_part(p); a->was_on = 1; a->pos = 0; a->cnt = 0xFFFFFFFFu; }
    if (!nheld[p]) { arp_stop_note(p); a->pos = 0; a->cnt = 0xFFFFFFFFu; return; }
    q = ctl_ext_quarter ? ctl_ext_quarter : 60u * FS / clampi(ctl_tempo, 40, 240);
    a->period = q / div;
    if (a->period < CTL) a->period = CTL;
    a->gate_at = (a->period * (pv[P_ARP_GATE] + 1u)) >> 7;
    if (a->cnt == 0xFFFFFFFFu || a->cnt + CTL >= a->period) {
        a->cnt = a->cnt == 0xFFFFFFFFu ? 0 : a->cnt + CTL - a->period;
        arp_stop_note(p);
        arp_step(p);
    } else {
        a->cnt += CTL;
        if (a->sounding && a->cnt >= a->gate_at && pv[P_ARP_GATE] < 127u) arp_stop_note(p);
    }
}

/* ---- note events (ISR), per sound: its held keys, its arp, its HOLD; the pedal holds both */
static uint32_t sus_was;                        /* bit p: sound p was held */
static inline uint32_t part_sus(uint32_t p) { return ctl_sustain || (ctl_hold >> p & 1u); }
static void ev_note_on(uint32_t p, uint32_t note, uint32_t vel)
{
    uint32_t i, arp_on = pt[p].p->v[P_ARP_MODE] != A_OFF, sus = part_sus(p);
    if (arp_on && sus) {                        /* latch: a new chord after all keys were up starts over */
        for (i = 0; i < nheld[p]; i++)
            if (held[p][i].phys) break;
        if (i == nheld[p]) { nheld[p] = 0; arp[p].pos = 0; arp[p].cnt = 0xFFFFFFFFu; }
    }
    held_remove(p, note);
    if (nheld[p] == NHELD) { for (i = 1; i < NHELD; i++) held[p][i - 1u] = held[p][i]; nheld[p]--; }
    held[p][nheld[p]].note = (uint8_t)note;
    held[p][nheld[p]].vel = (uint8_t)vel;
    held[p][nheld[p]].phys = 1;
    nheld[p]++;
    if (arp_on) {
        if (nheld[p] == 1u) { arp[p].pos = 0; arp[p].cnt = 0xFFFFFFFFu; }   /* the first key: a step right away */
        return;
    }
    voice_trigger(p, note, vel);
}

static void ev_note_off(uint32_t p, uint32_t note)
{
    uint32_t i;
    for (i = 0; i < nheld[p]; i++)
        if (held[p][i].note == note) held[p][i].phys = 0;
    if (part_sus(p)) return;                    /* the pedal / its HOLD keeps it */
    held_remove(p, note);
    if (pt[p].p->v[P_ARP_MODE] != A_OFF) return;
    voice_release(p, note, 0);
}

static void ev_sustain_up(uint32_t p)           /* the pedal / its HOLD let go: what no key holds is released */
{
    uint32_t i, j;
    uint8_t gone[NHELD], ng = 0;
    for (i = j = 0; i < nheld[p]; i++)
        if (held[p][i].phys) held[p][j++] = held[p][i];
        else gone[ng++] = held[p][i].note;
    nheld[p] = j;
    if (pt[p].p->v[P_ARP_MODE] != A_OFF) return;
    for (i = 0; i < ng; i++) voice_release(p, gone[i], 0);
}

static void parts_off(uint32_t p)               /* a sound gone (B back on A, a panic): its keys, its arp, its notes */
{
    nheld[p] = 0;
    arp_stop_note(p);
    arp[p].was_on = 0;
    voices_release_part(p);
}

static void engine_events(void)
{
    uint32_t sus = part_sus(0) | part_sus(1) << 1;
    if (ctl_panic) { ctl_panic = 0; parts_off(0); parts_off(1); voices_release_all(); }
    if (sus_was & ~sus & 1u) ev_sustain_up(0);  /* each sound on its own: A's HOLD off never touches B */
    if (sus_was & ~sus & 2u) ev_sustain_up(1);
    sus_was = sus;
    while (nq_r != nq_w) {
        nev_t e = nq[nq_r % NQ];
        uint32_t p = e.part && b_own;           /* (B without its own sound: its keys play A) */
        nq_r++;
        if (e.type) ev_note_on(p, e.note, e.vel);
        else ev_note_off(p, e.note);
    }
}

/* ---- the keyer: an attack / release with analog RC curves per block (CTL frames), level Q24 */
static inline void env_tick(env_t *e, uint32_t a, uint32_t r, int32_t gate, int32_t vint)
{
    int32_t k, target;
    if (gate && e->stage >= E_REL) e->stage = E_ATT;
    if (!gate && e->stage && e->stage < E_REL) e->stage = E_REL;
    switch (e->stage) {
    case E_ATT:
        k = (int32_t)ENV_K_ATT[a];
        k += (int32_t)(((int64_t)k * vint) >> 7);
        target = ENV_ONE + (ENV_ONE * 3 >> 3);  /* aims at 1.3, stops at 1.0 */
        e->level += (int32_t)(((int64_t)(target - e->level) * k) >> 24);
        if (e->level >= ENV_ONE) { e->level = ENV_ONE; e->stage = E_SUS; }
        break;
    case E_DEC:
    case E_SUS:
        e->level = ENV_ONE;
        break;
    case E_REL:
        k = (int32_t)ENV_K_DEC[r];
        k += (int32_t)(((int64_t)k * vint) >> 7);
        e->level += (int32_t)(((int64_t)(0 - e->level) * k) >> 24) - 32;
        if (e->level < LEVEL_FLOOR) { e->level = 0; e->stage = E_IDLE; }
        break;
    default:
        e->level = 0;
    }
}

/* ---- the vibrato: a triangle per sound */
static void lfo_tick(part_t *g)
{
    int32_t p;
    g->lfo_ph += LFO_INC[g->p->v[P_LFO_RATE]];
    p = (int32_t)((g->lfo_ph + 0x40000000u) >> 15);
    g->lfo_val = p < 65536 ? p - 32768 : 98303 - p;
}

/* ---- one control block for a sound: the derived numbers */
static int32_t vol_q12;                         /* the output stage: CC 7 (each sound's VOLUME is in its voices) */

static void block_globals(part_t *g)
{
    const uint8_t *pv = g->p->v;
    int32_t d;
    uint32_t i;
    d = pv[P_VIB]; g->vib_q = (d * d) >> 7;
    g->wheel_q = pv[P_MOD_DEPTH];               /* the knob and the mod wheel (CC 1) are the same value */
    g->bend8 = (ctl_bend * pv[P_BEND] * 256) >> 13;
    g->vint = pv[P_VINTAGE];
    g->velamp = pv[P_VEL];
    d = pv[P_VOLUME]; g->gain_q12 = (d * d) >> 2;
    g->oct8 = ((int32_t)pv[P_OCTAVE] - 1) * 12 * 256;
    g->split = pv[P_SPLIT];
    g->glide_k = GLIDE_K[pv[P_GLIDE]];
    for (i = 0; i < NREG; i++) { d = pv[P_CBASS + i]; g->reg_q12[i] = (d * d * 5) >> 4; }   /* 127 -> 1.23 */
    g->w_lo = (pv[P_CBASS] ? W_16 : 0u) | (pv[P_CELLO] ? W_8LO : 0u);
    g->w_up = (pv[P_VIOLA] ? W_8 : 0u) | (pv[P_VIOLIN] ? W_4 : 0u) | (pv[P_TRUMPET] || pv[P_HORN] ? W_SQ : 0u);
    d = pv[P_WARMTH]; g->warmth_q12 = d ? 4096 + ((d * d * 3 * 4096) / 16129) : 0;   /* up to x4 into the tanh */
    d = pv[P_BRIGHT]; g->bright_q = (d * d) >> 6;   /* 0 .. 252: the presence lift, Q8 */
    g->tone_sm += ((pv[P_TONE] << 8) - g->tone_sm) >> 6;   /* tau = 64 blocks = 23 ms: a CC sweep glides */
    svf_coef(&g->tone_c, g->tone_sm + 9 * 256, 5000);      /* TONE 0 .. 127 = 8 Hz .. 20 kHz on the table, a little lift */
}

static void voice_block(voice_t *v, uint32_t card)
{
    const part_t *g = &pt[v->part];
    const uint8_t *pv = g->p->v;
    int32_t lfo, p8, velg, vint = g->vint, vt = (vint * VINT_ENV[card]) >> 7, a;
    uint32_t glide_k = g->glide_k;
    if (v->pitch16 != v->target16) {            /* glide */
        if (!glide_k) v->pitch16 = v->target16;
        else {
            int32_t diff = v->target16 - v->pitch16;
            v->pitch16 += (int32_t)(((int64_t)diff * glide_k) >> 16);
            if (diff > -64 && diff < 64) v->pitch16 = v->target16;
        }
    }
    env_tick(&v->ae, pv[P_ATTACK], pv[P_RELEASE], v->gate, vt);
    lfo = g->lfo_val;                           /* the vibrato as this keyer sees it: after its delay, faded in */
    if (v->lfo_fade < 32767) {
        uint32_t dl = LFO_DELAY_BLK[pv[P_LFO_DELAY]];
        if (v->lfo_blk >= dl) { v->lfo_fade += 32767 / (int32_t)(dl / 2u + 1u) + 1; if (v->lfo_fade > 32767) v->lfo_fade = 32767; }
        v->lfo_blk++;
    }
    lfo = (lfo * v->lfo_fade) >> 15;
    v->drift += (int32_t)(xorshift(&v->noise) & 31u) - 16;   /* vintage: a slow random walk */
    v->drift -= v->drift >> 11;
    /* pitch, Q8 semitones: the 8' footage is the key */
    p8 = (v->pitch16 >> 8) + g->bend8 + ctl_tune8 + g->oct8 + ((VINT_TUNE8[card] * vint) >> 7) + ((v->drift * vint) >> 11);
    p8 += (lfo * (g->vib_q + g->wheel_q) * 2) >> 15;   /* up to about a semitone */
#if BOARD_SMK37
    p8 += (lfo * ctl_vib) >> 15;
#endif
    v->inc8 = pitch_inc(p8);
    /* which footages this keyer feeds: the bass section's, the upper's, or both (no SPLIT) */
    v->w = (uint8_t)((g->split ? (v->low ? g->w_lo : g->w_up) : g->w_lo | g->w_up));
    /* amp */
    velg = 32767 - ((((127 - (int32_t)v->vel) * g->velamp) >> 7) * 258);
    v->amp0 = v->amp1;
    a = (int32_t)(((int64_t)(v->ae.level >> 9) * velg) >> 15);
    v->amp1 = (a * g->gain_q12) >> 12;          /* the sound's VOLUME */
    ui_vlevel[card] = a;
    ui_vnote[card] = v->note;
}

/* the footage busses, per sound: 16' saw, 8' saw (the bass section's), 8' saw, 4' saw, 8' square */
static int32_t fbus[2][5][HALF_FRAMES] __attribute__((section(".pool")));

/* ---- the samples of one keyer: the locked footages, each into its bus */
static void voice_render(voice_t *v, int32_t (*bus)[HALF_FRAMES], uint32_t at)
{
    uint32_t i, inc8 = v->inc8, inc16 = inc8 >> 1, inc4 = inc8 << 1, d8 = inc8 >> 15, d16 = inc16 >> 15, d4 = inc4 >> 15, w = v->w;
    int32_t amp0 = v->amp0, da = v->amp1 - v->amp0;
    int32_t *b16 = bus[0] + at, *b8lo = bus[1] + at, *b8 = bus[2] + at, *b4 = bus[3] + at, *bsq = bus[4] + at;
    for (i = 0; i < CTL; i++) {
        int32_t amp = amp0 + ((da * (int32_t)i) >> 4), s;
        if (w & W_16) { s = osc_step(&v->o16, inc16, d16, 0, OSC_SAW, -65536, -1); b16[i] += (s * amp) >> 15; }
        if (w & (W_8LO | W_8)) {
            s = (osc_step(&v->o8, inc8, d8, 0, OSC_SAW, -65536, -1) * amp) >> 15;
            if (w & W_8LO) b8lo[i] += s;
            if (w & W_8) b8[i] += s;
        }
        if (w & W_4) { s = osc_step(&v->o4, inc4, d4, 0, OSC_SAW, -65536, -1); b4[i] += (s * amp) >> 15; }
        if (w & W_SQ) { s = osc_step(&v->osq, inc8, d8, PW50, OSC_PLS, -65536, -1); bsq[i] += (s * amp) >> 15; }
    }
}

/* ---- the registers of one sound: fixed formants on its busses, the tone control, warmth, brightness. Mono, as the
 * originals (the ensemble spreads it). dst gets the sound added, both channels. */
static inline __attribute__((always_inline)) int32_t svf_out(const svf_coef_t *c, svf_state_t *s, int32_t in, uint32_t which)
{
    int32_t lp, bp, hp;
    svf_step(c, s, in, &lp, &bp, &hp);
    return which == 0 ? lp : which == 1 ? bp : hp;
}

static void regs_render(part_t *g, uint32_t n, int32_t *dst)
{
    regs_t *r = &g->r;
    int32_t (*bus)[HALF_FRAMES] = fbus[g == &pt[1]];
    uint32_t i, k, any = 0;
    int32_t wq = g->warmth_q12, bq = g->bright_q;
    for (k = 0; k < NREG; k++) any |= (uint32_t)(r->gain[k] | g->reg_q12[k]);
    if (!any) return;
    for (i = 0; i < n; i++) {
        int32_t m = 0, x, y;
        for (k = 0; k < NREG; k++) r->gain[k] += (g->reg_q12[k] - r->gain[k]) >> 7;   /* ~3 ms: no zipper on the knobs */
        if (r->gain[R_CBASS]) {                 /* 16' saw, round: a 2-pole at 900 Hz with a small bump */
            y = svf_out(&C_CB, &r->cb, bus[0][i], 0);
            m += (y * r->gain[R_CBASS]) >> 12;
        }
        if (r->gain[R_CELLO]) {                 /* 8' saw, warm: low-pass 1.8 kHz plus a 400 Hz body */
            x = bus[1][i];
            y = svf_out(&C_CE, &r->ce, x, 0) + (svf_out(&C_CEF, &r->ce_f, x, 1) >> 1);
            m += (y * r->gain[R_CELLO]) >> 12;
        }
        if (r->gain[R_VIOLA]) {                 /* 8' saw, nasal: 250 Hz off the bottom, 3.5 kHz on top, a 1.2 kHz formant */
            x = svf_out(&C_VAH, &r->va_h, bus[2][i], 2);
            y = svf_out(&C_VA, &r->va, x, 0) + ((svf_out(&C_VAF, &r->va_f, x, 1) * 3) >> 3);
            m += (y * r->gain[R_VIOLA]) >> 12;
        }
        if (r->gain[R_VIOLIN]) {                /* 4' saw, bright and airy: 600 Hz off the bottom, 7 kHz on top */
            x = svf_out(&C_VIH, &r->vi_h, bus[3][i], 2);
            y = svf_out(&C_VI, &r->vi, x, 0);
            m += (y * r->gain[R_VIOLIN]) >> 12;
        }
        if (r->gain[R_TRUMPET]) {               /* 8' square, brassy: a 1.5 kHz band plus a 2.5 kHz low-pass */
            x = bus[4][i];
            y = ((svf_out(&C_TR, &r->tr, x, 1) * 5) >> 2) + (svf_out(&C_TRL, &r->tr_l, x, 0) >> 1);
            m += (y * r->gain[R_TRUMPET]) >> 12;
        }
        if (r->gain[R_HORN]) {                  /* 8' square, hollow and mellow: a 600 Hz low-pass */
            y = svf_out(&C_HO, &r->ho, bus[4][i], 0);
            m += (y * r->gain[R_HORN]) >> 12;
        }
        m = (m * 11) >> 7;                      /* x 0.086: a five-note chord on two registers peaks near -6 dBFS */
        m = clampi(m, -140000, 140000);
        m = svf_out(&g->tone_c, &r->tone, m, 0);   /* TONE */
        if (wq) m = softclip(((m >> 3) * wq) >> 9) << 1;   /* WARMTH: into the tanh and out again */
        if (bq) {                               /* BRIGHT: a presence lift above ~3 kHz */
            r->br_lp += ((m - r->br_lp) * 11000) >> 15;
            m += ((m - r->br_lp) * bq) >> 8;
        }
        m = clampi(m, -131000, 131000);
        dst[2u * i] += m;
        dst[2u * i + 1u] += m;
    }
}

static uint32_t seq_b_dry(void);                /* (shn_seq.h) how much of track B's sound passes the effects by, 0..127 */
static int32_t bus_b[2u * HALF_FRAMES];         /* track B's own sound: into the effects by its send, the rest dry */

static void engine_audio(int32_t *out, uint32_t n)
{
    uint32_t b, v, i, k, was_own = b_own, dry, bus = b_own, nb;
    static int32_t sc_prev;
    static uint32_t sc_i, sc_sub;
    fx_block(&live, ctl_ext_quarter ? ctl_ext_quarter : 60u * FS / clampi(ctl_tempo, 40, 240));
    vol_q12 = (int32_t)((ctl_cc7 * ctl_cc7) >> 2);
    for (v = 0; v < NV; v++)                    /* (one sound and no B tails: no bus) */
        if (vc[v].part && vc[v].ae.stage != E_IDLE) bus = 1;
    nb = bus ? 2u : 1u;
    for (b = 0; b < nb; b++)
        for (k = 0; k < 5u; k++)
            for (i = 0; i < n; i++) fbus[b][k][i] = 0;
    if (bus)
        for (i = 0; i < 2u * n; i++) bus_b[i] = 0;
    for (b = 0; b < n; b += CTL) {
        parts_point();
        if (was_own && !b_own) parts_off(1);    /* (B back on sound A: its own notes let go) */
        engine_events();
        if (ctl_arp_restart) { ctl_arp_restart = 0; arp[0].pos = arp[1].pos = 0; arp[0].cnt = arp[1].cnt = 0xFFFFFFFFu; }
        arp_tick(0);
        if (b_own) arp_tick(1);
        seq_tick();                             /* (may swap the pattern in, and B's sound with it) */
        parts_point();
        if (was_own && !b_own) parts_off(1);
        was_own = b_own;
        lfo_tick(&pt[0]);
        if (b_own) lfo_tick(&pt[1]);
        block_globals(&pt[0]);
        if (b_own) block_globals(&pt[1]);
        for (v = 0; v < NV; v++) {
            if (vc[v].ae.stage == E_IDLE && !vc[v].gate) {
                if (ui_vlevel[v]) { ui_vlevel[v] = 0; vc[v].amp0 = vc[v].amp1 = 0; }
                continue;
            }
            voice_block(&vc[v], v);
            voice_render(&vc[v], fbus[vc[v].part && bus], b);
        }
    }
    regs_render(&pt[0], n, out);
    if (bus) regs_render(&pt[1], n, bus_b);
    dry = bus ? seq_b_dry() : 0u;
    for (i = 0; bus && i < 2u * n; i++) {              /* B's send: that much joins A on its way into the effects */
        int32_t x = bus_b[i], d = (x * (int32_t)dry) >> 7;
        out[i] += x - d;
        bus_b[i] = d;
    }
    for (i = 0; i < n; i++) {                   /* the output stage: a gentle tanh, never a hard clip */
        int32_t l = softclip((clampi(out[2u * i], -262144, 262143) >> 2) * vol_q12 >> 10);
        int32_t r = softclip((clampi(out[2u * i + 1u], -262144, 262143) >> 2) * vol_q12 >> 10);
        int32_t m = (l + r) >> 1;
        out[2u * i] = l;
        out[2u * i + 1u] = r;
        if (scope_state == 1u) {                /* armed: wait for a rising zero crossing */
            if (sc_prev < 0 && m >= 0) { scope_state = 2; sc_i = 0; sc_sub = 0; }
        } else if (scope_state == 2u) {
            if (!sc_sub++) scope[sc_i++] = (int16_t)m;
            if (sc_sub >= scope_stride) sc_sub = 0;
            if (sc_i >= SCOPE_N) scope_state = 3;
        }
        sc_prev = m;
    }
    fx_process(out, n);
    if (dry)                                    /* B's dry part: its own output stage, after the effects */
        for (i = 0; i < 2u * n; i++)
            out[i] = sat16(out[i] + softclip((clampi(bus_b[i], -262144, 262143) >> 2) * vol_q12 >> 10));
    for (i = 0; i < n && click_amp; i++) {     /* the click: after the effects, never recorded */
        int32_t c;
        click_ph += click_inc;
        c = click_ph & 0x80000000u ? click_amp : -click_amp;
        out[2u * i] += c;
        out[2u * i + 1u] += c;
        click_amp -= (click_amp >> 8) + 1;      /* ~6 ms */
        if (click_amp < 64) click_amp = 0;
    }
}

static int engine_quiet(void)                   /* nothing sounds: a flash erase is safe */
{
    uint32_t v;
    for (v = 0; v < NV; v++)
        if (vc[v].ae.stage != E_IDLE || vc[v].gate) return 0;
    return nq_r == nq_w;
}

static void coef_fixed(svf_coef_t *c, int32_t g, int32_t k)   /* a register filter: g = tan(pi f / fs) Q12, k Q12 damping */
{
    int32_t den = 4096 + ((g * (g + k)) >> 12);
    c->a1 = (int32_t)((4096u << 13) / (uint32_t)den);
    c->a2 = (c->a1 * g) >> 12;
    c->a3 = (c->a2 * g) >> 12;
    c->k = k;
}

static void engine_init(void)
{
    uint32_t v;
    for (v = 0; v < NV; v++) vc[v].noise = 0x12345678u * (v + 1u) + 0x9E3779B9u;
    coef_fixed(&C_CB, G_900, 4900);             /* k 1.2: a small bump at the corner */
    coef_fixed(&C_CE, G_1800, 5793);
    coef_fixed(&C_CEF, G_400, 2048);            /* a body resonance */
    coef_fixed(&C_VAH, G_250, 5793);
    coef_fixed(&C_VA, G_3500, 4500);
    coef_fixed(&C_VAF, G_1200, 1800);           /* the viola's formant */
    coef_fixed(&C_VIH, G_600, 5793);
    coef_fixed(&C_VI, G_7000, 4000);
    coef_fixed(&C_TR, G_1500, 2500);
    coef_fixed(&C_TRL, G_2500, 5793);
    coef_fixed(&C_HO, G_600, 6500);
    pt[0].tone_sm = pt[1].tone_sm = 100 << 8;
    fx_init();
}
