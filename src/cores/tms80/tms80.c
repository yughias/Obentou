#include "cores/tms80/tms80.h"
#include "cores/tms80/memory.h"

#include "SDL_MAINLOOP.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define KEY(x) SDL_SCANCODE_ ## x
#define NO_KEY KEY(UNKNOWN)

#define tms80_SET_REGION(tms80, zone) \
tms80->cycles_per_frame = zone ## _CYCLES_PER_FRAME; \
tms80->refresh_rate = zone ## _REFRESH_RATE; \
tms80->vdp.region = REGION_ ## zone;

static SDL_Scancode keypad_row_a[8][8] = {
    { KEY(1),  KEY(Q),    KEY(A),    KEY(Z),     KEY(RCTRL),     KEY(COMMA),  KEY(K),            KEY(I)           },
    { KEY(2),  KEY(W),    KEY(S),    KEY(X),     KEY(SPACE),     KEY(PERIOD), KEY(L),            KEY(O)           },
    { KEY(3),  KEY(E),    KEY(D),    KEY(C),     KEY(DELETE),    KEY(SLASH),  KEY(SEMICOLON),    KEY(P)           },
    { KEY(4),  KEY(R),    KEY(F),    KEY(V),     KEY(BACKSPACE), KEY(RALT),   KEY(APOSTROPHE),   KEY(BACKSLASH)   },
    { KEY(5),  KEY(T),    KEY(G),    KEY(B),     NO_KEY,         KEY(DOWN),   KEY(RIGHTBRACKET), KEY(LEFTBRACKET) },
    { KEY(6),  KEY(Y),    KEY(H),    KEY(N),     NO_KEY,         KEY(LEFT),   KEY(RETURN),       NO_KEY           },
    { KEY(7),  KEY(U),    KEY(J),    KEY(M),     NO_KEY,         KEY(RIGHT),  KEY(UP),           NO_KEY           },
    { KEY(UP), KEY(DOWN), KEY(LEFT), KEY(RIGHT), KEY(Z),         KEY(X),      NO_KEY,            NO_KEY           }
};

static SDL_Scancode keypad_row_b[8][4] = {
    { KEY(8),         NO_KEY,    NO_KEY,     NO_KEY      },
    { KEY(9),         NO_KEY,    NO_KEY,     NO_KEY      },
    { KEY(0),         NO_KEY,    NO_KEY,     NO_KEY      },
    { KEY(MINUS),     NO_KEY,    NO_KEY,     NO_KEY      },
    { KEY(EQUALS),    NO_KEY,    NO_KEY,     NO_KEY      },
    { KEY(GRAVE),     NO_KEY,    NO_KEY,     KEY(TAB)    },
    { KEY(RSHIFT),    KEY(LALT), KEY(LCTRL), KEY(LSHIFT) },
    { NO_KEY,         NO_KEY,    NO_KEY,     NO_KEY      }
};

static void load_file(const char* filename, u8** buffer, size_t* size){
    FILE* fptr = fopen(filename, "rb");
    if(!fptr){
        printf("can't open file: %s\n", filename);
        exit(EXIT_FAILURE);
    }

    fseek(fptr, 0, SEEK_END);
    *size = ftell(fptr);
    rewind(fptr);

    *buffer = malloc(*size);
    fread(*buffer, 1, *size, fptr);
    fclose(fptr);
}

void* tms80_init(const char* rom_path, SDL_AudioDeviceID device_id){
    // stub
    const char* bios_path = "";

    tms80_t* tms80 = malloc(sizeof(tms80_t));
    memset(tms80, 0, sizeof(tms80_t));
    z80_init(&tms80->z80);
    tms80->z80.ctx = tms80;
    tms80->z80.readIO = tms80_readIO;
    tms80->z80.writeIO = tms80_writeIO;

    tms80->keypad_reg = 0x7;

    if(
        strstr(rom_path, "(Europe)") || strstr(rom_path, "(E)") ||
        strstr(rom_path, "(Brazil)") || strstr(rom_path, "[E]")
    ){
        tms80_SET_REGION(tms80, PAL);
        printf("PAL!\n");
    } else {
        tms80_SET_REGION(tms80, NTSC);
        printf("NTSC!\n");
    }

    for(int i = 0; i < 3; i++)
        tms80->banks[i] = i;

    if(strcmp(rom_path, ""))
        load_file(rom_path, &tms80->cartridge, &tms80->cartridge_size);
    else {
        tms80->cartridge = malloc(1 << 10);
        memset(tms80->cartridge, 0xFF, 1 << 10);
        tms80->cartridge_size = 1 << 10;
    }

    if(strcmp(bios_path, "")){
        load_file(bios_path, &tms80->bios, &tms80->bios_size);
        tms80->type = tms80_detect_type(bios_path);
    } else
        tms80->type = tms80_detect_type(rom_path);
    
    tms80->has_keyboard = tms80->type == SC3000;
    tms80->vdp.cram_size = tms80->type == GG ? CRAM_SIZE_GG : CRAM_SIZE_SMS;

    if(tms80->type == TMS80_UNKNOWN){
        printf("can't detect tms80!\n");
        exit(EXIT_FAILURE);
    }

    switch(tms80->type){
        case SG1000:
        case SC3000:
        // (the mirrored layout is required for safari hunting)
        tms80->z80.readMemory = tms80->cartridge_size == (1 << 14) ? tms80_sg_sc_mirrored_readMemory : tms80_sg_sc_readMemory;
        tms80->z80.writeMemory = tms80_detect_ram_adapter(tms80->cartridge, tms80->cartridge_size) ? tms80_sg_sc_ram_adapter_writeMemory : tms80_sg_sc_writeMemory;
        break;

        case SMS:
        case GG:
        if(strcmp("", bios_path)){
            tms80->z80.readMemory = tms80_sms_bios_readMemory;
            tms80->z80.writeMemory = tms80_sms_bios_writeMemory;
        } else {
            tms80->z80.readMemory = tms80_sms_readMemory;
            tms80->z80.writeMemory = tms80_sms_writeMemory;
        }
        break;

        default:
        printf("UNKNOWN tms80!\n");
        exit(EXIT_FAILURE);
        break;
    }

    sn76489_t* apu = &tms80->apu;
    if(tms80->type == SMS){
        SN76489_SET_TYPE(apu, SMS);
    } else {
        SN76489_SET_TYPE(apu, GENERIC);
    }
    
    apu->audioSpec.freq = 44100;
    apu->audioSpec.channels = 1;
    apu->audioSpec.format = AUDIO_S16;
    apu->audioSpec.samples = SAMPLE_BUFFER_SIZE;
    apu->audioSpec.callback = NULL;
    apu->audioDev = device_id;

    apu->push_rate_reload = tms80->refresh_rate * tms80->cycles_per_frame / apu->audioSpec.freq;

    if(tms80->vdp.cram_size == CRAM_SIZE_GG){
        size(SCREEN_WIDTH_GG, SCREEN_HEIGHT_GG);
    } else
        size(SCREEN_WIDTH_SMS, SCREEN_HEIGHT_SMS);
        
    setAspectRatio(ASPECT_RATIO);

    return tms80;
}

bool tms80_detect_ram_adapter(u8* cartridge, size_t cartridge_size){
    if(cartridge_size > 0x4000){
        u8* empty_space = malloc(0x2000);
        memset(empty_space, 0xFF, 0x2000);
        
        if(!memcmp(cartridge + 0x2000, empty_space, 0x2000)){
            printf("builtin ram detected!\n");
            return true;
        }

        free(empty_space);
    }

    return false;
}

TMS80_TYPE tms80_detect_type(const char* rom_path){
    const char* dot = strrchr(rom_path, '.');

    if(!dot || dot == rom_path)
        return TMS80_UNKNOWN;

    if(!strcmp(dot, ".sg"))
        return SG1000;

    if(!strcmp(dot, ".sc"))
        return SC3000;

    
    if(!strcmp(dot, ".sms"))
        return SMS;

    if(!strcmp(dot, ".gg"))
        return GG;

    return TMS80_UNKNOWN;
}

u8 tms80_get_keypad_a(tms80_t* tms80){
    if(tms80->force_paddle_controller){
        u8 x = 0xF0;
        if(!tms80->paddle_status)
            x &= ~(1 << 5);
        x |= !tms80->paddle_status ? (mouseX & 0xF) : (mouseX >> 4);
        int ms = SDL_GetMouseState(NULL, NULL);
        if(ms & SDL_BUTTON(SDL_BUTTON_LEFT))
            x &= ~(1 << 4);
        tms80->paddle_status ^= 1;
        return x;
    }

    const Uint8* keystate = SDL_GetKeyboardState(NULL);
    u8 row = tms80->keypad_reg & 0b111;
    u8 out = 0xFF;

    for(int i = 0; i < 8; i++)
        if(keystate[ keypad_row_a[row][i] ])
            out &= ~(1 << i);

    return out;
}

u8 tms80_get_keypad_b(tms80_t* tms80){
    const Uint8* keystate = SDL_GetKeyboardState(NULL);
    u8 row = tms80->keypad_reg & 0b111;
    u8 out = 0xFF;

    for(int i = 0; i < 4; i++)
        if(keystate[ keypad_row_b[row][i] ])
            out &= ~(1 << i);

    return out;
}

u8 tms80_gg_get_start_button(){
    const Uint8* keystate = SDL_GetKeyboardState(NULL);

    if(keystate[SDL_SCANCODE_RETURN])
        return 0x40;

    return 0xC0;
}

void tms80_run_frame(tms80_t* tms80){
    z80_t* z80 = &tms80->z80;
    vdp_t* vdp = &tms80->vdp;
    sn76489_t* apu = &tms80->apu;

    static bool prev_nmi;
    const Uint8* ks = SDL_GetKeyboardState(NULL);
    if(!prev_nmi && ks[SDL_SCANCODE_F1])
        z80_nmi(z80);
    prev_nmi = ks[SDL_SCANCODE_F1];


    while(z80->cycles < tms80->cycles_per_frame){
        u32 old_cycles = z80->cycles;
        int old_line = old_cycles / CYCLES_PER_LINE;
        
        z80_step(z80);

        u32 elapsed = z80->cycles - old_cycles;
        tms80_sn76489_update(apu, elapsed);
        tms80_sn76489_push_sample(apu, elapsed);

        int line = z80->cycles / CYCLES_PER_LINE;
        if(line != old_line){
            vdp->v_counter = old_line;

            if(vdp->regs[0] & (1 << 4)){
                if(old_line <= SCREEN_HEIGHT_SMS){
                    vdp->line_reg -= 1;
                    if(vdp->line_reg == 0xFF){
                        tms80_vdp_fire_interrupt(vdp, z80, false);
                        vdp->line_reg = vdp->regs[0xA];
                    }
                } else {
                    vdp->line_reg = vdp->regs[0xA];
                }
            }

            if(old_line < SCREEN_HEIGHT_SMS){
                if(old_line == 0)
                    vdp->scroll_y = vdp->regs[9];
                tms80_vdp_render_line(vdp, old_line);
            }

            if(old_line == SCREEN_HEIGHT_SMS){
                tms80_vdp_fire_interrupt(vdp, z80, true);
            }
        }
    }

    z80->cycles -= tms80->cycles_per_frame;

    tms80_vdp_show_frame(vdp);
}

bool tms80_detect(const char* filename){
    return tms80_detect_type(filename) != TMS80_UNKNOWN;
}