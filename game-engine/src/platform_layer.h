#ifndef PLATFORM_LAYER_H
#define PLATFORM_LAYER_H

typedef struct {
  void *Memory;
  int Width;
  int Height;
  int BytesPerPixel;
} engine_offscreen_buffer;

typedef struct {
  unsigned char *buffer;
  int SoundPlaying;
} engine_sound_buffer;

typedef void (*loopCallbackFn)(engine_offscreen_buffer *,
                               engine_sound_buffer *);
extern loopCallbackFn platformLoopCallback;

void platformMain(loopCallbackFn loopCallback);

#endif // PLATFORM_LAYER_H
