#include "cores/jace/jace.h"

#include "SDL_MAINLOOP.h"

bool jace_draw_char_ram(void* ctx) {
    jace_t* jace = ctx;

    size(16*8, 8*8);

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int idx = (x / 8) + (y / 8) * (width / 8);
            int px = x & 7;
            int py = y & 7;
            bool pix = jace->cram[idx * 8 + py] & (1 << (px ^ 7));
            pixels[x + y * stride] = pix ? color(255, 255, 255) : color(0, 0, 0); 
        }
    }
        
    return true;
}