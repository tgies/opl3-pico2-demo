#!/bin/bash
# Build/flash a music firmware for any clock/voltage/option combination
# without defining a platformio env per permutation. Injects defines via
# PLATFORMIO_BUILD_FLAGS on top of the base pico2-music env.
#
# Usage: scripts/flash.sh <MHz|kHz> [dual] [s12] [prof] [flashtune] [vreg <mV>] [build]
#   scripts/flash.sh 294                  # single-core 294 MHz
#   scripts/flash.sh 180 dual             # dual-core 180 MHz
#   scripts/flash.sh 294 vreg 1150        # try a lower core voltage
#   scripts/flash.sh 366 prof build       # profiling build, no upload
set -e
cd "$(dirname "$0")/.."

KHZ=""
FLAGS=""
TARGET="upload"
while [ $# -gt 0 ]; do
  case "$1" in
    dual)      FLAGS="$FLAGS -DOPL3_DUAL_CORE=1" ;;
    s12)       FLAGS="$FLAGS -DOPL3_DUAL_CORE=1 -DOPL3_BANK_SPLIT=12" ;;
    prof)      FLAGS="$FLAGS -DOPL3_PROFILE" ;;
    flashtune) FLAGS="$FLAGS -DTUNE_FROM_FLASH=1" ;;
    vreg)      shift; FLAGS="$FLAGS -DBENCH_VREG_MV=$1" ;;
    build)     TARGET="" ;;
    *)         KHZ="$1" ;;
  esac
  shift
done

if ! [[ "$KHZ" =~ ^[0-9]+(\.[0-9]+)?$ ]]; then
  echo "usage: $0 <MHz|kHz> [dual] [s12] [prof] [flashtune] [vreg <mV>] [build]"
  exit 2
fi
# MHz shorthand: 294 -> 294000, 316.8 -> 316800
if [[ "$KHZ" == *.* ]]; then
  KHZ=$(awk "BEGIN{printf \"%d\", $KHZ * 1000}")
elif [ "$KHZ" -lt 10000 ]; then
  KHZ=$((KHZ * 1000))
fi

# Exact I2S MCLK wants a multiple of 240 kHz; the PLL wants vco = 12 MHz *
# int within 750-1600 MHz over postdivs 1-7 x 1-7.
if [ $((KHZ % 240)) -ne 0 ]; then
  echo "warning: $KHZ kHz is not a multiple of 240 kHz (mclk will carry ppm error)"
fi
ok=""
for p1 in 1 2 3 4 5 6 7; do
  for p2 in 1 2 3 4 5 6 7; do
    vco=$((KHZ * p1 * p2))
    if [ "$vco" -ge 750000 ] && [ "$vco" -le 1600000 ] && [ $((vco % 12000)) -eq 0 ]; then
      ok=1
    fi
  done
done
if [ -z "$ok" ]; then
  echo "error: $KHZ kHz is not PLL-synthesizable from the 12 MHz crystal"
  exit 1
fi

FLAGS="-DBENCH_CLOCK_KHZ=$KHZ$FLAGS"
echo "+ PLATFORMIO_BUILD_FLAGS=\"$FLAGS\" pio run -e pico2-music ${TARGET:+-t $TARGET}"
PLATFORMIO_BUILD_FLAGS="$FLAGS" exec pio run -e pico2-music ${TARGET:+-t $TARGET}
