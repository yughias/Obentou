#include "cores/chip8/chip8.h"

#include "utils/archive.h"
#include "utils/controls.h"
#include "utils/sound.h"
#include "SDL_MAINLOOP.h"

#include <time.h>

#define pushStack(val) (ch8->stack[++ch8->stackIndex] = val)
#define popStack() (ch8->stack[ch8->stackIndex--])

void* CHIP8_init(const archive_t* rom_archive, const archive_t* bios_archive) {
    ch8_t* ch8 = malloc(sizeof(ch8_t));
    memset(ch8, 0, sizeof(ch8_t));
    srand(time(NULL));

    ch8->PC = 0x200;
    ch8->I = 0;
    ch8->ST = 0;
    ch8->DT = 0;
    ch8->stackIndex = -1;

    const u8 font[] = {
        0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
        0x20, 0x60, 0x20, 0x20, 0x70, // 1
        0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
        0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
        0x90, 0x90, 0xF0, 0x10, 0x10, // 4
        0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
        0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
        0xF0, 0x10, 0x20, 0x40, 0x40, // 7
        0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
        0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
        0xF0, 0x90, 0xF0, 0x90, 0x90, // A
        0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
        0xF0, 0x80, 0x80, 0x80, 0xF0, // C
        0xE0, 0x90, 0x90, 0x90, 0xE0, // D
        0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
        0xF0, 0x80, 0xF0, 0x80, 0x80  // F
    };

    memcpy(ch8->memory+0x50, font, sizeof(font));

    file_t* rom = archive_get_file_by_ext(rom_archive, "ch8");
    if(!rom)
        rom = archive_get_file_by_ext(rom_archive, "chip8");
    memcpy(ch8->memory + 0x200, rom->data, rom->size);

    return ch8;
}

static void tick_timers(ch8_t* ch){
    if(ch->ST != 0) ch->ST--;
    if(ch->DT != 0) ch->DT--;
}

static void run_opcode(ch8_t* ch8) {
    u16 opcode = (ch8->memory[ch8->PC] << 8) | ch8->memory[ch8->PC+1];
    u8 first = (opcode & 0xF000) >> 12;
    u8 second = (opcode & 0xF00) >> 8;
    u8 third = (opcode & 0xF0) >> 4;
    u8 fourth = opcode & 0xF;
    ch8->PC += 2;
    
    if(first == 0x0){
        if(second == 0x0){
            if(third == 0xE){
                if(fourth == 0x0){
                    //00E0
                    //CLS
                    memset(ch8->display, 0, sizeof(ch8->display));
                }
                if(fourth == 0xE){
                    //00EE
                    //RET
                    ch8->PC = popStack();
                }
            }
        }
    }
    if(first == 0x1){
        //1NNN
        //JUMP
        ch8->PC = opcode & 0xFFF;
    }
    if(first == 0x2){
        //2NNN
        //CALL addr
        pushStack(ch8->PC);
        ch8->PC = opcode & 0xFFF;
    }
    if(first == 0x3){
        //3XNN
        //SE Vx, byte
        if(ch8->V[second] == (opcode & 0xFF)) ch8->PC += 2;
    }
    if(first == 0x4){
        //4XNN
        //SNE Vx, byte
        if(ch8->V[second] != (opcode & 0xFF)) ch8->PC += 2;
    }
    if(first == 0x5){
        if(fourth == 0x0){
            //5XY0
            //SE Vx, Vy
            if(ch8->V[second] == ch8->V[third]) ch8->PC += 2;
        }
    }
    if(first == 0x6){
        //6XNN
        ch8->V[second] = opcode & 0xFF;
    }
    if(first == 0x7){
        //7XNN
        ch8->V[second] += (opcode & 0xFF);
    }
    if(first == 0x8){
        if(fourth == 0x0){
            //8XY0
            ch8->V[second] = ch8->V[third];
        }
        if(fourth == 0x1){
            //8XY1
            ch8->V[second] |= ch8->V[third];
        }
        if(fourth == 0x2){
            //8XY2
            ch8->V[second] &= ch8->V[third];
        }
        if(fourth == 0x3){
            //8XY3
            ch8->V[second] ^= ch8->V[third];
        }
        if(fourth == 0x4){
            //8XY4
            bool flag = false;
            if((ch8->V[second])+(ch8->V[third]) > 255)
                flag = true;
            ch8->V[second] += ch8->V[third];
            ch8->V[0xF] = flag;
        }
        if(fourth == 0x5){
            //8XY5
            bool flag = false;
            if(ch8->V[second] > ch8->V[third])
                flag = true;
            ch8->V[second] -= ch8->V[third];
            ch8->V[0xF] = flag;
        }
        if(fourth == 0x6){
            //8XY6
            bool flag = false;
            if(ch8->V[second] & 0x01)
                flag = true;
            ch8->V[second] = ch8->V[second] >> 1;
            ch8->V[0xF] = flag;
        }
        if(fourth == 0x7){
            //8XY7
            bool flag = false;
            if(ch8->V[third] > ch8->V[second])
                flag = true;
            ch8->V[second] = ch8->V[third] - ch8->V[second];
            ch8->V[0xF] = flag;
        }
        if(fourth == 0xE){
            //8XYE
            bool flag = false;
            if(ch8->V[second] & 0x80)
                flag = true;
            ch8->V[second] = ch8->V[second] << 1;
            ch8->V[0xF] = flag;
        }
    }
    if(first == 0x9){
        if(fourth == 0x0){
            //9XY0
            //SNE Vx, Vy
            if(ch8->V[second] != ch8->V[third]) ch8->PC += 2;
        }
    }
    if(first == 0xA){
        //ANN
        ch8->I = opcode & 0xFFF;
    }
    if(first == 0xB){
        //BNN
        ch8->PC = (opcode & 0xFFF) + ch8->V[0x0];
    }
    if(first == 0xC){
        ch8->V[second] = (rand()%256) & (opcode & 0xFF);
    }
    if(first == 0xD){
        //DXYN
        int vx = ch8->V[second] % CH8_W;
        int vy = ch8->V[third] % CH8_H;
        ch8->V[0xF] = 0;
        for(int y = 0; y < fourth; y++){
            int row = ch8->memory[ch8->I+y];
            for(int x = 0; x < 8; x++){
                int offset = vx+x+(vy+y)*CH8_W;
                if(vy+y >= CH8_H || vx+x >= CH8_W) break;
                bool value = (row & (1 << (7-x))) >> (7-x);  
                if(ch8->display[offset] && value)
                    ch8->V[0xF] = 1;
                ch8->display[offset] ^= value;
            }
        }
    }
    if(first == 0xE){
        if(third == 0x9){
            if(fourth == 0xE){
                //EX95
                if(ch8->keys[ch8->V[second]]) ch8->PC += 2;
            }
        }
        if(third == 0xA){
            if(fourth == 0x1){
                //EXA1
                if(!ch8->keys[ch8->V[second]]) ch8->PC += 2;
            }
        }
    }
    if(first == 0xF){
        if(third == 0x0){
            if(fourth == 0x7){
                //FX07
                ch8->V[second] = ch8->DT;
            }
            if(fourth == 0xA){
                //FX0A
                bool detectKeyPress = false;
                for(int i = 0; i < 16; i++)
                    if(!ch8->preKeys[i] && ch8->keys[i]){
                        ch8->V[second] = i;
                        detectKeyPress = true;
                        break;
                    }
                if(!detectKeyPress)
                    ch8->PC -= 2;
            }
        }
        if(third == 0x1){
            if(fourth == 0x5){
                //FX15
                ch8->DT = ch8->V[second];
            }
            if(fourth == 0x8){
                ch8->ST = ch8->V[second];
            }
            if(fourth == 0xE){
                //FX1E
                ch8->I += ch8->V[second];
            }
        }
        if(third == 0x2){
            if(fourth == 0x9){
                //FX29
                ch8->I = ch8->V[second]*5 + 0x50;
            }
        }
        if(third == 0x3){
            if(fourth == 0x3){
                //FX33
                ch8->memory[ch8->I] = ( ch8->V[second] / 100);
                ch8->memory[ch8->I+1] = ( (ch8->V[second] / 10) % 10);
                ch8->memory[ch8->I+2] = ( ch8->V[second] % 10);
            }
        }
        if(third == 0x5){
            if(fourth == 0x5){
                //FX55
                for(int i = 0; i <= second; i++)
                    ch8->memory[ch8->I+i] = ch8->V[i];
            }
        }
        if(third == 0x6){
            if(fourth == 0x5){
                //FX65
                for(int i = 0; i <= second; i++)
                    ch8->V[i] = ch8->memory[ch8->I+i];
            }
        }
    }
}

static void update_keys(ch8_t* ch){
    memcpy(ch->preKeys, ch->keys, sizeof(ch->keys));
    for(int i = 0; i < 16; i++){
        ch->keys[i] = controls_pressed(CONTROL_CHIP8_0+i, 0);
    }
}

static void get_buzzer(void* ctx, void* sample) {
    ch8_t* ch = (ch8_t*)ctx;
    *(u8*)sample = sound_set_channel_sample(ch->buzzer, 0) * 0x7F;
}

void CHIP8_run_frame(ch8_t* ch) {
    update_keys(ch);
    for(int i = 0; i < CYCLES_PER_FRAME; i++) {
        run_opcode(ch);

        if (ch->ST)
            ch->buzzer ^= 1;
        else
            ch->buzzer = 0;

        sound_push_sample(1, 1, ch, get_buzzer);
    }
    tick_timers(ch);

    for(int i = 0; i < CH8_W*CH8_H; i++)
        pixels[i] = ch->display[i] ? COLOR_FG : COLOR_BG;
    renderPixels();
}

bool CHIP8_detect(const archive_t* rom_archive, const archive_t* bios_archive) {
    if(archive_get_file_by_ext(rom_archive, "ch8"))
        return true;
    if(archive_get_file_by_ext(rom_archive, "chip8"))
        return true;
    return false;
}

byte_vec_t CHIP8_savestate(ch8_t* ch8) {
    byte_vec_t state;
    byte_vec_init(&state);
    serialize_ch8_t(ch8, &state);
    byte_vec_shrink(&state);
    return state;
}

bool CHIP8_loadstate(ch8_t* ctx, byte_vec_t* state) {
    const u8* end = state->data + state->size;
    return deserialize_ch8_t(ctx, state->data, state->data + state->size) == end;
}