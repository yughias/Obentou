#include "cores/pacman/pacman.h"
#include "cores/pacman/memory.h"
#include "cores/pacman/video.h"
#include "cores/pacman/audio.h"
#include "cpus/z80.h"
#include "utils/archive.h"
#include "utils/controls.h"
#include "SDL_MAINLOOP.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void pacman_update_input(pacman_t* p)
{
    p->IN0 = 0xFF;
    p->IN1 = 0xFF;
    for (int i = 0; i < 8; i++) {
        p->IN0 &= ~(u8)(controls_pressed(p->input_map[i], CONTROLS_BOTH) << i);
        p->IN1 &= ~(u8)(controls_pressed(p->input_map[i+8], CONTROLS_BOTH) << i);
    }
}

static u16 decrypt_addr1(u16 v)
{
    u16 o  = v & 0x807;
    o |= (v & 0x400) >> 7;
    o |= (v & 0x200) >> 2;
    o |= (v & 0x080) << 3;
    o |= (v & 0x040) << 2;
    o |= (v & 0x138) << 1;
    return o;
}

static u16 decrypt_addr2(u16 v)
{
    u16 o  = v & 0x807;
    o |= (v & 0x040) << 4;
    o |= (v & 0x100) >> 3;
    o |= (v & 0x080) << 2;
    o |= (v & 0x600) >> 2;
    o |= (v & 0x028) << 1;
    o |= (v & 0x010) >> 1;
    return o;
}

static u8 decrypt_byte(u8 v)
{
    u8 o  = (v & 0xC0) >> 3;
    o |= (v & 0x10) << 2;
    o |= (v & 0x0E) >> 1;
    o |= (v & 0x01) << 7;
    o |= (v & 0x20);
    return o;
}

static void init_aux_board_mspacman(pacman_t* p, const archive_t* archive)
{
    file_t* fu6 = archive_get_file_by_name(archive, "u6");
    file_t* fu7 = archive_get_file_by_name(archive, "u7");
    file_t* fu5 = archive_get_file_by_name(archive, "u5");

    if (!fu6 || fu6->size < 0x1000 ||
        !fu7 || fu7->size < 0x1000 ||
        !fu5 || fu5->size < 0x0800) {
        printf("PACMAN: Ms Pac-Man auxiliary ROMs not found, disabling aux board\n");
        return;
    }

    p->AUX_ROM   = calloc(1, PACMAN_ROM_SIZE);
    p->ROM_HIGH  = calloc(1, 0x1800);

    if (!p->AUX_ROM || !p->ROM_HIGH) {
        free(p->AUX_ROM);
        free(p->ROM_HIGH);
        p->AUX_ROM = p->ROM_HIGH = NULL;
        return;
    }

    memcpy(p->AUX_ROM, p->ROM, 0x3000);

    for (u16 i = 0; i < 0x1000; i++) {
        p->AUX_ROM [0x3000 + decrypt_addr1(i)] = decrypt_byte(fu7->data[i]);
        p->ROM_HIGH[0x0800 + decrypt_addr1(i)] = decrypt_byte(fu6->data[i]);
    }
    for (u16 i = 0; i < 0x0800; i++)
        p->ROM_HIGH[decrypt_addr2(i)] = decrypt_byte(fu5->data[i]);

    /* Patch jump table as per MAME reference */
    static const u16 patches[][2] = {
        {0x0410,0x0008},{0x08E0,0x01D8},{0x0A30,0x0118},{0x0BD0,0x00D8},
        {0x0C20,0x0120},{0x0E58,0x0168},{0x0EA8,0x0198},
        {0x1000,0x0020},{0x1008,0x0010},{0x1288,0x0098},{0x1348,0x0048},
        {0x1688,0x0088},{0x16B0,0x0188},{0x16D8,0x00C8},{0x16F8,0x01C8},
        {0x19A8,0x00A8},{0x19B8,0x01A8},
        {0x2060,0x0148},{0x2108,0x0018},{0x21A0,0x01A0},{0x2298,0x00A0},
        {0x23E0,0x00E8},{0x2418,0x0000},{0x2448,0x0058},{0x2470,0x0140},
        {0x2488,0x0080},{0x24B0,0x0180},{0x24D8,0x00C0},{0x24F8,0x01C0},
        {0x2748,0x0050},{0x2780,0x0090},{0x27B8,0x0190},{0x2800,0x0028},
        {0x2B20,0x0100},{0x2B30,0x0110},{0x2BF0,0x01D0},{0x2CC0,0x00D0},
        {0x2CD8,0x00E0},{0x2CF0,0x01E0},{0x2D60,0x0160},
    };
    for (int k = 0; k < (int)(sizeof(patches)/sizeof(patches[0])); k++)
        for (int b = 0; b < 8; b++)
            p->AUX_ROM[patches[k][0] + b] = p->ROM_HIGH[patches[k][1] + b];

    u8* buf = malloc(0x1800);
    memcpy(buf + 0x0000, p->ROM_HIGH + 0x0000, 0x800); // fu5 [0x0000..0x07FF]
    memcpy(buf + 0x0800, p->ROM_HIGH + 0x1000, 0x800); // fu6 [0x0800..0x0FFF]
    memcpy(buf + 0x1000, p->ROM_HIGH + 0x0800, 0x800); // fu6 [0x1000..0x17FF]
    free(p->ROM_HIGH);
    p->ROM_HIGH = buf;
}

static bool load_rom(const archive_t* archive, const char* name,
                     u8* dst, size_t size)
{
    file_t* f = archive_get_file_by_name(archive, name);
    if (!f || f->size < size) {
        printf("PACMAN: ROM not found or too small: %s\n", name);
        return false;
    }
    memcpy(dst, f->data, size);
    return true;
}

static u16 swap_word(u16 word, const u8* index)
{
    u16 out = 0;
    for (int i = 0; i < 16; i++) {
        bool bit = (word >> index[i]) & 1;
        out |= (u16)(bit << (15 - i));
    }
    return out;
}

static u8 swap_byte(u8 byte, const u8* index)
{
    u8 out = 0;
    for (int i = 0; i < 8; i++) {
        bool bit = (byte >> index[i]) & 1;
        out |= (u8)(bit << (7 - i));
    }
    return out;
}

// decoding code at MAME source code: 
// https://github.com/mamedev/mame/blob/master/src/mame/pacman/pacplus.cpp
static u8 decrypt_pacplus_addr(u16 addr, u8 byte)
{
    static const u8 swap_xor_table[6][9] = {
        { 7,6,5,4,3,2,1,0, 0x00 },
        { 7,6,5,4,3,2,1,0, 0x28 },
        { 6,1,3,2,5,7,0,4, 0x96 },
        { 6,1,5,2,3,7,0,4, 0xbe },
        { 0,3,7,6,4,2,1,5, 0xd5 },
        { 0,3,4,6,7,2,1,5, 0xdd }
    };
    static const int picktable[32] = {
        0,2,4,2,4,0,4,2,2,0,2,2,4,0,4,2,
        2,2,4,0,4,2,4,0,0,4,0,4,4,2,4,2
    };

	/* pick method from bits 0 2 5 7 9 of the address */
    u32 method = (u32)picktable[
        (addr & 0x001) |
        ((addr & 0x004) >> 1) |
        ((addr & 0x020) >> 3) |
        ((addr & 0x080) >> 4) |
        ((addr & 0x200) >> 5)];

	/* switch method if bit 11 of the address is set */
    if (addr & 0x800)
        method ^= 1;

    const u8* tbl = swap_xor_table[method];
    return swap_byte(byte, tbl) ^ tbl[8];
}

static void decrypt_pacplus_system(pacman_t* p) {
    for (u16 i = 0; i < PACMAN_ROM_SIZE; i++)
            p->ROM[i] = decrypt_pacplus_addr(i, p->ROM[i]);
}

static void eyes_decode_gfx(u8* data) {
    u8 swapbuffer[8];

	for (int i = 0; i < 8; i++) {
        const u8 reorder[16] = { 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 0, 1 ,2 };
		swapbuffer[i] = data[swap_word(i, reorder)];
    }

	for (int i = 0; i < 8; i++) {
        const u8 reorder[8] = { 7, 4, 5, 6, 3, 2, 1, 0 };
		data[i] = swap_byte(swapbuffer[i], reorder);
    }
}

static void eyes_decrypt_system(pacman_t* p) {
	/* Data lines D3 and D5 swapped */
	for (int i = 0; i < PACMAN_ROM_SIZE; i++) {
		const u8 reorder[8] = { 7, 6, 3, 4, 5, 2, 1, 0 };
        p->ROM[i] = swap_byte(p->ROM[i], reorder);
    }

	for (int i = 0; i < 4096; i += 8) {
		eyes_decode_gfx(p->tileROM + i);
        eyes_decode_gfx(p->spriteROM + i);
    }
}

typedef struct {
    const char*  name;
    const char*  rom_6e;        /* program ROM banks */
    const char*  rom_6f;
    const char*  rom_6h;
    const char*  rom_6j;
    const char*  color_rom;     /* colour PROM (32 bytes) */
    const char*  palette_rom;   /* palette PROM (256 bytes) */
    const char*  tile_rom;      /* tile gfx ROM (4096 bytes) */
    const char*  sprite_rom;    /* sprite gfx ROM (4096 bytes) */
    const char*  audio_rom[2];     /* audio ROM (4096 bytes) */
    PACMAN_TYPE  type;             /* arcade hardware type */
    void         (*decrypt) (pacman_t* p); /* decryption function */ 
    bool         is_270_degree;
    u8           default_dip_switch; /* 1C_C, 2 Lives, Lowest Bonus, Normal Difficulty, Cabinet */
    control_t    input_map[16]; /* bit map for IN0 and IN1 */
} pacman_romset_t;

#define PACMAN_INPUT_MAP { \
    CONTROL_PACMAN_UP, \
    CONTROL_PACMAN_LEFT, \
    CONTROL_PACMAN_RIGHT, \
    CONTROL_PACMAN_DOWN, \
    CONTROL_NONE, \
    CONTROL_NONE, \
    CONTROL_NONE, \
    CONTROL_PACMAN_COIN, \
\
    CONTROL_PACMAN_UP, \
    CONTROL_PACMAN_LEFT, \
    CONTROL_PACMAN_RIGHT, \
    CONTROL_PACMAN_DOWN, \
    CONTROL_NONE, \
    CONTROL_PACMAN_START1, \
    CONTROL_PACMAN_START2, \
    CONTROL_NONE \
}

#define KOROSUKE_INPUT_MAP { \
    CONTROL_PACMAN_UP, \
    CONTROL_PACMAN_LEFT, \
    CONTROL_PACMAN_RIGHT, \
    CONTROL_PACMAN_DOWN, \
    CONTROL_ALWAYS, \
    CONTROL_PACMAN_BUTTON, \
    CONTROL_PACMAN_COIN, \
    CONTROL_PACMAN_BUTTON, \
\
    CONTROL_PACMAN_UP, \
    CONTROL_PACMAN_LEFT, \
    CONTROL_PACMAN_RIGHT, \
    CONTROL_PACMAN_DOWN, \
    CONTROL_NONE, \
    CONTROL_PACMAN_START1, \
    CONTROL_PACMAN_START2, \
    CONTROL_NONE \
}

#define EYES_MRTNT_INPUT_MAP { \
    CONTROL_PACMAN_UP, \
    CONTROL_PACMAN_LEFT, \
    CONTROL_PACMAN_RIGHT, \
    CONTROL_PACMAN_DOWN, \
    CONTROL_NONE, \
    CONTROL_PACMAN_COIN, \
    CONTROL_NONE, \
    CONTROL_NONE, \
\
    CONTROL_PACMAN_UP, \
    CONTROL_PACMAN_LEFT, \
    CONTROL_PACMAN_RIGHT, \
    CONTROL_PACMAN_DOWN, \
    CONTROL_PACMAN_BUTTON, \
    CONTROL_PACMAN_START1, \
    CONTROL_PACMAN_START2, \
    CONTROL_PACMAN_BUTTON \
}

static const pacman_romset_t romsets[] = {
    {
        "pacman plus",
        "pacplus.6e", "pacplus.6f", "pacplus.6h", "pacplus.6j",
        "pacplus.7f", "pacplus.4a", "pacplus.5e", "pacplus.5f",
        { "82s126.1m", "82s126.3m" },
        PACMAN_TYPE_NORMAL, decrypt_pacplus_system, false,
        0x1 | 0x08 | 0x00 | 0x40 | 0x80,
        PACMAN_INPUT_MAP
    },
    {
        "pacman mod",
        "pacmanh.6e", "pacman.6f", "pacmanh.6h", "pacmanh.6j",
        "82s123.7f", "82s126.4a", "pacmanh.5e", "pacman.5f",
        { "82s126.1m", "82s126.3m" },
        PACMAN_TYPE_NORMAL, NULL, false,
        0x1 | 0x08 | 0x00 | 0x40 | 0x80,
        PACMAN_INPUT_MAP
    },
    {
        "ms pacman fast",
        "pacman.6e", "pacfast.6f", "pacman.6h", "pacman.6j",
        "82s123.7f", "82s126.4a", "5e", "5f",
        { "82s126.1m", "82s126.3m" },
        PACMAN_TYPE_MSPACMAN, NULL, false,
        0x01 | 0x08 | 0x00 | 0x40,
        PACMAN_INPUT_MAP
    },
    {
        "ms pacman",
        "pacman.6e", "pacman.6f", "pacman.6h", "pacman.6j",
        "82s123.7f", "82s126.4a", "5e", "5f",
        { "82s126.1m", "82s126.3m" },
        PACMAN_TYPE_MSPACMAN, NULL, false,
        0x01 | 0x08 | 0x00 | 0x40,
        PACMAN_INPUT_MAP
    },
    {
        "pacman fast",
        "pacman.6e", "pacfast.6f", "pacman.6h", "pacman.6j",
        "82s123.7f", "82s126.4a", "pacman.5e", "pacman.5f",
        { "82s126.1m", "82s126.3m" },
        PACMAN_TYPE_NORMAL, NULL, false,
        0x1 | 0x08 | 0x00 | 0x40 | 0x80,
        PACMAN_INPUT_MAP
    },
    {
        "pacman",
        "pacman.6e", "pacman.6f", "pacman.6h", "pacman.6j",
        "82s123.7f", "82s126.4a", "pacman.5e", "pacman.5f",
        { "82s126.1m", "82s126.3m" },
        PACMAN_TYPE_NORMAL, NULL, false,
        0x1 | 0x08 | 0x00 | 0x40 | 0x80,
        PACMAN_INPUT_MAP
    },
    {
        "maketrax",
        "maketrax.6e", "maketrax.6f", "maketrax.6h", "maketrax.6j",
        "82s123.7f", "2s140.4a", "maketrax.5e", "maketrax.5f",
        { "82s126.1m", "82s126.3m" },
        PACMAN_TYPE_MAKETRAX, NULL, true,
        0x01 | 0x00 | 0x10 | 0x00,
        PACMAN_INPUT_MAP
    },
    {
        "korosuke",
        "kr.6e", "kr.6f", "kr.6h", "kr.6j",
        "82s123.7f", "2s140.4a", "kr.5e", "kr.5f",
        { "82s126.1m", "82s126.3m" },
        PACMAN_TYPE_MAKETRAX, NULL, false,
        0x01 | 0x00 | 0x10 | 0x00,
        KOROSUKE_INPUT_MAP
    },
    {
        "painter",
        "pain1.6e", "pain2.6f", "pain3.6h", "pain4-pennello2.6j",
        "mb7051.7f", "n82s129n.4a", "pain5.5e", "pain6.5f",
        { "mb7052.1m", "mb7052.3m" },
        PACMAN_TYPE_MAKETRAX, NULL, false,
        0x01 | 0x00 | 0x10 | 0x00,
        PACMAN_INPUT_MAP
    },
    {
        "crush",
        "crushkrl.6e", "crushkrl.6f", "crushkrl.6h", "crushkrl.6j",
        "82s123.7f", "2s140.4a", "maketrax.5e", "maketrax.5f",
        { "82s126.1m", "82s126.3m" },
        PACMAN_TYPE_MAKETRAX, NULL, false,
        0x01 | 0x00 | 0x10 | 0x00,
        PACMAN_INPUT_MAP
    },
    {
        "eyes",
        "d7", "e7", "f7", "h7",
        "82s123.7f", "82s129.4a", "d5", "e5",
        { "82s126.1m", "82s126.3m" },
        PACMAN_TYPE_NORMAL, eyes_decrypt_system, false,
        0x03 | 0x08 | 0x30 | 0x40,
        EYES_MRTNT_INPUT_MAP
    },
    {
        "mrtnt",
        "tnt.1", "tnt.2", "tnt.3", "tnt.4",
        "82s123.7f", "82s126.4a", "tnt.5", "tnt.6",
        { "82s126.1m", "82s126.3m" },
        PACMAN_TYPE_NORMAL, eyes_decrypt_system, false,
        0x03 | 0x08 | 0x30 | 0x40,
        EYES_MRTNT_INPUT_MAP
    }
};

#define N_ROMSETS ((int)(sizeof(romsets) / sizeof(romsets[0])))

static bool rom_exists(const archive_t* archive, const char* name)
{
    return name && archive_get_file_by_name(archive, name) != NULL;
}

static const pacman_romset_t* find_romset(const archive_t* archive)
{
    for (int i = 0; i < N_ROMSETS; i++) {
        const pacman_romset_t* rs = &romsets[i];

        /* Check all required ROMs */
        if (!rom_exists(archive, rs->rom_6e)) continue;
        if (!rom_exists(archive, rs->rom_6f)) continue;
        if (!rom_exists(archive, rs->rom_6h)) continue;
        if (!rom_exists(archive, rs->rom_6j)) continue;
        if (!rom_exists(archive, rs->color_rom)) continue;
        if (!rom_exists(archive, rs->palette_rom)) continue;
        if (!rom_exists(archive, rs->tile_rom)) continue;
        if (!rom_exists(archive, rs->sprite_rom)) continue;

        if (rs->type == PACMAN_TYPE_MSPACMAN) {
            if (!rom_exists(archive, "u6") || !rom_exists(archive, "u5"))
                continue;
        }

        return rs;
    }

    return NULL;
}

void PACMAN_free(pacman_t* p)
{
    free(p->ROM);
    free(p->colorROM);
    free(p->paletteROM);
    free(p->tileROM);
    free(p->spriteROM);
    free(p->audioROM);
    free(p->AUX_ROM);
    free(p->ROM_HIGH);
}

static bool find_jrpacman(const archive_t* archive) {
    // ROM
    if (!rom_exists(archive, "jrp8d.8d")) return false;
    if (!rom_exists(archive, "jrp8e.8e")) return false;
    if (!rom_exists(archive, "jrp8h.8h")) return false;
    if (!rom_exists(archive, "jrp8j.8j")) return false;
    if (!rom_exists(archive, "jrp8k.8k")) return false;
    
    // GFX
    if (!rom_exists(archive, "jrp2c.2c")) return false;
    if (!rom_exists(archive, "jrp2e.2e")) return false;
    
    // PALETTE
    if (!rom_exists(archive, "a290-27axv-bxhd.9e")) return false;
    if (!rom_exists(archive, "a290-27axv-cxhd.9f")) return false;

    // COLOR
    if (!rom_exists(archive, "a290-27axv-axhd.9p")) return false;

    
    // WSG
    if (!rom_exists(archive, "a290-27axv-dxhd.7p")) return false;
    if (!rom_exists(archive, "a290-27axv-exhd.5s")) return false;

    return true;
}

static void decrypt_jrpacman(pacman_t* p) {
    // taken from: https://github.com/mamedev/mame/blob/4d4da6bcd6d4537849893fd8bdd675f9ad5f166c/src/mame/pacman/jrpacman.cpp#L367
	static const struct {
		int count;
		int value;
	} table[] =
	{
		{ 0x00C1, 0x00 },{ 0x0002, 0x80 },{ 0x0004, 0x00 },{ 0x0006, 0x80 },
		{ 0x0003, 0x00 },{ 0x0002, 0x80 },{ 0x0009, 0x00 },{ 0x0004, 0x80 },
		{ 0x9968, 0x00 },{ 0x0001, 0x80 },{ 0x0002, 0x00 },{ 0x0001, 0x80 },
		{ 0x0009, 0x00 },{ 0x0002, 0x80 },{ 0x0009, 0x00 },{ 0x0001, 0x80 },
		{ 0x00AF, 0x00 },{ 0x000E, 0x04 },{ 0x0002, 0x00 },{ 0x0004, 0x04 },
		{ 0x001E, 0x00 },{ 0x0001, 0x80 },{ 0x0002, 0x00 },{ 0x0001, 0x80 },
		{ 0x0002, 0x00 },{ 0x0002, 0x80 },{ 0x0009, 0x00 },{ 0x0002, 0x80 },
		{ 0x0009, 0x00 },{ 0x0002, 0x80 },{ 0x0083, 0x00 },{ 0x0001, 0x04 },
		{ 0x0001, 0x01 },{ 0x0001, 0x00 },{ 0x0002, 0x05 },{ 0x0001, 0x00 },
		{ 0x0003, 0x04 },{ 0x0003, 0x01 },{ 0x0002, 0x00 },{ 0x0001, 0x04 },
		{ 0x0003, 0x01 },{ 0x0003, 0x00 },{ 0x0003, 0x04 },{ 0x0001, 0x01 },
		{ 0x002E, 0x00 },{ 0x0078, 0x01 },{ 0x0001, 0x04 },{ 0x0001, 0x05 },
		{ 0x0001, 0x00 },{ 0x0001, 0x01 },{ 0x0001, 0x04 },{ 0x0002, 0x00 },
		{ 0x0001, 0x01 },{ 0x0001, 0x04 },{ 0x0002, 0x00 },{ 0x0001, 0x01 },
		{ 0x0001, 0x04 },{ 0x0002, 0x00 },{ 0x0001, 0x01 },{ 0x0001, 0x04 },
		{ 0x0001, 0x05 },{ 0x0001, 0x00 },{ 0x0001, 0x01 },{ 0x0001, 0x04 },
		{ 0x0002, 0x00 },{ 0x0001, 0x01 },{ 0x0001, 0x04 },{ 0x0002, 0x00 },
		{ 0x0001, 0x01 },{ 0x0001, 0x04 },{ 0x0001, 0x05 },{ 0x0001, 0x00 },
		{ 0x01B0, 0x01 },{ 0x0001, 0x00 },{ 0x0002, 0x01 },{ 0x00AD, 0x00 },
		{ 0x0031, 0x01 },{ 0x005C, 0x00 },{ 0x0005, 0x01 },{ 0x604E, 0x00 },
		{ 0,0 }
	};

	for (int i = 0, A = 0; table[i].count; i++) {
		for (int j = 0; j < table[i].count; j++) {
            if (A < PACMAN_ROM_SIZE)
			    p->ROM[A] ^= table[i].value;
            if (A > 0x8000 && A < 0xE000)
                p->ROM_HIGH[A - 0x8000] ^= table[i].value;
            A += 1;
        }
    }

    printf("JRPAMCAN DECRYPTED\n");
}

static void init_jrpacman(pacman_t* p, const archive_t* rom_archive) {
    p->type = PACMAN_TYPE_JRPACMAN;
    p->ROM      =  calloc(1, PACMAN_ROM_SIZE);
    p->colorROM   = calloc(1, 32);
    p->paletteROM = calloc(1, 256); 
    p->tileROM    = calloc(1, 4096 * 2);
    p->spriteROM  = calloc(1, 4096 * 2);
    p->audioROM   = calloc(1, PACMAN_AUDIO_ROM_SIZE);
    p->ROM_HIGH = calloc(1, 0x2000 * 3);

    static const control_t jrpacman_input_map[] = {
        CONTROL_PACMAN_UP,
        CONTROL_PACMAN_LEFT,
        CONTROL_PACMAN_RIGHT,
        CONTROL_PACMAN_DOWN,
        CONTROL_NONE,
        CONTROL_PACMAN_COIN,
        CONTROL_NONE,
        CONTROL_NONE,

        CONTROL_PACMAN_UP,
        CONTROL_PACMAN_LEFT,
        CONTROL_PACMAN_RIGHT,
        CONTROL_PACMAN_DOWN,
        CONTROL_NONE,
        CONTROL_PACMAN_START1,
        CONTROL_PACMAN_START2,
        CONTROL_NONE
    };

    static const u8 jrpacman_default_dip_switch = 0x1 | 0x08 | 0x00 | 0x40 | 0x80;

    load_rom(rom_archive, "jrp8d.8d", p->ROM,          0x2000);
    load_rom(rom_archive, "jrp8e.8e", p->ROM + 0x2000, 0x2000);

    load_rom(rom_archive, "jrp8h.8h", p->ROM_HIGH, 0x2000);
    load_rom(rom_archive, "jrp8j.8j", p->ROM_HIGH + 0x2000, 0x2000);
    load_rom(rom_archive, "jrp8k.8k", p->ROM_HIGH + 0x4000, 0x2000);


    load_rom(rom_archive, "a290-27axv-axhd.9p",   p->paletteROM, 256);

    u8 low_color_nibbles[256];
    u8 high_color_nibbles[256];
    load_rom(rom_archive, "a290-27axv-bxhd.9e", low_color_nibbles, 256);
    load_rom(rom_archive, "a290-27axv-cxhd.9f", high_color_nibbles, 256);

    for (u16 i = 0; i < 32; i++) {
        p->colorROM[i] = low_color_nibbles[i] | (high_color_nibbles[i] << 4);
    }
    
    load_rom(rom_archive, "jrp2c.2c",  p->tileROM,   4096 * 2);
    load_rom(rom_archive, "jrp2e.2e",  p->spriteROM, 4096 * 2);

    load_rom(rom_archive, "a290-27axv-dxhd.7p", p->audioROM,       256);
    load_rom(rom_archive, "a290-27axv-exhd.5s", p->audioROM + 256, 256);

    p->input_map = jrpacman_input_map;
    p->DIP_SWITCH_SETTINGS = jrpacman_default_dip_switch;

    decrypt_jrpacman(p);
}

bool PACMAN_detect(const archive_t* rom_archive, const archive_t* bios_archive)
{
    if(find_jrpacman(rom_archive))
        return true;

    return find_romset(rom_archive) != NULL;
}

void* PACMAN_init(const archive_t* rom_archive, const archive_t* bios_archive)
{
    pacman_t* p = calloc(1, sizeof(pacman_t));
    if (!p) return NULL;

    z80_init(&p->z80);
    p->z80.ctx         = p;
    p->z80.readMemory  = pacman_read_memory;
    p->z80.writeMemory = pacman_write_memory;
    p->z80.readIO      = pacman_read_io;
    p->z80.writeIO     = pacman_write_io;

    if (find_jrpacman(rom_archive)) {
        init_jrpacman(p, rom_archive);
        printf("PACMAN: JRPACMAN\n");
        return p;
    }

    const pacman_romset_t* rs = find_romset(rom_archive);
    if (!rs) return NULL;

    printf("PACMAN: loading romset '%s'\n", rs->name);

    p->type = rs->type;

    p->ROM      = calloc(1, PACMAN_ROM_SIZE);
    p->colorROM   = calloc(1, 32);
    p->paletteROM = calloc(1, 256); 
    p->tileROM    = calloc(1, 4096);
    p->spriteROM  = calloc(1, 4096);
    p->audioROM   = calloc(1, PACMAN_AUDIO_ROM_SIZE);

    if (!p->ROM || !p->colorROM || !p->paletteROM ||
        !p->tileROM || !p->spriteROM || !p->audioROM) {
        PACMAN_free(p);
        return NULL;
    }

    load_rom(rom_archive, rs->rom_6e, p->ROM,          0x1000);
    load_rom(rom_archive, rs->rom_6f, p->ROM + 0x1000, 0x1000);
    load_rom(rom_archive, rs->rom_6h, p->ROM + 0x2000, 0x1000);
    load_rom(rom_archive, rs->rom_6j, p->ROM + 0x3000, 0x1000);

    load_rom(rom_archive, rs->color_rom,   p->colorROM,    32);
    load_rom(rom_archive, rs->palette_rom, p->paletteROM, 256);
    load_rom(rom_archive, rs->tile_rom,    p->tileROM,   4096);
    load_rom(rom_archive, rs->sprite_rom,  p->spriteROM, 4096);

    load_rom(rom_archive, rs->audio_rom[0], p->audioROM,       256);
    load_rom(rom_archive, rs->audio_rom[1], p->audioROM + 256, 256);

    if (rs->type == PACMAN_TYPE_MSPACMAN)
        init_aux_board_mspacman(p, rom_archive);

    if (rs->decrypt)
        rs->decrypt(p);

    p->IN0               = 0xFF;
    p->IN1               = 0xFF;
    p->DIP_SWITCH_SETTINGS = rs->default_dip_switch;
    p->input_map = rs->input_map;

    p->is_270_degree = rs->is_270_degree;
    return p;
}


void PACMAN_run_frame(pacman_t* p)
{
    pacman_update_input(p);

    while (p->z80.cycles < (u64)PACMAN_CYCLES_PER_FRAME) {
        u64 old_cycles = p->z80.cycles;
        z80_step(&p->z80);
        PACMAN_push_sample(p, (int)(p->z80.cycles - old_cycles));
    }

    p->z80.cycles -= PACMAN_CYCLES_PER_FRAME;

    if (p->VBLANK_ENABLED && z80_is_interrupt_enabled(&p->z80)) {
        p->z80.INTERRUPT_VECT = p->IO;
        z80_irq(&p->z80);
    }

    pacman_draw_video(p);
    renderPixels();
}

byte_vec_t PACMAN_savestate(pacman_t* p)
{
    byte_vec_t state;
    byte_vec_init(&state);
    serialize_pacman_t(p, &state);
    byte_vec_shrink(&state);
    return state;
}

bool PACMAN_loadstate(pacman_t* p, byte_vec_t* state)
{
    const u8* end = state->data + state->size;
    u8* data = deserialize_pacman_t(p, state->data, (u8*)end);
    if (!data) return false;

    return data == end;
}
