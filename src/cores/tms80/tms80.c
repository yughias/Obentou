#include "cores/tms80/tms80.h"
#include "cores/tms80/memory.h"
#include "utils/sound.h"
#include "utils/controls.h"
#include "utils/archive.h"

#include "SDL_MAINLOOP.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define KEY(x) CONTROL_TMS80_ ## x
#define NO_KEY CONTROL_NONE

#define tms80_SET_REGION(tms80, zone) \
tms80->cycles_per_frame = zone ## _CYCLES_PER_FRAME; \
tms80->refresh_rate = zone ## _REFRESH_RATE; \
tms80->vdp.region = REGION_ ## zone;

static control_t keypad_row_a[8][8] = {
    { KEY(1),  KEY(Q),    KEY(A),    KEY(Z),     KEY(ED),    KEY(COMMA),     KEY(K),             KEY(I)            },
    { KEY(2),  KEY(W),    KEY(S),    KEY(X),     KEY(SPC),   KEY(DOT),       KEY(L),             KEY(O)            },
    { KEY(3),  KEY(E),    KEY(D),    KEY(C),     KEY(HC),    KEY(SLASH),     KEY(SEMICOLON),     KEY(P)            },
    { KEY(4),  KEY(R),    KEY(F),    KEY(V),     KEY(ID),    KEY(PI),        KEY(COLON),         KEY(AT)           },
    { KEY(5),  KEY(T),    KEY(G),    KEY(B),     NO_KEY,     KEY(DA),        KEY(CLOSE_BRACKET), KEY(OPEN_BRACKET) },
    { KEY(6),  KEY(Y),    KEY(H),    KEY(N),     NO_KEY,     KEY(LA),        KEY(CR),            NO_KEY            },
    { KEY(7),  KEY(U),    KEY(J),    KEY(M),     NO_KEY,     KEY(RA),        KEY(UA),            NO_KEY            },
    { KEY(UP), KEY(DOWN), KEY(LEFT), KEY(RIGHT), KEY(BTN_1), KEY(BTN_2),     NO_KEY,             NO_KEY            }
};

static control_t keypad_row_b[8][4] = {
    { KEY(8),      NO_KEY,   NO_KEY,   NO_KEY   },
    { KEY(9),      NO_KEY,   NO_KEY,   NO_KEY   },
    { KEY(0),      NO_KEY,   NO_KEY,   NO_KEY   },
    { KEY(MINUS),  NO_KEY,   NO_KEY,   NO_KEY   },
    { KEY(CARET),  NO_KEY,   NO_KEY,   NO_KEY   },
    { KEY(YEN),    NO_KEY,   NO_KEY,   KEY(FNC) },
    { KEY(BRK),    KEY(GRP), KEY(CTL), KEY(SHF) },
    { NO_KEY,      NO_KEY,   NO_KEY,   NO_KEY   }
};

static TMS80_TYPE detect_type(const archive_t* rom_archive){
    if(archive_get_file_by_ext(rom_archive, "sg"))
        return SG1000;

    if(archive_get_file_by_ext(rom_archive, "sc"))
        return SC3000;

    
    if(archive_get_file_by_ext(rom_archive, "sms"))
        return SMS;

    if(archive_get_file_by_ext(rom_archive, "gg"))
        return GG;

    return TMS80_UNKNOWN;
}

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

void* TMS80_init(const archive_t* rom_archive, const archive_t* bios_archive){
    tms80_t* tms80 = malloc(sizeof(tms80_t));
    memset(tms80, 0, sizeof(tms80_t));
    z80_init(&tms80->z80);
    tms80->z80.ctx = tms80;
    tms80->z80.readIO = tms80_readIO;
    tms80->z80.writeIO = tms80_writeIO;

    tms80->keypad_reg = 0x7;

    file_t* rom_file = archive_get_file_by_ext(rom_archive, "sg");
    if(!rom_file)
        rom_file = archive_get_file_by_ext(rom_archive, "sc");
    if(!rom_file)
        rom_file = archive_get_file_by_ext(rom_archive, "sms");
    if(!rom_file)
        rom_file = archive_get_file_by_ext(rom_archive, "gg");

    file_t* bios_file = archive_get_file_by_ext(bios_archive, "sms");

    if(
        (
            bios_file &&
            (
                strstr(bios_file->path, "(Europe)") || strstr(bios_file->path, "(E)") ||
                strstr(bios_file->path, "(Brazil)") || strstr(bios_file->path, "[E]")
            ) 
        )
        ||
        (
            rom_file &&
            (
                strstr(rom_file->path, "(Europe)") || strstr(rom_file->path, "(E)") ||
                strstr(rom_file->path, "(Brazil)") || strstr(rom_file->path, "[E]")
            )
        )
    ){
        tms80_SET_REGION(tms80, PAL);
        printf("PAL!\n");
    } else {
        tms80_SET_REGION(tms80, NTSC);
        printf("NTSC!\n");
    }

    for(int i = 0; i < 3; i++)
        tms80->banks[i] = i;

    if(rom_file){
        tms80->cartridge = rom_file->data;
        tms80->cartridge_size = rom_file->size;
    } else {
        tms80->no_cartridge = true;
        tms80->cartridge = malloc(1 << 10);
        memset(tms80->cartridge, 0xFF, 1 << 10);
        tms80->cartridge_size = 1 << 10;
    }

    if(bios_file){
        tms80->bios = bios_file->data;
        tms80->bios_size = bios_file->size;
        tms80->type = detect_type(bios_archive);
    } else
        tms80->type = detect_type(rom_archive);
    
    tms80->has_keyboard = tms80->type == SC3000;
    tms80->vdp.cram_size = tms80->type == GG ? CRAM_SIZE_GG : CRAM_SIZE_SMS;
    

    if(tms80->type == TMS80_UNKNOWN){
        printf("can't detect tms80!\n");
        exit(EXIT_FAILURE);
    }

    tms80->bios_masked = true;

    switch(tms80->type){
        case SG1000:
        case SC3000:
        // (the mirrored layout is required for safari hunting)
        tms80->z80.readMemory = tms80->cartridge_size == (1 << 14) ? tms80_sg_sc_mirrored_readMemory : tms80_sg_sc_readMemory;
        tms80->z80.writeMemory = tms80_detect_ram_adapter(tms80->cartridge, tms80->cartridge_size) ? tms80_sg_sc_ram_adapter_writeMemory : tms80_sg_sc_writeMemory;
        break;

        case SMS:
        case GG:
        if(bios_file){
            tms80->bios_masked = false;
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


    float push_rate = tms80->refresh_rate * tms80->cycles_per_frame / 44100.0f;
    sound_set_push_rate(push_rate);

    if(tms80->vdp.cram_size == CRAM_SIZE_GG){
        size(SCREEN_WIDTH_GG, SCREEN_HEIGHT_GG);
    } else
        size(SCREEN_WIDTH_SMS, SCREEN_HEIGHT_SMS);
        
    setAspectRatio(ASPECT_RATIO);
    frameRate(tms80->refresh_rate);

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

u8 tms80_get_keypad_a(tms80_t* tms80){
    if(tms80->force_paddle_controller){
        u8 x = 0xF0;
        if(!tms80->paddle_status)
            x &= ~(1 << 5);
        x |= !tms80->paddle_status ? (mouseX & 0xF) : (mouseX >> 4);
        SDL_MouseButtonFlags ms = SDL_GetMouseState(NULL, NULL);
        if(ms & SDL_BUTTON_LMASK)
            x &= ~(1 << 4);
        tms80->paddle_status ^= 1;
        return x;
    }

    u8 row = tms80->keypad_reg & 0b111;
    u8 out = 0xFF;

    for(int i = 0; i < 8; i++)
        if(controls_pressed(keypad_row_a[row][i]))
            out &= ~(1 << i);

    return out;
}

u8 tms80_get_keypad_b(tms80_t* tms80){
    u8 row = tms80->keypad_reg & 0b111;
    u8 out = 0xFF;

    for(int i = 0; i < 4; i++)
        if(controls_pressed(keypad_row_b[row][i]))
            out &= ~(1 << i);

    return out;
}

u8 tms80_gg_get_start_button(){
    if(controls_pressed(CONTROL_TMS80_GG_START))
        return 0x40;

    return 0xC0;
}

void TMS80_run_frame(tms80_t* tms80){
    z80_t* z80 = &tms80->z80;
    vdp_t* vdp = &tms80->vdp;
    sn76489_t* apu = &tms80->apu;

    if(tms80->type != GG && controls_released(CONTROL_TMS80_PAUSE))
        z80_nmi(z80);

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

bool TMS80_detect(const archive_t* rom_archive, const archive_t* bios_archive){
    return (detect_type(rom_archive) != TMS80_UNKNOWN) || (detect_type(bios_archive) != TMS80_UNKNOWN);
}

void TMS80_close(tms80_t* tms80, const char* sav_path){
    if(tms80->no_cartridge)
        free(tms80->cartridge);
}

byte_vec_t TMS80_savestate(tms80_t* tms80){
    byte_vec_t state;
    byte_vec_init(&state);
    serialize_tms80_t(tms80, &state);
    byte_vec_shrink(&state);
    return state;
}

void TMS80_loadstate(tms80_t* tms80, byte_vec_t* state){
    deserialize_tms80_t(tms80, state->data);
    if(tms80->bios_masked){
        tms80->z80.readMemory = tms80_sms_readMemory;
        tms80->z80.writeMemory = tms80_sms_writeMemory;
    } else {
        tms80->z80.readMemory = tms80_sms_bios_readMemory;
        tms80->z80.writeMemory = tms80_sms_bios_writeMemory;
    }
}