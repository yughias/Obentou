#include "utils/overlay.h"

#ifdef __ANDROID__

#include "SDL_MAINLOOP.h"
#include <SDL3_ttf/SDL_ttf.h>

#include "font.ttf.h"

#include "core.h"

#define BUTTON_DETECT_RADIUS 0.12f
#define BUTTON_INVISIBLE_PERCENTAGE 0.70f

static SDL_Texture** button_texts;
static int button_texts_count;

static float get_button_radius(SDL_FRect* rect, float scale) {
    return scale * BUTTON_DETECT_RADIUS * SDL_min(rect->w, rect->h);
}

static void scale_button(control_t control, SDL_FRect* rect, float* x_out, float* y_out, float* r_out) {
    float x = controls_overlays[control][0];
    float y = controls_overlays[control][1];
    float scale = controls_overlays[control][2];
    if (screenWidth > screenHeight) {
        if (x < 0.5f) {
            *x_out = rect->x + x * rect->w;
            *y_out = rect->y + y * rect->h;
        } else {
            *x_out = (screenWidth - rect->w) + x * rect->w;
            *y_out = rect->y + y * rect->h;
        }
    } else {
        *x_out = rect->x + x * rect->w;
        *y_out = rect->y + y * rect->h;
    }
    *r_out = get_button_radius(rect, scale);
}

static void render_overlay(SDL_Renderer* renderer, SDL_Texture* text, float x, float y, float r, float alpha) {
    r *= BUTTON_INVISIBLE_PERCENTAGE;
    int n_of_vertices = 32;
    SDL_Vertex vs[n_of_vertices + 1];
    int indices[n_of_vertices * 3];
    vs[0].position.x = x;
    vs[0].position.y = y;
    vs[0].color.r = 1.0f;
    vs[0].color.g = 1.0f;
    vs[0].color.b = 1.0f;
    vs[0].color.a = alpha;
    for (int j = 0; j < n_of_vertices; j++) {
        float angle = 2.0f * SDL_PI_F * j / n_of_vertices;
        vs[j + 1].position.x = SDL_cosf(angle) * r + x;
        vs[j + 1].position.y = SDL_sinf(angle) * r + y;
        vs[j + 1].color.r = 1.0f;
        vs[j + 1].color.g = 1.0f;
        vs[j + 1].color.b = 1.0f;
        vs[j + 1].color.a = alpha;
    }
    for (int j = 0; j < n_of_vertices; j++) {
        indices[j * 3] = 0;
        indices[j * 3 + 1] = j + 1;
        indices[j * 3 + 2] = (j + 1) % n_of_vertices + 1;
    }
    SDL_RenderGeometry(renderer, NULL, vs, n_of_vertices + 1, indices, n_of_vertices * 3);
    float size = r * 1.5f;
    SDL_FRect rect = {x - size / 2, y - size / 2, size, size};
    SDL_SetTextureAlphaMod(text, alpha * 255);
    SDL_RenderTexture(renderer, text, NULL, &rect);
}

static bool is_button_pressed(float x, float y, float r) {
    int n_devices;
    SDL_TouchID* touch_ids = SDL_GetTouchDevices(&n_devices);
    if (!touch_ids)
        return false;

    bool touched = false;

    for (int d = 0; d < n_devices; d++) {
        int n_fingers;
        SDL_Finger** fingers = SDL_GetTouchFingers(touch_ids[d], &n_fingers);
        if (!fingers) continue;

        for (int i = 0; i < n_fingers; i++) {
            SDL_Finger* f = fingers[i];
            float finger_x_pixels = f->x * screenWidth;
            float finger_y_pixels = f->y * screenHeight;
            
            float dx = x - finger_x_pixels;
            float dy = y - finger_y_pixels;
            if (dx * dx + dy * dy < r * r) {
                touched = true;
                break;
            }
        }
        
        SDL_free(fingers);
        if (touched) break;
    }

    SDL_free(touch_ids);
    return touched;
}

static SDL_FRect get_overlay_borders() {
    float scale_factor = SDL_min(screenWidth / (float)aspectRatioWidth, screenHeight / (float)aspectRatioHeight);
    float scaled_w = aspectRatioWidth * scale_factor;
    float scaled_h = aspectRatioHeight * scale_factor;
    SDL_FRect rect;

    if (screenWidth > screenHeight) {
        float size = SDL_min(screenWidth - scaled_w, scaled_h);
        rect.x = 0;
        rect.y = screenHeight/2 - size/2;
        rect.w = size;
        rect.h = size;
    } else {
        float size = SDL_min(scaled_w, screenHeight - scaled_h);
        rect.x = screenWidth/2 - size/2;
        rect.y = scaled_h + (screenHeight - scaled_h) / 2 - size/2;
        rect.w = size;
        rect.h = size;
    }

    return rect;
}

static void draw_overlay(SDL_Renderer* renderer, void* ctx){
    const core_t* core = ctx;

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

    SDL_FRect rect = get_overlay_borders();

    for (control_t i = core->control_begin; i <= core->control_end; i++) {
        float x = controls_overlays[i][0];
        float y = controls_overlays[i][1];
        if (x < 1e-3f && y < 1e-3f)
            continue;
        float r;
        scale_button(i, &rect, &x, &y, &r);
        float alpha = is_button_pressed(x, y, r) ? 1.0f : 0.5f;
        render_overlay(renderer, button_texts[i - core->control_begin], x, y, r, alpha);
    }
}

void overlay_init(const core_t* core) {
    overlay_clear();
    overlayCtx = (void*)core;
    onDrawOverlay = draw_overlay;

    TTF_Font* font = TTF_OpenFontIO(SDL_IOFromConstMem(assets_font_ttf, sizeof(assets_font_ttf)), true, 36);
    button_texts_count = core->control_end - core->control_begin + 1;
    button_texts = SDL_calloc(button_texts_count, sizeof(SDL_Texture*));
    for (int i = 0; i < button_texts_count; i++) {
        SDL_Color color = { 20, 20, 20, 255 };
        SDL_Surface* tmp = TTF_RenderText_Blended(font, controls_names[core->control_begin + i], 0, color);
        button_texts[i] = SDL_CreateTextureFromSurface(getMainRenderer(), tmp);
        SDL_DestroySurface(tmp);
    }
    TTF_CloseFont(font);
}

bool overlay_pressed(control_t control) {
    SDL_FRect rect = get_overlay_borders();
    float x = controls_overlays[control][0];
    float y = controls_overlays[control][1];
    if (x < 1e-3f && y < 1e-3f)
        return false;
    float r;
    scale_button(control, &rect, &x, &y, &r);
    return is_button_pressed(x, y, r);
}

void overlay_clear() {
    overlayCtx = NULL;
    onDrawOverlay = NULL;

    for (int i = 0; i < button_texts_count; i++)
        SDL_DestroyTexture(button_texts[i]);
    SDL_free(button_texts);
    button_texts = NULL;

    button_texts_count = 0;
}

#else

void overlay_init(const core_t* ctx) {
}

bool overlay_pressed(control_t control) {
    return false;
}

void overlay_clear() {
}

#endif