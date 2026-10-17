/* SPDX-License-Identifier: GPL-3.0-only */
/* The effects after the output stage: the ensemble (three bucket-brigade lines swayed by two LFOs, 120 degrees
 * apart, the heart of every string machine), a phaser (six allpass stages, the string machine's classic companion),
 * a ping-pong delay in note values of the tempo, and a plate reverb (J. Dattorro's figure-of-eight tank, 1997).
 * All lines are int16 in one .pool buffer; the signals are Q15 as the output stage leaves them. Integers only.
 * REV_DIV 1: the reverb at the full rate (the sound comes first). 2: at half the rate, if the chip turns out
 * to need it: a dark hall loses little by it and it halves the reverb's time and memory. */
#ifndef REV_DIV
#define REV_DIV 1u
#endif
#define RVL(x) ((uint32_t)((x) * 44100u / 29761u / REV_DIV))   /* the paper's 29761 Hz lengths at our rate */
#define DLY_MAX 32768u                          /* a side of the delay: 743 ms */
#define ENS_N 1024u                             /* the ensemble's line: up to 23 ms */
#define REV_PRE_N (8192u / REV_DIV)             /* up to 120 ms of pre-delay */
#define ENS_BASE 265                            /* the lines' middle: 6 ms */
#define RV_IN1 RVL(142)
#define RV_IN2 RVL(107)
#define RV_IN3 RVL(379)
#define RV_IN4 RVL(277)
#define RV_AP1 RVL(672)
#define RV_D1 RVL(4453)
#define RV_AP2 RVL(1800)
#define RV_D2 RVL(3720)
#define RV_AP3 RVL(908)
#define RV_D3 RVL(4217)
#define RV_AP4 RVL(2656)
#define RV_D4 RVL(3163)
#define REV_N (REV_PRE_N + RV_IN1 + RV_IN2 + RV_IN3 + RV_IN4 + RV_AP1 + 16u + RV_D1 + RV_AP2 + RV_D2 + RV_AP3 + 16u + RV_D3 + RV_AP4 + RV_D4)
#define RV_BW (REV_DIV == 2u ? 24000 : 16700)   /* the input's one-pole: ~5 kHz either way */
#define FX_MEM_N (ENS_N + 2u * DLY_MAX + REV_N)

typedef struct { int16_t *b; uint32_t len, w; } line_t;   /* w: where the next sample goes */
static int16_t fx_mem[FX_MEM_N] __attribute__((section(".pool")));
static uint32_t fx_used;

static inline int32_t sat16(int32_t v) { return v > 32767 ? 32767 : v < -32767 ? -32767 : v; }
static void line_init(line_t *l, uint32_t len) { l->b = fx_mem + fx_used; l->len = len; l->w = 0; fx_used += len; }
static inline __attribute__((always_inline)) int32_t line_read(const line_t *l, uint32_t back)   /* 1 <= back <= len */
{
    uint32_t i = l->w + l->len - back;
    if (i >= l->len) i -= l->len;
    return l->b[i];
}
static inline __attribute__((always_inline)) void line_write(line_t *l, int32_t v)
{
    l->b[l->w] = (int16_t)sat16(v);
    if (++l->w >= l->len) l->w = 0;
}
/* Dattorro's allpass: the line holds the inner node, the output is tapped from it. g Q15, signed */
static inline __attribute__((always_inline)) int32_t ap_step(line_t *l, int32_t in, int32_t g)
{
    int32_t d = line_read(l, l->len), v = in - ((d * g) >> 15);
    line_write(l, v);
    return d + ((v * g) >> 15);
}
/* the same with the delay swaying by e (Q8 samples, |e| <= 8 << 8) around base; the line is base + 16 long */
static inline __attribute__((always_inline)) int32_t ap_mod(line_t *l, int32_t in, int32_t g, uint32_t base, int32_t e)
{
    uint32_t back = (uint32_t)((int32_t)base + (e >> 8));
    int32_t f = e & 255, a = line_read(l, back), b = line_read(l, back + 1u), d = a + (((b - a) * f) >> 8), v;
    v = in - ((d * g) >> 15);
    line_write(l, v);
    return d + ((v * g) >> 15);
}
/* a sine, Q15, interpolated between the table's 256 steps (a plain lookup stepped the ensemble's lines by
 * samples: it crackled) */
static inline __attribute__((always_inline)) int32_t sin_q15(uint32_t ph)
{
    uint32_t i = ph >> 24, f = (ph >> 16) & 255u;
    int32_t a = SIN_Q15[i], b = SIN_Q15[(i + 1u) & 255u];
    return a + (((b - a) * (int32_t)f) >> 8);
}
static inline int32_t tri_q(uint32_t ph)        /* a triangle -2048..2047 (so Q8 = +-8 samples) */
{
    int32_t t = (int32_t)(ph >> 19);            /* 0..8191 */
    return t < 4096 ? t - 2048 : 6143 - t;
}

/* ---- the state */
static line_t ens, dl, dr, rv_pre, rv_in[4], rv_ap1, rv_d1, rv_ap2, rv_d2, rv_ap3, rv_d3, rv_ap4, rv_d4;
static uint32_t ens_ph_s, ens_ph_f, ens_inc_s, ens_inc_f, ens_mode, rv_ph1, rv_ph2;
static int32_t ens_depth_s, ens_depth_f, ens_mix, ens_lp, ens_lp2, ens_lpl, ens_lpl2, ens_lpr, ens_lpr2, ens_cur_s, ens_cur_f, dly_mix, rev_mix, dly_fb, dly_tone_k, rev_decay, rev_damp_k;
/* the phaser: six first-order allpasses a channel, the LFO a quarter turn apart left and right */
static int32_t pha_x[2][6], pha_y[2][6], pha_fb_s[2], pha_mix, pha_fb, pha_depth;
static uint32_t pha_ph, pha_inc, ens_active, pha_active;
static int32_t dly_time, dly_cur;               /* Q16 samples: the target and where the read is (it slews like a tape) */
static int32_t dly_lpl, dly_lpr, rv_lp_in, rv_lpl, rv_lpr, rv_yl, rv_yr;
#if REV_DIV == 2u
static int32_t rv_hold;
#endif
static uint32_t rev_pre_n = 1, dly_active, rev_active;
#if REV_DIV == 2u
static uint32_t fx_odd;
#endif

static void fx_init(void)
{
    line_init(&ens, ENS_N);
    line_init(&dl, DLY_MAX);
    line_init(&dr, DLY_MAX);
    line_init(&rv_pre, REV_PRE_N);
    line_init(&rv_in[0], RV_IN1); line_init(&rv_in[1], RV_IN2); line_init(&rv_in[2], RV_IN3); line_init(&rv_in[3], RV_IN4);
    line_init(&rv_ap1, RV_AP1 + 16u); line_init(&rv_d1, RV_D1); line_init(&rv_ap2, RV_AP2); line_init(&rv_d2, RV_D2);
    line_init(&rv_ap3, RV_AP3 + 16u); line_init(&rv_d3, RV_D3); line_init(&rv_ap4, RV_AP4); line_init(&rv_d4, RV_D4);
    dly_cur = dly_time = 11025 << 16;
}

/* the patch's numbers, once per ISR call. quarter: frames per quarter note (the arp's clock) */
static void fx_block(const patch_t *p, uint32_t quarter)
{
    int32_t d, t;
    uint32_t div = p->v[P_DLY_TIME] % NDDIV, samples;
    /* the ensemble: a slow LFO near 0.6 Hz and a fast one near 6 Hz (RATE scales both, 0.5 .. 2 x), the slow one
     * sways the lines by up to +-3 ms, the fast one adds a shimmer of a tenth of that */
    d = p->v[P_ENS_RATE]; t = 512 + ((d - 64) * (d > 64 ? 8 : 4));   /* Q10: 0.5 .. 1.0 .. 1.5 */
    ens_inc_s = (uint32_t)((58435u * (uint32_t)t) >> 10);   /* 0.6 Hz in 2^32 / 44100 steps */
    ens_inc_f = (uint32_t)((584350u * (uint32_t)t) >> 10);  /* 6.0 Hz */
    d = p->v[P_ENS_DEPTH]; ens_depth_s = (d * d * 11) >> 9;   /* x the sine's Q15 = Q16 samples: up to +-3.9 ms at 127 */
    ens_depth_f = ens_depth_s / 5;              /* the fast shimmer at a fifth: the creamy part */
    ens_mode = p->v[P_ENS_MODE] % NENS;
    ens_mix = ens_mode == E_OFF ? 0 : (p->v[P_ENS_MIX] * 256) >> 7;   /* Q8, up to 1.0 = the lines only, as the originals */
    if (ens_mix) ens_active = 2u * FS; else if (ens_active > HALF_FRAMES) ens_active -= HALF_FRAMES; else ens_active = 0;
    /* the phaser: 0.05 .. 4 Hz, the sweep's width, the feedback up to 0.8 */
    d = p->v[P_PHA_RATE]; pha_inc = (uint32_t)(5 + ((d * d * 395) >> 14)) * 974u;
    pha_depth = p->v[P_PHA_DEPTH];
    pha_fb = p->v[P_PHA_FB] * 206;
    pha_mix = (p->v[P_PHA_MIX] * 128) >> 7;     /* Q8, up to 0.5 of each: the classic equal mix */
    if (pha_mix) pha_active = 2u * FS; else if (pha_active > HALF_FRAMES) pha_active -= HALF_FRAMES; else pha_active = 0;
    dly_mix = (p->v[P_DLY_MIX] * 180) >> 7;     /* wet up to 0.7 of the dry (Q8): the sum stays in range */
    rev_mix = (p->v[P_REV_MIX] * 180) >> 7;
    dly_fb = p->v[P_DLY_FB] * 250;              /* up to 0.97 */
    t = p->v[P_DLY_TONE]; dly_tone_k = clampi(((t * t) >> 6) + 8, 8, 256);
    d = p->v[P_REV_SIZE]; rev_decay = 12000 + ((d * d * 19500) >> 14);   /* 0.37 .. 0.96: the top is a near-freeze */
    d = p->v[P_REV_DAMP]; rev_damp_k = 32768 - d * d * 2;   /* the one-pole's k at the half rate */
    if (REV_DIV == 1u) {                        /* the same corner at the full rate: k' = 1 - sqrt(1 - k) */
        uint32_t x = (uint32_t)(32768 - rev_damp_k) << 15, r = 0, bit = 1u << 15;
        for (; bit; bit >>= 1)
            if ((r + bit) * (r + bit) <= x) r += bit;
        rev_damp_k = 32768 - (int32_t)r;
    }
    rev_pre_n = 1u + (uint32_t)p->v[P_REV_PRE] * (41u / REV_DIV);
    samples = quarter * DDIV_NUM[div] / DDIV_DEN[div];
    if (samples > DLY_MAX - 4u) samples = DLY_MAX - 4u;
    if (samples < 64u) samples = 64u;
    dly_time = (int32_t)(samples << 16);
    if (dly_mix) dly_active = 12u * FS; else if (dly_active > HALF_FRAMES) dly_active -= HALF_FRAMES; else dly_active = 0;
    if (rev_mix) rev_active = 24u * FS; else if (rev_active > HALF_FRAMES) rev_active -= HALF_FRAMES; else rev_active = 0;
}

static inline __attribute__((always_inline)) int32_t line_read_q16(const line_t *l, int32_t back_q16)
{
    uint32_t b = (uint32_t)back_q16 >> 16;
    int32_t f = (back_q16 >> 8) & 255, a = line_read(l, b), c = line_read(l, b + 1u);
    return a + (((c - a) * f) >> 8);
}
/* 4-point Hermite for a tap that is always moving (the chorus): no treble loss between the samples */
static inline __attribute__((always_inline)) int32_t line_read_q16_h(const line_t *l, int32_t back_q16)
{
    uint32_t b = (uint32_t)back_q16 >> 16;
    int32_t f = (back_q16 >> 8) & 255, p0 = line_read(l, b - 1u), p1 = line_read(l, b), p2 = line_read(l, b + 1u), p3 = line_read(l, b + 2u);
    int32_t c1 = (p2 - p0) >> 1, c2 = p0 - ((5 * p1) >> 1) + 2 * p2 - (p3 >> 1), c3 = ((p3 - p0) >> 1) + ((3 * (p1 - p2)) >> 1);
    return p1 + ((f * (c1 + ((f * (c2 + ((f * c3) >> 8))) >> 8))) >> 8);
}

/* one reverb sample at the half rate: in -> pre-delay -> bandwidth -> 4 diffusers -> the two tank halves */
static inline void rev_tick(int32_t in)
{
    int32_t x, a, b, e1 = tri_q(rv_ph1), e2 = tri_q(rv_ph2), dec = rev_decay, dk = rev_damp_k;
    rv_ph1 += 97391u * REV_DIV;                 /* ~1.0 Hz and ~0.7 Hz: the tank's sway against metallic rings */
    rv_ph2 += 68174u * REV_DIV;
    line_write(&rv_pre, in);
    x = line_read(&rv_pre, rev_pre_n);
    rv_lp_in += ((x - rv_lp_in) * RV_BW) >> 15;
    x = ap_step(&rv_in[0], rv_lp_in, 24576);
    x = ap_step(&rv_in[1], x, 24576);
    x = ap_step(&rv_in[2], x, 20480);
    x = ap_step(&rv_in[3], x, 20480);
    /* left half, fed by the right's end */
    a = x + ((line_read(&rv_d4, RV_D4) * dec) >> 15);
    a = ap_mod(&rv_ap1, a, -22938, RV_AP1, e1);
    line_write(&rv_d1, a);
    b = line_read(&rv_d1, RV_D1);
    rv_lpl += ((b - rv_lpl) * dk) >> 15;
    b = ap_step(&rv_ap2, (rv_lpl * dec) >> 15, 16384);
    line_write(&rv_d2, b);
    /* right half, fed by the left's end */
    a = x + ((line_read(&rv_d2, RV_D2) * dec) >> 15);
    a = ap_mod(&rv_ap3, a, -22938, RV_AP3, e2);
    line_write(&rv_d3, a);
    b = line_read(&rv_d3, RV_D3);
    rv_lpr += ((b - rv_lpr) * dk) >> 15;
    b = ap_step(&rv_ap4, (rv_lpr * dec) >> 15, 16384);
    line_write(&rv_d4, b);
    /* the output taps (the paper's), x 0.6 */
    a = line_read(&rv_d3, RVL(266)) + line_read(&rv_d3, RVL(2974)) - line_read(&rv_ap4, RVL(1913)) + line_read(&rv_d4, RVL(1996))
      - line_read(&rv_d1, RVL(1990)) - line_read(&rv_ap2, RVL(187)) - line_read(&rv_d2, RVL(1066));
    b = line_read(&rv_d1, RVL(353)) + line_read(&rv_d1, RVL(3627)) - line_read(&rv_ap2, RVL(1228)) + line_read(&rv_d2, RVL(2673))
      - line_read(&rv_d3, RVL(2111)) - line_read(&rv_ap4, RVL(335)) - line_read(&rv_d4, RVL(121));
    rv_yl = (a * 19661) >> 15;
    rv_yr = (b * 19661) >> 15;
}

static void fx_process(int32_t *out, uint32_t n)
{
    uint32_t i, do_ens = ens_active != 0, do_pha = pha_active != 0, do_dly = dly_active != 0, do_rev = rev_active != 0;
    int32_t em = ens_mix, pm = pha_mix, dm = dly_mix, rm = rev_mix;
    for (i = 0; i < n; i++) {
        int32_t l = out[2u * i], r = out[2u * i + 1u];
        if (do_ens) {                           /* three lines, 120 degrees apart on both LFOs, read with Hermite taps */
            int32_t m = (l + r) >> 1, t0, t1, t2, w0, w1, w2;
            uint32_t ps = ens_ph_s, pf = ens_ph_f;
            ens_lp += ((m - ens_lp) * 22000) >> 15;   /* the bucket brigade's input filter: two poles at ~8 kHz */
            ens_lp2 += ((ens_lp - ens_lp2) * 22000) >> 15;
            line_write(&ens, ens_lp2);
            if (ens_cur_s < ens_depth_s) ens_cur_s++; else if (ens_cur_s > ens_depth_s) ens_cur_s--;   /* DEPTH turned: the lines glide there */
            if (ens_cur_f < ens_depth_f) ens_cur_f++; else if (ens_cur_f > ens_depth_f) ens_cur_f--;
            w0 = (ENS_BASE << 16) + sin_q15(ps) * ens_cur_s + sin_q15(pf) * ens_cur_f;
            w1 = (ENS_BASE << 16) + sin_q15(ps + 0x55555555u) * ens_cur_s + sin_q15(pf + 0x55555555u) * ens_cur_f;
            w2 = (ENS_BASE << 16) + sin_q15(ps + 0xAAAAAAAAu) * ens_cur_s + sin_q15(pf + 0xAAAAAAAAu) * ens_cur_f;
            ens_ph_s += ens_inc_s;
            ens_ph_f += ens_inc_f;
            t0 = line_read_q16_h(&ens, w0);
            t1 = line_read_q16_h(&ens, w1);
            t2 = line_read_q16_h(&ens, w2);
            {
                int32_t wl, wr;
                if (ens_mode == E_MONO) {       /* one output, as the originals: the three summed */
                    wl = wr = ((t0 + t1 + t2) * 11) >> 5;
                } else if (ens_mode == E_STEREO) {   /* line 1 left, 3 right, 2 in the middle */
                    wl = (t0 + (t1 >> 1)) * 22 >> 5;
                    wr = (t2 + (t1 >> 1)) * 22 >> 5;
                } else {                        /* WIDE: 1 and 2 left, 3 and 1 inverted right: the widest field */
                    wl = (t0 + t1) * 11 >> 4;
                    wr = (t2 + t1 - (t0 >> 1)) * 11 >> 4;
                }
                ens_lpl += ((wl - ens_lpl) * 22000) >> 15;   /* the brigade's reconstruction filter: two poles at ~8 kHz, the cream */
                ens_lpl2 += ((ens_lpl - ens_lpl2) * 22000) >> 15;
                ens_lpr += ((wr - ens_lpr) * 22000) >> 15;
                ens_lpr2 += ((ens_lpr - ens_lpr2) * 22000) >> 15;
                l += ((ens_lpl2 - l) * em) >> 8;
                r += ((ens_lpr2 - r) * em) >> 8;
            }
        }
        if (do_pha) {                           /* six allpasses a side, swept together, the right a quarter turn on */
            int32_t tl = tri_q(pha_ph), tr = tri_q(pha_ph + 0x40000000u), c, ch;
            uint32_t ia = (uint32_t)clampi(64 + ((tl * pha_depth) >> 12), 0, 127), ib = (uint32_t)clampi(64 + ((tr * pha_depth) >> 12), 0, 127);
            pha_ph += pha_inc;
            for (ch = 0; ch < 2; ch++) {
                int32_t a = PHA_A_Q15[ch ? ib : ia], x = (ch ? r : l) + ((pha_fb_s[ch] * pha_fb) >> 15), k;
                x = clampi(x, -60000, 60000);
                for (k = 0; k < 6; k++) {       /* y = -a x + x1 + a y1 */
                    int32_t y = ((-a * x) >> 15) + pha_x[ch][k] + ((a * pha_y[ch][k]) >> 15);
                    pha_x[ch][k] = x;
                    pha_y[ch][k] = y;
                    x = y;
                }
                pha_fb_s[ch] = x;
                c = x;
                if (ch) r += ((c - r) * pm) >> 8; else l += ((c - l) * pm) >> 8;
            }
        }
        if (do_dly) {                           /* ping-pong: the input enters left, the feedback crosses */
            int32_t wl, wr;
            if (dly_cur < dly_time) { dly_cur += 1 << 15; if (dly_cur > dly_time) dly_cur = dly_time; }
            else if (dly_cur > dly_time) { dly_cur -= 1 << 15; if (dly_cur < dly_time) dly_cur = dly_time; }
            wl = line_read_q16(&dl, dly_cur);
            wr = line_read_q16(&dr, dly_cur);
            dly_lpl += ((wr - dly_lpl) * dly_tone_k) >> 8;
            dly_lpr += ((wl - dly_lpr) * dly_tone_k) >> 8;
            line_write(&dl, ((l + r) >> 1) + ((dly_lpl * dly_fb) >> 15));
            line_write(&dr, (dly_lpr * dly_fb) >> 15);
            l += (wl * dm) >> 8;
            r += (wr * dm) >> 8;
        }
        if (do_rev) {
#if REV_DIV == 2u                               /* half rate: a pair in, the output interpolated between ticks */
            int32_t pl = rv_yl, pr = rv_yr, din = l + r;
            if (!(fx_odd ^= 1u)) {
                rev_tick((rv_hold + din) >> 2);
                l += (((pl + rv_yl) >> 1) * rm) >> 8;
                r += (((pr + rv_yr) >> 1) * rm) >> 8;
            } else {
                l += (rv_yl * rm) >> 8;
                r += (rv_yr * rm) >> 8;
            }
            rv_hold = din;
#else
            rev_tick((l + r) >> 1);
            l += (rv_yl * rm) >> 8;
            r += (rv_yr * rm) >> 8;
#endif
        }
        out[2u * i] = sat16(l);
        out[2u * i + 1u] = sat16(r);
    }
}
