#include <graphx.h>
#include <keypadc.h>
#include <stdint.h>
#include <stdio.h>
#include <ti/getcsc.h>

bool partial_redraw;

void begin();
void end();
bool step();
void draw();
uint8_t get_single_key_pressed(void);
void PrintCentered(const char *str);

int main() {
  begin();
  gfx_Begin();
  gfx_SetDrawBuffer();

  while (step()) {
    if (partial_redraw)
      gfx_BlitScreen();
    draw();
    gfx_SwapDraw();
  }

  gfx_End();
  end();
}

void begin() {}

void end() {}

bool step() {
  if (os_GetCSC() == sk_Clear) {
    return false;
  }
  return true;
}

uint8_t last_key;
void draw() {
  gfx_FillScreen(255);
  uint8_t key = get_single_key_pressed();
  if (key != 0 && key != last_key)
    last_key = key;
  char buffer[32];
  sprintf(buffer, "Key pressed: %i", last_key);
  PrintCentered(buffer);
}

uint8_t get_single_key_pressed(void) {
  static uint8_t last_key;
  uint8_t only_key = 0;
  kb_Scan();
  for (uint8_t key = 1, group = 7; group; --group) {
    for (uint8_t mask = 1; mask; mask <<= 1, ++key) {
      if (kb_Data[group] & mask) {
        if (only_key) {
          last_key = 0;
          return 0;
        } else {
          only_key = key;
        }
      }
    }
  }
  if (only_key == last_key) {
    return 0;
  }
  last_key = only_key;
  return only_key;
}

void PrintCentered(const char *str) {
  gfx_PrintStringXY(str, (GFX_LCD_WIDTH - gfx_GetStringWidth(str)) / 2,
                    (GFX_LCD_HEIGHT - 8) / 2);
}
