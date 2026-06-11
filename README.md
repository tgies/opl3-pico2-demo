# opl3-pico2-demo

[Nuked-OPL3-fast](https://github.com/tgies/Nuked-OPL3-fast) running in real time
on an RP2350, playing VGM tunes out an I2S codec, at full fidelity. Able to
maintain realtime on the densest OPL3 music on one core at 294 MHz, or dual core
at 180 MHz.

> [!NOTE]
> This is an experimental proof of concept for research & development purposes.
> It is not a finished end-user product. The intended audience is developers
> working on sound chip emulation on the Pico 2, as a reference and a
> demonstration of an idea.

The dense demoscene track Crystal Oscillator by Otomata Labs sustains real time
at 294 MHz on a single core, or 180 MHz split across both cores, with output
bit-for-bit identical to upstream Nuked-OPL3 either way. Crystal Oscillator is
a pathological worst case (all 36 operator slots active nearly every sample);
typical OPL game music is far sparser and leaves more headroom.

## What! How?

tl;dr: [Nuked-OPL3](https://github.com/nukeykt/Nuked-OPL3), by the brilliant
Nuke.YKT, is a bit slower than it needs to be. I poked around with it a bit and
created [Nuked-OPL3-fast](https://github.com/tgies/Nuked-OPL3-fast), which is
bit-for-bit identical in output to the original Nuked-OPL3, but 1.4x to 2.2x
faster. The pico2 branch vendored here adds RP2350-specific work on top:

- hot code and lookup tables placed in SRAM;
- the per-sample noise LFSR advance is now a handful of word-parallel shifts
  and xors;
- vibrato, tremolo, and the envelope rate machinery were refactored into tables
  rebuilt on register writes, so the per-sample path is mostly loads;
- fast paths that skip fully-silent operators (slightly differently than
  Nuked-OPL3-fast);
- an optional dual-core mode: half the operators plus one of the two stereo
  mixes run on the second core.

Every one of these is bit-exact: single-core, dual-core, and upstream produce
identical sample streams.

## Why?

I mean, if you're reading this, you probably already know, but Yamaha's OPL2 and
OPL3 chips defined the sound of many home computers in the 1980s and 1990s, and
"retrocomputing" enthusiasts (among others) are interested in hardware
emulations of old sound devices e.g. as a basis for DIYing a reasonable
facsimile of an old hard-to-find sound card. Also because it's neat.

## How fast?

Measured floors for Crystal Oscillator (dense 18-channel OPL3, effectively all
36 slots active), against the chip's native 49716 Hz sample rate:

| Configuration | Floor | Comfortable |
| --- | --- | --- |
| Single core | 294 MHz | 300 MHz |
| Dual core | 180 MHz | 186 MHz |

"Floor" means the lowest clock that sustains 49716 Hz without dropping
samples; at the single-core floor the heaviest sections run ~97-99% of the
core. The boot benchmark gives the absolute numbers behind this. The fast core
is about 4060 cycles/sample on a settled synthetic workload and about 5920
cycles/sample worst case (all 36 operators with envelopes in constant motion,
re-keyed so nothing ever goes quiet). 5920 x 49716 ~= 294 MHz, so a single
core at 300 MHz handles any register stream you can throw at it, and typical
game music is a far lighter workload (for instance, most of it is written for
OPL2 and only uses half the available channels, with the other half getting to
take the dead-slot shortcut).

## Can it go even faster?

Probably not much, on this codebase. The envelope generator is still ~46% of
the time and resists further optimization because its state transitions are
data-dependent per sample. I also tried profile-guided optimization, but it
measured 2% *slower*. M33 has no branch predictor and zero-wait-state SRAM, so
I don't think PGO can really help on this core.

## Hardware

It bears repeating that this is a proof of concept, so the demo harness is
written to work with the devboard I had on hand, and I did not waste time making
it super-easily retargetable to different boards, because this code is for
developers to use as a reference, not for end users. I trust that the audience
for this work can easily find the pin assignments etc. they need to change if
they wish to run the demo on different hardware.

Work was done on a [Waveshare
RP2350-Touch-LCD-3.5](https://docs.waveshare.com/RP2350-Touch-LCD-3.5), with
an ES8311 codec. May work without modification on other RP2350 devboards with an
ES8311 but YMMV.

Pin assignment is in `lib/waveshare-audio/src/audio_pio.h` if you need to mess
with it.

> [!NOTE]
> The demo code renders in full stereo, but mixes down to mono because I was
> testing with a built-in mono speaker on my devboard. If you have stereo sound,
> you can wire that up and drop the mixdown.

## Build, flash, monitor

Requires [PlatformIO](https://platformio.org/).

```sh
pio run -e pico2-music-366            # build (plays crystal_oscillator at 366 MHz)
pio run -e pico2-music-366 -t upload  # flash (board in BOOTSEL mode)
pio device monitor -b 115200          # watch the serial diagnostic
```

Or skip the named envs and build any combination directly:

```sh
scripts/flash.sh 294                  # single-core 294 MHz
scripts/flash.sh 180 dual             # dual-core 180 MHz
scripts/flash.sh 294 vreg 1150        # try a lower core voltage
```

The script validates that the clock is PLL-synthesizable and an exact-MCLK
multiple of 240 kHz before building, with the former being a hard failure and
the latter a warning.

On boot the firmware runs a benchmark and prints two results: `steady` (a
fixed synthetic workload) and `worstcase` (all 36 operators kept maximally
busy). This boot block reprints when a serial host connects.

After that the monitor prints a line every 5 s:

```
[5s] chip_rate=49716.0 Hz  total_ticks=N  core1_busy=89.5%  chip_only=88.3%  harness=1.2%  cpu=294.0 MHz  ring_min=1018/1024  underruns=0  mclk=12288000 Hz (+0 ppm)
```

- `chip_rate` is the OPL3 tick rate the renderer sustains (target 49716 Hz).
- `core1_busy` is core 1's render-loop CPU fraction; `chip_only` is the
  `OPL3_Generate` part alone; `harness` is the VGM-player overhead. Dual-core
  builds print `core0_busy` (worker for half the operators plus the right mix)
  instead of the chip/harness split.
- `ring_min` is the audio ring buffer's low-water mark for the window; it
  hovers near 1023 when healthy and craters when the renderer falls behind.
- `underruns` counts distinct times the DMA caught up with the renderer and
  replayed stale samples (note: this is a bit flaky and may not count every
  underrun; trust your ears/the math if you think you're getting some).
- `mclk` reports the achieved I2S master clock and its ppm error from desired.

## Environments

Clock note: the RP2350 PLL can only synthesize certain frequencies, and the
I2S MCLK is exact (0 ppm) only when the system clock is a multiple of 240 kHz.

- `pico2-music-294 / -300 / -307 / -317 / -366 / -384`: single core, default
  tune (`crystal_oscillator`) at that clock.
- `pico2-music-180-dual / -186 / -197 / -216 / -264`: dual core, default tune at
  that clock.
- `pico2-music-<clk>-pm2`: alternate OPL2 tune (see Tunes below).
- `pico2-music-366-prof`: per-slot tier + per-stage cycle profiling over
  serial. The instrumentation slows everything down. You should be looking at
  the ratio between stages.
- `pico2-tone`: 440 Hz square wave, for verifying the audio path alone.
- `pico2-fast / pico2-upstream` (+ `-200/-300`): boot benchmark only, fast
  fork vs upstream Nuked, for A/B timing.

## Tunes

The default is an embedded tune, selected at compile time in `src/tune_vgm.h`
(`-DTUNE_PM2` for the alternate). Regenerate the embedded C arrays from any
VGM/VGZ:

```sh
python3 scripts/vgm_to_header.py INPUT.vgm src/tune_crystal.h --guard TUNE_CRYSTAL_H --name "..." --artist "..."
```

You can also flash the tune separately instead of flashing it along with the
code, if you're repeatedly flashing the code to try stuff out. If you build with
`-DTUNE_FROM_FLASH=1`, or `scripts/flash.sh <clk> flashtune`, the firmware
instead reads the tune from a blob at flash offset 0x200000, which is out of the
way of code uploads:

```sh
python3 scripts/vgm_to_flash.py assets/crystal_oscillator.vgm --load   # once, board in BOOTSEL
```

Crystal Oscillator is bundled in `assets/`.

## Layout

```
src/main.cpp              core-1 audio renderer + core-0 worker/housekeeping, VGM player, diagnostics
src/tune_*.h              embedded VGM data (generated)
lib/nuked-opl3-fast/      optimized OPL3 core (my Nuked-OPL3-fast, pico2 branch)
lib/nuked-opl3-upstream/  stock Nuked-OPL3, for A/B comparison
lib/waveshare-audio/      ES8311 + PIO I2S driver
scripts/vgm_to_header.py  VGM/VGZ -> embedded C header converter
scripts/vgm_to_flash.py   VGM/VGZ -> flash-resident tune blob (build + load)
scripts/flash.sh          build/flash any clock/option combination
assets/                   source VGM for the bundled tune
```

## Licensing

- `lib/nuked-opl3-fast`, `lib/nuked-opl3-upstream`: LGPL 2.1 (see each
  directory's `LICENSE`). Nuked-OPL3 by nukeykt; fast fork by Tony Gies.
- `lib/waveshare-audio`: Espressif MIT, ported from Waveshare's
  RP2350-Touch-LCD-3.5 reference.
- Tunes: Crystal Oscillator courtesy Otomata Labs, used by permission. Other
  tunes may or may not be included; see headers for info. Any permission I might
  have to distribute any given tune as part of this repository may not
  necessarily extend to you; before republishing, ensure you have the necessary
  rights or licenses to do so.
