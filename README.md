# opl3-pico2-demo

[Nuked-OPL3](https://github.com/tgies/Nuked-OPL3-fast) running in real time on a
single core of an RP2350, playing VGM tunes out an I2S codec.

> [!NOTE]
> This is an experimental proof of concept for research & development purposes.
> It is not a finished end-user product. The intended audience is developers
> working on sound chip emulation on the Pico 2, as a reference and a
> demonstration of an idea.

The dense demoscene track Crystal Oscillator by Otomata Labs sustains real time
at 366MHz, with output bit-for-bit identical to upstream Nuked-OPL3. Crystal
Oscillator is a pathological worst case (all 36 operator slots active nearly
every sample); typical OPL game music is far sparser and leaves more headroom. 

## Hardware

Work was done on a [Waveshare
RP2350-Touch-LCD-3.5](https://docs.waveshare.com/RP2350-Touch-LCD-3.5), with
an ES8311 codec. May work without modification on other RP2350 devboards with an
ES8311 but YMMV.

Pin assignment is in `lib/waveshare-audio/src/audio_pio.h` if you need to mess
with it.

## Build, flash, monitor

Requires [PlatformIO](https://platformio.org/).

```sh
pio run -e pico2-music-366            # build (plays crystal_oscillator at 366 MHz)
pio run -e pico2-music-366 -t upload  # flash (board in BOOTSEL mode)
pio device monitor -b 115200          # watch the serial diagnostic
```

The monitor prints a line every 5 s:

```
[5s] chip_rate=49716.0 Hz  total_ticks=N  core1_busy=89.5%  chip_only=88.3%  harness=1.2%  mclk=12288000 Hz (+0 ppm)
```

- `chip_rate` is the OPL3 tick rate the renderer sustains (target 49716 Hz).
- `core1_busy` is core 1's render-loop CPU fraction; `chip_only` is the
  `OPL3_Generate` part alone; `harness` is the VGM-from-flash overhead.
- `mclk` reports the achieved I2S master clock and its ppm error.

## Environments

Clock note: the RP2350 PLL can only synthesize certain frequencies, and the
I2S MCLK is exact (0 ppm) only when the system clock is a multiple of 240 kHz.

- `pico2-music-300 / -307 / -360 / -366 / -378 / -384`: play the default tune
  (`crystal_oscillator`) at that clock.
- `pico2-music-<clk>-pm2`: same clocks, alternate OPL2 tune (see Tunes below).
- `pico2-music-384-prof`: adds per-slot tier + per-stage cycle profiling over
  serial. Heavily perturbs timing and absolute figures are approximate.
- `pico2-tone`: 440 Hz square wave, for verifying the audio path alone.
- `pico2-fast / pico2-upstream` (+ `-200/-250/-300`): boot-time synthetic
  chip-core benchmark, fast fork vs upstream Nuked, for A/B timing.

## Tunes

The embedded tune is selected at compile time in `src/tune_vgm.h`: default is
Crystal Oscillator; define `-DTUNE_PM2` for the alternate. Regenerate the
embedded C arrays from any VGM/VGZ with the converter:

```sh
python3 scripts/vgm_to_header.py INPUT.vgm src/tune_crystal.h --guard TUNE_CRYSTAL_H --name "..." --artist "..."
```

Crystal Oscillator is bundled in `assets/`.

## Layout

```
src/main.cpp              core-1 audio renderer + core-0 housekeeping, VGM player, diagnostic
src/tune_*.h              embedded VGM data (generated)
lib/nuked-opl3-fast/      optimized OPL3 core
lib/nuked-opl3-upstream/  stock Nuked-OPL3, for A/B comparison
lib/waveshare-audio/      ES8311 + PIO I2S driver
scripts/vgm_to_header.py  VGM/VGZ -> C header converter
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
