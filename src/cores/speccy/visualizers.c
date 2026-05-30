#include "cores/speccy/speccy.h"

#include "SDL_MAINLOOP.h"

bool speccy_draw_tape_state(speccy_t* speccy) {
    size(256, 256);

    tape_t* tape = &speccy->tape;

    int progress = 0;
    if (tape->tape_size)
        progress = (tape->tape_pos * width) / tape->tape_size;

    for (int x = 0; x < width; x++)
        for (int y = 1; y < height - 1; y++)
            pixels[x + y * stride] = x < progress ? color(0, 0, 255) : color(255, 0, 0);
    
    for (int i = 0; i < width; i += 8)
        pixels[i] = pixels[i + (height - 1) * stride] = color(0, 255, 0);

    return true;
}