#include "platform_layer.h"
#include "win32_layer.c"

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

static int XOffset = 0;
static int YOffset = 0;

void engineLoop(engine_offscreen_buffer *Buffer) {
  XOffset++;
  YOffset++;
  RenderWeirdGradient(Buffer, XOffset, YOffset);
}

int main() { platformMain(engineLoop); }
