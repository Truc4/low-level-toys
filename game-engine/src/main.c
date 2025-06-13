#include "platform_layer.h"
#include "win32_layer.c"

internal void EngineOutputSound(engine_sound_buffer *SoundBuffer) {
  // Fill a buffer.
  double phase = 0;
  uint32_t bufferIndex = 0;
  while (bufferIndex < AUDIOBUFFERSIZEINBYTES) {
    phase += (2 * PI) / SAMPLESPERCYCLE;
    int16_t sample = (int16_t)(sin(phase) * INT16_MAX * VOLUME);
    SoundBuffer->buffer[bufferIndex++] = (byte)sample; // little-endian low byte
    SoundBuffer->buffer[bufferIndex++] =
        (byte)(sample >> 8); // little-endian high byte
  }
}

internal void RenderWeirdGradient(engine_offscreen_buffer *Buffer, int XOffset,
                                  int YOffset) {
  int Pitch = Buffer->Width * Buffer->BytesPerPixel;
  uint8_t *Row = (uint8_t *)Buffer->Memory;
  for (int Y = 0; Y < Buffer->Height; ++Y) {
    uint32_t *Pixel = (uint32_t *)Row;
    for (int X = 0; X < Buffer->Width; ++X) {
      uint8_t Blue = (X + XOffset);
      uint8_t Green = (Y + YOffset);
      *Pixel++ = ((Green << 8) | Blue);
    }
    Row += Pitch;
  }
}

global_variable int XOffset = 0;
global_variable int YOffset = 0;

void engineLoop(engine_offscreen_buffer *Buffer,
                engine_sound_buffer *SoundBuffer) {
  XOffset++;
  YOffset++;
  RenderWeirdGradient(Buffer, XOffset, YOffset);
  if (!SoundBuffer->SoundPlaying) {
    EngineOutputSound(SoundBuffer);
  }
}

int main() { platformMain(engineLoop); }
