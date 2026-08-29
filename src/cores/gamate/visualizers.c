#include "cores/gamate/gamate.h"

#include "SDL_MAINLOOP.h"

static int gamate_width = 160;
static int gamate_height = 150;

bool gamate_draw_pixelmap(gamate_t* gamate) {
    size(256, 256);

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            pixels[x + y * stride] = lcd_compose_pixel(&gamate->lcd, x, y);
        }
    }

    for (int screen_y = 0; screen_y < gamate_height; screen_y++) {
        for (int screen_x = 0; screen_x < gamate_width; screen_x++) {
            bool is_border = (screen_x == 0 || screen_x == gamate_width - 1 || screen_y == 0 || screen_y == gamate_height - 1);
            if (gamate->lcd.window_mode && (screen_y == 15 || screen_y == 16)) {
                is_border = true;
            }

            if (is_border) {
                int vram_x, vram_y;
                
                if (gamate->lcd.window_mode && screen_y < 16) {
                    vram_x = screen_x;
                    vram_y = 0xD0 + screen_y;
                } else {
                    vram_x = (screen_x + gamate->lcd.xscroll) % 256;
                    vram_y = (screen_y + gamate->lcd.yscroll) % 200;
                }

                pixels[vram_x + vram_y * stride] = color(255, 0, 0);
            }
        }
    }

    return true;
}