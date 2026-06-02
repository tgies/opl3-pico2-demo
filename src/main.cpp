// OPL3 cycle bench + VGM audio output for the Waveshare RP2350-Touch-LCD-3.5.

#include <Arduino.h>
#include <pico/stdlib.h>
#include <hardware/clocks.h>
#include <hardware/vreg.h>
#include <hardware/dma.h>
#include <hardware/pio.h>
#include <RP2350.h>

extern "C" {
#include "opl3.h"
#include "DEV_Config.h"
#include "audio_pio.h"
#include "es8311.h"
}

#ifndef OPL3_VARIANT_NAME
#define OPL3_VARIANT_NAME "unknown"
#endif

#ifndef AUDIO_TEST_TONE
#define AUDIO_TEST_TONE 0
#endif
#ifndef AUDIO_PLAY_TUNE
#define AUDIO_PLAY_TUNE 0
#endif

#if AUDIO_PLAY_TUNE
#include "tune_vgm.h"
#endif

#define CHIP_RATE_NATIVE 49716u
#define BENCH_SAMPLES    50000u
#define WARMUP_SAMPLES   1000u

// chip-core synthetic workload
static void setup_workload(opl3_chip *chip) {
    OPL3_WriteReg(chip, 0x105, 0x01);
    OPL3_WriteReg(chip, 0x001, 0x20);
    OPL3_WriteReg(chip, 0x104, 0x01);

    OPL3_WriteReg(chip, 0x020, 0xA1);
    OPL3_WriteReg(chip, 0x040, 0x14);
    OPL3_WriteReg(chip, 0x060, 0xC8);
    OPL3_WriteReg(chip, 0x080, 0x37);
    OPL3_WriteReg(chip, 0x0E0, 0x00);
    OPL3_WriteReg(chip, 0x023, 0x21);
    OPL3_WriteReg(chip, 0x043, 0x00);
    OPL3_WriteReg(chip, 0x063, 0xF8);
    OPL3_WriteReg(chip, 0x083, 0x07);
    OPL3_WriteReg(chip, 0x0E3, 0x02);
    OPL3_WriteReg(chip, 0x028, 0x61);
    OPL3_WriteReg(chip, 0x048, 0x10);
    OPL3_WriteReg(chip, 0x068, 0xD8);
    OPL3_WriteReg(chip, 0x088, 0x47);
    OPL3_WriteReg(chip, 0x0E8, 0x04);
    OPL3_WriteReg(chip, 0x02B, 0x21);
    OPL3_WriteReg(chip, 0x04B, 0x05);
    OPL3_WriteReg(chip, 0x06B, 0xF0);
    OPL3_WriteReg(chip, 0x08B, 0x0F);
    OPL3_WriteReg(chip, 0x0EB, 0x06);
    OPL3_WriteReg(chip, 0x0A0, 0x44);
    OPL3_WriteReg(chip, 0x0C0, 0x35);
    OPL3_WriteReg(chip, 0x0C3, 0x37);
    OPL3_WriteReg(chip, 0x0B0, 0x32);

    static const uint8_t two_op_chans[] = {1, 2, 4, 5};
    static const uint16_t fnum_lo[] = {0x88, 0xCC, 0x55, 0x99};
    static const uint8_t fnum_hi[] = {0x29, 0x2A, 0x31, 0x33};
    static const uint8_t ch_op_lo[] = {0x00, 0x01, 0x02, 0x08, 0x09, 0x0A};
    static const uint8_t ch_op_hi[] = {0x03, 0x04, 0x05, 0x0B, 0x0C, 0x0D};
    for (int i = 0; i < 4; i++) {
        uint8_t ch = two_op_chans[i];
        uint8_t op_lo = ch_op_lo[ch];
        uint8_t op_hi = ch_op_hi[ch];
        uint8_t wf = (uint8_t)(i & 0x07);
        OPL3_WriteReg(chip, 0x020 + op_lo, 0x21 | (uint8_t)((i & 1) << 7));
        OPL3_WriteReg(chip, 0x040 + op_lo, 0x10 + (uint8_t)i);
        OPL3_WriteReg(chip, 0x060 + op_lo, 0xF0);
        OPL3_WriteReg(chip, 0x080 + op_lo, 0x0F);
        OPL3_WriteReg(chip, 0x0E0 + op_lo, wf);
        OPL3_WriteReg(chip, 0x020 + op_hi, 0x21);
        OPL3_WriteReg(chip, 0x040 + op_hi, 0x00);
        OPL3_WriteReg(chip, 0x060 + op_hi, 0xF0);
        OPL3_WriteReg(chip, 0x080 + op_hi, 0x0F);
        OPL3_WriteReg(chip, 0x0E0 + op_hi, (uint8_t)((wf + 4) & 7));
        OPL3_WriteReg(chip, 0x0A0 + ch, fnum_lo[i]);
        OPL3_WriteReg(chip, 0x0C0 + ch, 0x31);
        OPL3_WriteReg(chip, 0x0B0 + ch, fnum_hi[i]);
    }

    /* 4-op pair on slots 6/7 */
    OPL3_WriteReg(chip, 0x104, 0x03);
    OPL3_WriteReg(chip, 0x026, 0x21);
    OPL3_WriteReg(chip, 0x046, 0x08);
    OPL3_WriteReg(chip, 0x066, 0xF0);
    OPL3_WriteReg(chip, 0x086, 0x0F);
    OPL3_WriteReg(chip, 0x0E6, 0x01);
    OPL3_WriteReg(chip, 0x029, 0x21);
    OPL3_WriteReg(chip, 0x049, 0x00);
    OPL3_WriteReg(chip, 0x069, 0xF0);
    OPL3_WriteReg(chip, 0x089, 0x0F);
    OPL3_WriteReg(chip, 0x0E9, 0x03);
    OPL3_WriteReg(chip, 0x0A6, 0x77);
    OPL3_WriteReg(chip, 0x0B6, 0x32);
    OPL3_WriteReg(chip, 0x0C6, 0x39);
    OPL3_WriteReg(chip, 0x0C9, 0x3B);

    /* rhythm mode */
    OPL3_WriteReg(chip, 0x0BD, 0xFF);
}

struct BenchResult {
    uint32_t cpu_hz;
    uint32_t samples;
    uint32_t elapsed_cycles;
    double cycles_per_sample;
    double ns_per_sample;
    uint64_t checksum;
};

/* cast arcane cortex spell that turns on some kind of TSC */
static void enable_cyccnt(void) {
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CYCCNT = 0;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
}

static BenchResult run_bench(void) {
    opl3_chip chip;
    int16_t s[2];

    OPL3_Reset(&chip, CHIP_RATE_NATIVE);
    setup_workload(&chip);

    /* warm caches, settle envelopes */
    for (uint32_t i = 0; i < WARMUP_SAMPLES; i++) OPL3_Generate(&chip, s);

    /* FNV-1a checksum. hope you like magic numbers */
    uint64_t cs = 0xcbf29ce484222325ULL;
    uint32_t t0 = DWT->CYCCNT;
    for (uint32_t i = 0; i < BENCH_SAMPLES; i++) {
        OPL3_Generate(&chip, s);
        cs = (cs * 0x100000001b3ULL) +
             ((uint16_t)s[0] | ((uint64_t)(uint16_t)s[1] << 16));
    }
    uint32_t t1 = DWT->CYCCNT;

    BenchResult r;
    r.cpu_hz = clock_get_hz(clk_sys);
    r.samples = BENCH_SAMPLES;
    r.elapsed_cycles = t1 - t0;
    r.cycles_per_sample = (double)r.elapsed_cycles / (double)BENCH_SAMPLES;
    r.ns_per_sample = 1e9 * (double)r.elapsed_cycles /
                      ((double)r.cpu_hz * (double)BENCH_SAMPLES);
    r.checksum = cs;
    return r;
}

static void print_result(const BenchResult &r) {
    Serial.printf("---\n");
    Serial.printf("variant=%s\n", OPL3_VARIANT_NAME);
    Serial.printf("cpu_hz=%u\n", r.cpu_hz);
    Serial.printf("samples=%u\n", r.samples);
    Serial.printf("elapsed_cycles=%u\n", r.elapsed_cycles);
    Serial.printf("cycles_per_sample=%.2f\n", r.cycles_per_sample);
    Serial.printf("ns_per_sample=%.2f\n", r.ns_per_sample);
    Serial.printf("real_time_budget_ns=%.2f (at 49716 Hz)\n",
                  1e9 / 49716.0);
    Serial.printf("budget_utilization_pct=%.1f\n",
                  100.0 * r.ns_per_sample / (1e9 / 49716.0));
    Serial.printf("checksum=0x%016llx\n", (unsigned long long)r.checksum);
}

static opl3_chip audio_chip;
static BenchResult boot_result;
static uint16_t boot_chip_id;
static uint32_t last_heartbeat_ms;
static uint32_t sample_counter;

static uint32_t clk_requested_khz = 0;
static bool     clk_ok = false;   /* false if the clock was not PLL-synthesizable */

/* Bresenham resampler, 49716 -> 48000 Hz. */
#define RESAMP_OUT_RATE   48000
#define RESAMP_CHIP_RATE  49716
static int32_t resamp_acc = 0;
static int16_t resamp_prev_chip = 0;

static volatile uint32_t total_chip_ticks = 0;

/* cycles inside OPL3_Generate only (subset of render_cycles) */
static volatile uint32_t chip_cycles = 0;

/* core 1 waits on this; arduino-pico starts it before core 0's setup(). */
static volatile bool core0_audio_ready = false;

/* Aligned to its own size for the DMA ring (auto-wrap) addressing mode. */
#define AUDIO_RING_SAMPLES 1024
#define AUDIO_RING_SIZE_LOG2 12   /* log2(AUDIO_RING_SAMPLES * sizeof(int32_t)) */
static __attribute__((aligned(AUDIO_RING_SAMPLES * sizeof(int32_t))))
    int32_t audio_ring[AUDIO_RING_SAMPLES];
static int audio_dma_chan = -1;
static uint32_t audio_write_idx = 0;

#if AUDIO_PLAY_TUNE
#define VGM_RATE 44100u
#define CHIP_RATE 49716u

struct vgm_player {
    const uint8_t *data;
    size_t pos;
    size_t end_pos;
    size_t initial_pos;
    size_t loop_pos;        /* 0 if no loop */
    uint32_t pending_chip;  /* chip ticks remaining before next command */
    uint32_t rem;           /* VGM->chip conversion remainder */
};
static struct vgm_player vgm;

static inline uint16_t rd_le16(const uint8_t *d, size_t off) {
    return (uint16_t)d[off] | ((uint16_t)d[off + 1] << 8);
}
static inline uint32_t rd_le32(const uint8_t *d, size_t off) {
    return (uint32_t)d[off] | ((uint32_t)d[off + 1] << 8) |
           ((uint32_t)d[off + 2] << 16) | ((uint32_t)d[off + 3] << 24);
}

static void vgm_advance_wait(struct vgm_player *p, uint32_t vgm_samples) {
    /* 44100 Hz wait -> 49716 Hz chip ticks. VGM is an insanely stupid format */
    uint32_t numerator = vgm_samples * CHIP_RATE + p->rem;
    p->pending_chip += numerator / VGM_RATE;
    p->rem = numerator % VGM_RATE;
}

static void vgm_init(struct vgm_player *p, const uint8_t *data, size_t len) {
    p->data = data;
    p->pending_chip = 0;
    p->rem = 0;
    p->loop_pos = 0;
    if (len < 0x40 || memcmp(data, "Vgm ", 4) != 0) {
        p->pos = p->end_pos = p->initial_pos = 0;
        return;
    }
    uint32_t version = rd_le32(data, 0x08);
    size_t eof = (size_t)rd_le32(data, 0x04) + 4u;
    size_t data_off;
    if (version < 0x150) data_off = 0x40;
    else { uint32_t o = rd_le32(data, 0x34); data_off = o == 0 ? 0x40 : (size_t)o + 0x34u; }
    uint32_t loop_off = rd_le32(data, 0x1C);
    if (loop_off != 0) p->loop_pos = (size_t)loop_off + 0x1Cu;
    size_t gd3 = rd_le32(data, 0x14) == 0 ? eof : (size_t)rd_le32(data, 0x14) + 0x14u;
    p->end_pos = (eof < gd3 ? eof : gd3);
    if (len < p->end_pos) p->end_pos = len;
    p->pos = data_off;
    p->initial_pos = data_off;
}

static void vgm_step_commands(struct vgm_player *p) {
    while (p->pending_chip == 0) {
        if (p->pos >= p->end_pos) {
            /* Restart from loop point (or from data offset if no loop). */
            p->pos = p->loop_pos ? p->loop_pos : p->initial_pos;
            continue;
        }
        uint8_t cmd = p->data[p->pos++];
        switch (cmd) {
        case 0x66:
            p->pos = p->loop_pos ? p->loop_pos : p->initial_pos;
            break;
        case 0x61: {
            uint16_t n = rd_le16(p->data, p->pos); p->pos += 2;
            vgm_advance_wait(p, n);
            break;
        }
        case 0x62: vgm_advance_wait(p, 735); break;
        case 0x63: vgm_advance_wait(p, 882); break;
        case 0x5a: case 0x5e:
            OPL3_WriteRegBuffered(&audio_chip, p->data[p->pos], p->data[p->pos + 1]);
            p->pos += 2;
            break;
        case 0x5f:
            OPL3_WriteRegBuffered(&audio_chip,
                                  (uint16_t)p->data[p->pos] | 0x100u,
                                  p->data[p->pos + 1]);
            p->pos += 2;
            break;
        case 0x67: {
            /* data block: [0x66][type][size_le32][payload] */
            p->pos += 2;
            uint32_t size = rd_le32(p->data, p->pos);
            p->pos += 4 + size;
            break;
        }
        /* Fixed-operand commands need skipped past */
        case 0x30: case 0x31: case 0x4f: case 0x50:
        case 0x94:
            p->pos += 1; break;
        case 0x90: case 0x91: case 0x95:
            p->pos += 4; break;
        case 0x92:
            p->pos += 5; break;
        case 0x93:
            p->pos += 10; break;
        default:
            if (cmd >= 0x70 && cmd <= 0x7f) {
                vgm_advance_wait(p, (uint64_t)(cmd & 0x0f) + 1u);
            } else if (cmd >= 0x80 && cmd <= 0x8f) {
                uint64_t n = cmd & 0x0f;
                if (n) vgm_advance_wait(p, n);
            } else if ((cmd >= 0x51 && cmd <= 0x5d) || (cmd >= 0xa0 && cmd <= 0xbf)) {
                p->pos += 2;
            } else if (cmd >= 0x40 && cmd <= 0x4e) {
                p->pos += 2;
            } else if (cmd >= 0xc0 && cmd <= 0xdf) {
                p->pos += 3;
            } else if (cmd >= 0xe0) {
                p->pos += 4;
            }
            break;
        }
    }
}
#endif /* AUDIO_PLAY_TUNE */

static void bump_vreg_for_clock(uint32_t target_khz) {
    enum vreg_voltage v;
    if      (target_khz <= 150000) v = VREG_VOLTAGE_1_10;
    else if (target_khz <= 200000) v = VREG_VOLTAGE_1_15;
    else if (target_khz <= 250000) v = VREG_VOLTAGE_1_20;
    else if (target_khz <= 300000) v = VREG_VOLTAGE_1_25;
    else                           v = VREG_VOLTAGE_1_30;
    vreg_set_voltage(v);
    busy_wait_us(10000);  // 10ms for VREG to settle
}

void setup(void) {
#ifdef BENCH_CLOCK_KHZ
    clk_requested_khz = BENCH_CLOCK_KHZ;
    bump_vreg_for_clock(BENCH_CLOCK_KHZ);
    clk_ok = set_sys_clock_khz(BENCH_CLOCK_KHZ, false);
#endif

    Serial.begin(115200);
    delay(500);

    if (clk_requested_khz) {
        Serial.printf("[clock] requested=%u kHz  achieved=%u kHz  %s\n",
                      (unsigned)clk_requested_khz,
                      (unsigned)(clock_get_hz(clk_sys) / 1000),
                      clk_ok ? "OK" : "UNACHIEVABLE (ran at default)");
    }

    enable_cyccnt();
    boot_result = run_bench();
    print_result(boot_result);

    /* Audio bring-up: I2C, codec init, MCLK, I2S out, then unmute amp. */
    Serial.printf("---\n[audio] DEV_Module_Init\n");
    DEV_Module_Init();
    Serial.printf("[audio] es8311_init\n");
    es8311_init(pico_audio);
    es8311_sample_frequency_config(pico_audio.mclk_freq, pico_audio.sample_freq);
    es8311_voice_volume_set(pico_audio.volume);

    boot_chip_id = es8311_read_id();
    Serial.printf("[audio] ES8311 chip id=0x%04x (expect 0x8311)\n", boot_chip_id);

    Serial.printf("[audio] mclk_pio_init (target %u Hz)\n", pico_audio.mclk_freq);
    mclk_pio_init();
    Serial.printf("[audio] dout_pio_init\n");
    dout_pio_init();

    memset(audio_ring, 0, sizeof(audio_ring));
    audio_dma_chan = dma_claim_unused_channel(true);
    dma_channel_config c = dma_channel_get_default_config(audio_dma_chan);
    channel_config_set_transfer_data_size(&c, DMA_SIZE_32);
    channel_config_set_read_increment(&c, true);
    channel_config_set_write_increment(&c, false);
    channel_config_set_dreq(&c, pio_get_dreq(pico_audio.pio_2, pico_audio.sm_dout, true));
    channel_config_set_ring(&c, false /*write*/, AUDIO_RING_SIZE_LOG2);
    dma_channel_configure(
        audio_dma_chan, &c,
        &pico_audio.pio_2->txf[pico_audio.sm_dout],    /* dest: PIO TX FIFO */
        audio_ring,                                    /* src: ring buffer */
        0xFFFFFFFFu,                                   /* effectively infinite */
        true                                           /* start now */
    );
    Serial.printf("[audio] DMA channel %d active (ring buffer %u samples, %.1f ms @ %u Hz)\n",
                  audio_dma_chan, AUDIO_RING_SAMPLES,
                  1000.0 * AUDIO_RING_SAMPLES / (double)pico_audio.sample_freq,
                  pico_audio.sample_freq);

    /* Native rate: we drive OPL3_Generate at chip rate and resample ourselves,
     * so the chip's own rateratio is unused. */
    OPL3_Reset(&audio_chip, CHIP_RATE_NATIVE);
#if AUDIO_PLAY_TUNE
    vgm_init(&vgm, TUNE_VGM_DATA, TUNE_VGM_LEN);
    Serial.printf("[audio] playing VGM: %s\n[audio] artist: %s\n",
                  TUNE_VGM_NAME, TUNE_VGM_ARTIST);
#else
    setup_workload(&audio_chip);
    Serial.printf("[audio] streaming chip-core workload at %u Hz...\n",
                  pico_audio.sample_freq);
#endif

    last_heartbeat_ms = millis();
    sample_counter = 0;

    __dmb();                   /* release: init writes visible before the flag */
    core0_audio_ready = true;
    __sev();                   /* wake core 1 from __wfe() */
}

void setup1(void) {
    while (!core0_audio_ready) {
        __wfe();
    }
    enable_cyccnt();
    __dmb();           /* acquire; pairs with core 0's release before the flag */
    DEV_PA_Set(1);
}

/* Returns true and fills *out_mono on ticks that emit an output sample. */
static inline bool render_one_chip_tick(int16_t *out_mono) {
    int16_t s[2];
    uint32_t c0 = DWT->CYCCNT;
    OPL3_Generate(&audio_chip, s);
    chip_cycles += DWT->CYCCNT - c0;
    total_chip_ticks++;
    int16_t curr = (int16_t)((s[0] + s[1]) >> 1);

    bool emitted = false;
    resamp_acc += RESAMP_OUT_RATE;
    if (resamp_acc >= RESAMP_CHIP_RATE) {
        resamp_acc -= RESAMP_CHIP_RATE;
        int32_t prev_w = resamp_acc;   /* post-subtract acc = prev weight */
        int32_t curr_w = RESAMP_OUT_RATE - resamp_acc;
        int32_t v = ((int32_t)resamp_prev_chip * prev_w + (int32_t)curr * curr_w)
                    / RESAMP_OUT_RATE;
        *out_mono = (int16_t)v;
        emitted = true;
    }
    resamp_prev_chip = curr;
    return emitted;
}

static inline int16_t render_one_output(void) {
#if AUDIO_TEST_TONE
    return test_tone_sample(sample_counter);
#else
    int16_t mono;
    while (1) {
#if AUDIO_PLAY_TUNE
        vgm_step_commands(&vgm);
        vgm.pending_chip--;
#endif
        if (render_one_chip_tick(&mono)) return mono;
    }
#endif
}

/* core 1 writes, core 0 reads; uint32_t access is atomic on M33, so no lock */
static volatile uint32_t render_cycles = 0;
static volatile uint32_t idle_cycles   = 0;

__attribute__((section(".time_critical"))) __attribute__((hot))
void loop1(void) {
    const uintptr_t base = (uintptr_t)audio_ring;
    uint32_t read_idx = ((uintptr_t)dma_hw->ch[audio_dma_chan].read_addr - base)
                        / sizeof(int32_t);

    uint32_t t_enter = DWT->CYCCNT;
    bool did_work = false;

    while (((audio_write_idx + 1) & (AUDIO_RING_SAMPLES - 1)) != read_idx) {
        did_work = true;
        int16_t mono = render_one_output();
        uint32_t u = (uint16_t)mono;
        audio_ring[audio_write_idx] = (int32_t)((u << 16) | u);
        audio_write_idx = (audio_write_idx + 1) & (AUDIO_RING_SAMPLES - 1);
        sample_counter++;

        read_idx = ((uintptr_t)dma_hw->ch[audio_dma_chan].read_addr - base)
                   / sizeof(int32_t);
    }

    uint32_t t_exit = DWT->CYCCNT;
    uint32_t dt = t_exit - t_enter;
    if (did_work) render_cycles += dt;
    else          idle_cycles   += dt;
}

static inline int16_t test_tone_sample(uint32_t n) {
    const uint32_t half_period = 48000 / (2 * 440);
    return ((n / half_period) & 1) ? 8000 : -8000;
}

void loop(void) {
    /* Core 0 prints core 1's render stats every 5s */
    static uint32_t last_ticks = 0;
    static uint32_t last_render = 0;
    static uint32_t last_chip = 0;
    static uint64_t last_us = 0;
    if (last_us == 0) {
        last_us = time_us_64();
        last_ticks = total_chip_ticks;
        last_render = render_cycles;
        last_chip = chip_cycles;
    }
    delay(5000);
    uint64_t now_us = time_us_64();
    uint32_t now_ticks = total_chip_ticks;
    uint32_t now_render = render_cycles;
    uint32_t now_chip = chip_cycles;
    uint64_t d_us = now_us - last_us;
    uint32_t d_ticks = now_ticks - last_ticks;
    uint32_t d_render = now_render - last_render;
    uint32_t d_chip = now_chip - last_chip;
    double rate = (double)d_ticks * 1e6 / (double)d_us;
    uint32_t sys_hz = clock_get_hz(clk_sys);
    double wall_cycles = (double)d_us * (double)sys_hz / 1e6;
    double cpu_pct  = wall_cycles > 0 ? 100.0 * (double)d_render / wall_cycles : 0.0;
    double chip_pct = wall_cycles > 0 ? 100.0 * (double)d_chip   / wall_cycles : 0.0;
    Serial.printf("[5s] chip_rate=%.2f Hz  total_ticks=%u  "
                  "core1_busy=%.1f%%  chip_only=%.1f%%  harness=%.1f%%  "
                  "mclk=%u Hz (%+d ppm)\n",
                  rate, now_ticks,
                  cpu_pct, chip_pct, cpu_pct - chip_pct,
                  (unsigned)mclk_actual_hz, (int)mclk_error_ppm);
#ifdef OPL3_PROFILE
    /* Per-slot tier counts and per-stage cycle shares, averaged per tick */
    extern volatile uint32_t opl3_prof_trivial, opl3_prof_silent,
                             opl3_prof_sustain, opl3_prof_full;
    extern volatile uint32_t opl3_cyc_fb, opl3_cyc_env,
                             opl3_cyc_phase, opl3_cyc_slotgen;
    static uint32_t last_triv = 0, last_sil = 0, last_sus = 0, last_full = 0;
    static uint32_t last_fb = 0, last_env = 0, last_ph = 0, last_sg = 0;
    uint32_t triv = opl3_prof_trivial, sil = opl3_prof_silent,
             sus = opl3_prof_sustain, full = opl3_prof_full;
    uint32_t fb = opl3_cyc_fb, env = opl3_cyc_env,
             ph = opl3_cyc_phase, sg = opl3_cyc_slotgen;
    double dt2 = (double)d_ticks;
    if (dt2 > 0) {
        Serial.printf("      slots/tick: trivial=%.1f silent=%.1f sustain=%.1f "
                      "FULL=%.1f  (of 36)\n",
                      (triv - last_triv) / dt2, (sil - last_sil) / dt2,
                      (sus - last_sus) / dt2, (full - last_full) / dt2);
        double dfb = fb - last_fb, denv = env - last_env,
               dph = ph - last_ph, dsg = sg - last_sg;
        double tot = dfb + denv + dph + dsg;
        if (tot > 0) {
            Serial.printf("      FULL-path cycle share: fb=%.0f%% env=%.0f%% "
                          "phase=%.0f%% slotgen=%.0f%%\n",
                          100.0 * dfb / tot, 100.0 * denv / tot,
                          100.0 * dph / tot, 100.0 * dsg / tot);
        }
    }
    last_triv = triv; last_sil = sil; last_sus = sus; last_full = full;
    last_fb = fb; last_env = env; last_ph = ph; last_sg = sg;
#endif
    last_us = now_us;
    last_ticks = now_ticks;
    last_render = now_render;
    last_chip = now_chip;
}
