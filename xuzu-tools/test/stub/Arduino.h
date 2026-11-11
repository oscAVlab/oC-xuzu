#pragma once
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
template<class T, class A, class B> static inline T constrain(T x, A a, B b) { return x < (T)a ? (T)a : (x > (T)b ? (T)b : x); }
static uint32_t g_micros = 12345;
static inline uint32_t micros() { return g_micros += 61; }
static inline uint32_t millis() { return g_micros / 1000; }
static inline long random(long n) { return n > 0 ? rand() % n : 0; }
static inline long random(long a, long b) { return b > a ? a + rand() % (b - a) : a; }
struct UsbMidiStub { int on = 0, off = 0, rt = 0;
  void sendNoteOn(int, int, int) { on++; } void sendNoteOff(int, int, int) { off++; }
  void sendRealTime(int) { rt++; } enum { Clock = 0xF8, Start = 0xFA, Stop = 0xFC }; };
static UsbMidiStub usbMIDI;
#define FASTRUN
#define DMAMEM
