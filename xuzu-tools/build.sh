#!/bin/bash
# Build o_C Phazerville (T3.2) without PlatformIO. Usage: build.sh <software_dir> <out_dir> [extra flags...]
set -e
SW=$(realpath "$1"); OUT=$(realpath -m "$2"); shift 2
HERE=$(dirname $(realpath "$0"))/..
CORE=$HERE/tcores/teensy3; EEP=$HERE/teeprom
mkdir -p $OUT/obj
TAG=$(cd $SW && git rev-parse --short HEAD 2>/dev/null || echo nogit)
COMMON="-mcpu=cortex-m4 -mthumb -fsingle-precision-constant -ffunction-sections -fdata-sections -nostdlib -Os --specs=nano.specs \
 -D__MK20DX256__ -DTEENSYDUINO=159 -DARDUINO=10805 -DARDUINO_TEENSY31 -DF_CPU=120000000 -DUSB_MIDI -DLAYOUT_US_ENGLISH -DTEENSY_OPT_SMALLEST_CODE \
 -DCUSTOM_BUILD -DOC_VERSION_EXTRA=\"${VERSION_EXTRA:-_custom}\" -DOC_BUILD_TAG=\"$TAG\" $CUSTOM_BUILD_FLAGS \
 -I$SW/src -I$SW/src/extern -I$CORE -I$EEP -Wall -Wfatal-errors -Wno-unused-variable"
CXXF="$COMMON -std=gnu++17 -fno-exceptions -fpermissive -fno-rtti -felide-constructors -Wno-error=narrowing -fno-threadsafe-statics"
CF="$COMMON"
build() { # src obj
  local s=$1 o=$2
  if [ ! -f $o ] || [ $s -nt $o ] || [ -n "$FORCE" ]; then
    case $s in *.cpp) arm-none-eabi-g++ $CXXF -c $s -o $o ;; *.c) arm-none-eabi-gcc $CF -c $s -o $o ;; *.S) arm-none-eabi-gcc $CF -x assembler-with-cpp -c $s -o $o ;; esac
  fi
}
export -f build; export CXXF CF
FILES=$( (cd $SW/src && find . -name '*.cpp' -o -name '*.c' | sed "s|^|$SW/src/|"); ls $CORE/*.c $CORE/*.cpp $CORE/*.S; ls $EEP/*.cpp)
for f in $FILES; do o=$OUT/obj/$(echo $f | md5sum | cut -c1-10)_$(basename $f).o; echo "$f $o"; done > $OUT/list
cat $OUT/list | xargs -P $(nproc) -n 2 bash -c 'build "$0" "$1"'
arm-none-eabi-gcc -Os -Wl,--gc-sections,--relax,--defsym=__rtc_localtime=0 --specs=nano.specs -mcpu=cortex-m4 -mthumb -fsingle-precision-constant \
  -T$CORE/mk20dx256.ld -o $OUT/firmware.elf $(cut -d' ' -f2 $OUT/list) -lm -lstdc++ 2>&1 | grep -v 'warning: .* has a LOAD segment' || true
arm-none-eabi-objcopy -O ihex -R .eeprom $OUT/firmware.elf $OUT/firmware.hex
arm-none-eabi-size $OUT/firmware.elf
