#ifndef PLATFORM_LAYER_H
#define PLATFORM_LAYER_H

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>

#define Kilobytes(value) ((value) * 1024)
#define Megabytes(value) (Kilobytes(value) * 1024)
#define Gigabytes(value) (Megabytes(value) * 1024)

#define internal static
#define local_persist static
#define global_variable static

typedef int16_t int16;
typedef int64_t int64;
typedef uint32_t uint32;
typedef uint64_t uint64;

typedef unsigned char byte;

// TODO: reduce sound constants to a minimum
// Sound constants
#define BITSPERSSAMPLE 16
#define SAMPLESPERSEC 44100
#define CYCLESPERSEC 220
#define VOLUME 0.5
#define AUDIOBUFFERSIZEINCYCLES 10
#define PI 3.14159265358979323846

#define SAMPLESPERCYCLE ((int)(SAMPLESPERSEC / CYCLESPERSEC)) // 200
#define AUDIOBUFFERSIZEINSAMPLES                                               \
  (SAMPLESPERCYCLE * AUDIOBUFFERSIZEINCYCLES) // 2000
#define AUDIOBUFFERSIZEINBYTES                                                 \
  ((AUDIOBUFFERSIZEINSAMPLES * BITSPERSSAMPLE) / 8) // 4000
// End sound

typedef struct {
  void *Memory;
  int Width;
  int Height;
  int BytesPerPixel;
} engine_offscreen_buffer;

typedef struct {
  int16 *SampleOut;
  int SoundPlaying;
} engine_sound_buffer;

typedef enum {
  KEY_DOWN,
  KEY_UP,
} InputEventType;

typedef struct {
  InputEventType inputEventType;
  int inputEventKeyCode;
} InputEvent;

typedef struct {
  bool IsInitialized;
  uint64 PermanentStorageSize;
  void *PermanentStorage; // NOTE: MUST be zero initialized by platform!
  uint64 TransientStorageSize;
  void *TransientStorage; // Same here!
} engine_memory;

typedef struct {
  int GreenOffset;
  int BlueOffset;
} engine_state;

internal inline uint32 SafeTruncateUInt64(uint64 Value) {
  assert(Value <= 0xFFFFFFFF);
  uint32 Result = (uint32)Value;
  return Result;
}

#if !RELEASE
typedef struct {
  uint32 ContentsSize;
  void *Contents;
} debug_read_file_result;

debug_read_file_result DEBUGPlatformReadEntireFile(char *Filename);
void DEBUGPlatformFreeFileMemory(void *BitmapMemory);

bool DEBUGPlatformWriteEntireFile(char *Filename, uint32 MemorySize,
                                  void *Memory);
#endif

typedef void (*loopCallbackFn)(engine_memory *, engine_offscreen_buffer *,
                               engine_sound_buffer *);
extern loopCallbackFn platformLoopCallback;

typedef void (*inputEventCallbackFn)(InputEvent *);
extern inputEventCallbackFn platformInputEventCallback;

void platformMain(loopCallbackFn loopCallback,
                  inputEventCallbackFn inputEventCallback);

#endif // PLATFORM_LAYER_H
