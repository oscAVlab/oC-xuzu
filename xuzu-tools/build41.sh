#!/bin/bash
# Build o_C Phazerville for Teensy 4.1 (o_C T4.1 / O.R.N.8) without PlatformIO. Usage: build41.sh <software_dir> <out_dir>
set -e
SW=$(realpath "$1"); OUT=$(realpath -m "$2"); shift 2
HERE=$(realpath $(dirname $(realpath "$0"))/..)
CORE=$HERE/tcores/teensy4; L=$HERE/tlibs
mkdir -p $OUT/obj
TAG=$(cd $SW && git rev-parse --short HEAD 2>/dev/null || echo nogit)
LIBS="Audio Audio/utility SD/src SdFat/src USBHost_t36 Wire SPI SerialFlash LittleFS/src MIDI/src teensy-variable-playback/src"
INC="-I$SW/src -I$SW/src/extern -I$CORE -I$HERE/teeprom"; for l in $LIBS; do INC="$INC -I$L/$l"; done
COMMON="-mcpu=cortex-m7 -mfloat-abi=hard -mfpu=fpv5-d16 -mthumb -ffunction-sections -fdata-sections -nostdlib -Os \
 -D__IMXRT1062__ -DTEENSYDUINO=159 -DARDUINO=10805 -DARDUINO_TEENSY41 -DF_CPU=600000000 -DUSB_MIDI -DLAYOUT_US_ENGLISH -DTEENSY_OPT_SMALLEST_CODE \
 -DCUSTOM_BUILD -DOC_VERSION_EXTRA=\"${VERSION_EXTRA:-_T41}\" -DOC_BUILD_TAG=\"$TAG\" $CUSTOM_BUILD_FLAGS $INC -Wall -Wfatal-errors -Wno-unused-variable"
CXXF="$COMMON -std=gnu++17 -fno-exceptions -fpermissive -fno-rtti -felide-constructors -Wno-error=narrowing -fno-threadsafe-statics"
CF="$COMMON"
build() {
  local s=$1 o=$2
  if [ ! -f $o ] || [ $s -nt $o ] || [ -n "$FORCE" ]; then
    case $s in *.cpp) arm-none-eabi-g++ $CXXF -c $s -o $o ;; *.c) arm-none-eabi-gcc $CF -c $s -o $o ;; *.S) arm-none-eabi-gcc $CF -x assembler-with-cpp -c $s -o $o ;; esac
  fi
}
export -f build; export CXXF CF
FILES=$( (cd $SW/src && find . -name '*.cpp' -o -name '*.c' | sed "s|^|$SW/src/|"); ls $CORE/*.c $CORE/*.cpp $CORE/*.S 2>/dev/null;
  for l in $LIBS; do find $L/$l -maxdepth 1 ! -name filter_variable.cpp \( -name '*.cpp' -o -name '*.c' -o -name '*.S' \); done;
  find $L/SdFat/src $L/LittleFS/src -mindepth 2 \( -name '*.cpp' -o -name '*.c' \) )
for f in $FILES; do o=$OUT/obj/$(echo $f | md5sum | cut -c1-10)_$(basename $f).o; echo "$f $o"; done > $OUT/list
cat $OUT/list | xargs -P $(nproc) -n 2 bash -c 'build "$0" "$1"'
# CMSIS-DSP functions the Audio library uses (Teensyduino ships them as libarm_cortexM7lfsp_math.a)
DSP=$L/cmsis/CMSIS/DSP
if [ ! -f $OUT/libdsp.a ]; then
  for fn in arm_add_f32 arm_add_q15 arm_fill_f32 arm_fir_decimate_f32 arm_fir_decimate_init_f32 arm_fir_interpolate_f32 \
            arm_fir_interpolate_init_f32 arm_float_to_q15 arm_mult_f32 arm_mult_q15 arm_q15_to_float arm_scale_f32 arm_scale_q15; do
    arm-none-eabi-gcc -mcpu=cortex-m7 -mfloat-abi=hard -mfpu=fpv5-d16 -mthumb -Os -ffunction-sections -fdata-sections -DARM_MATH_CM7 -D__FPU_PRESENT=1 \
      -I$DSP/Include -I$DSP/PrivateInclude -I$L/cmsis/CMSIS/Core/Include -c $(find $DSP/Source -name $fn.c) -o $OUT/obj/dsp_$fn.o
  done
  arm-none-eabi-ar rcs $OUT/libdsp.a $OUT/obj/dsp_*.o
fi
arm-none-eabi-gcc -Os -Wl,--gc-sections,--relax -mcpu=cortex-m7 -mfloat-abi=hard -mfpu=fpv5-d16 -mthumb \
  -T$HERE/tools/ld/imxrt1062_t41.ld -o $OUT/firmware.elf $(cut -d' ' -f2 $OUT/list) $OUT/libdsp.a -lm -lstdc++ 2>&1 | grep -v 'LOAD segment' || true
arm-none-eabi-objcopy -O ihex -R .eeprom $OUT/firmware.elf $OUT/firmware.hex
arm-none-eabi-size $OUT/firmware.elf
