#include "cores/gbc/mbc.h"
#include "cores/gbc/mbcs/mbc_cam.h"
#include "cores/gbc/memory.h"
#include "cores/gbc/gb.h"

#include "peripherals/camera.h"

#include <string.h>
#include <stdlib.h>
#include <time.h>

#define GBCAM_SENSOR_EXTRA_LINES (8)
#define GBCAM_W (128)
#define GBCAM_H (112)
#define GBCAM_SENSOR_W (128)
#define GBCAM_SENSOR_H (112 + GBCAM_SENSOR_EXTRA_LINES)
#define BIT(n) (1 << (n))

typedef struct mbc_cam_t
{
    u8 regs[0x36];
    int frame[GBCAM_SENSOR_W * GBCAM_SENSOR_H];
} mbc_cam_t;

void getCapturedFrame(gb_t *gb);

void gb_mbc_cam_init(gb_t *gb)
{
    gb->mbc.data = malloc(sizeof(mbc_cam_t));
    memset(gb->mbc.data, 0, sizeof(mbc_cam_t));
    camera_open(GBCAM_SENSOR_W, GBCAM_SENSOR_H);
}

void gb_mbc_cam_free(gb_t *gb)
{
    free(gb->mbc.data);
    camera_close();
}

u8 gb_mbc_cam_ram_read(gb_t *gb, u16 addr)
{
    mbc_t *mbc = &gb->mbc;
    mbc_cam_t *cam = gb->mbc.data;
    size_t real_addr = addr;
    real_addr &= (1 << 13) - 1;
    if (mbc->REG_4000_5FFF & 0x10)
    {
        real_addr &= 0x7F;
        if (!real_addr)
        {
            getCapturedFrame(gb);
            cam->regs[0] &= 0x07;
            return cam->regs[0];
        }
        else
            return 0;
    }

    real_addr |= (mbc->REG_4000_5FFF & 0x0F) << 13;
    real_addr &= gb->ERAM_SIZE - 1;
    return gb->ERAM[real_addr];
}

void gb_mbc_cam_ram_write(gb_t *gb, u16 addr, u8 byte)
{
    mbc_t *mbc = &gb->mbc;
    mbc_cam_t *cam = gb->mbc.data;
    size_t real_addr = addr;
    real_addr &= (1 << 13) - 1;

    if (mbc->REG_4000_5FFF & 0x10)
    {
        real_addr &= 0x7F;
        if (real_addr >= 0x36)
            return;
        cam->regs[real_addr] = byte;
        // if start capturing
        if (!real_addr && (byte & 1))
            camera_copy_frame(cam->frame);
    }
    else
    {
        real_addr |= (mbc->REG_4000_5FFF & 0x0F) << 13;
        real_addr &= gb->ERAM_SIZE - 1;
        gb->ERAM[real_addr] = byte;
    }
}

// from this point code is heavily taken from https://gbdev.io/pandocs/Gameboy_Camera.html

static inline int clamp(int min, int value, int max)
{
    if (value < min)
        return min;
    if (value > max)
        return max;
    return value;
}

static inline int min(int a, int b) { return (a < b) ? a : b; }

static inline int max(int a, int b) { return (a > b) ? a : b; }

static inline uint32_t gb_cam_matrix_process(u8 *regs, uint32_t value, uint32_t x, uint32_t y)
{
    x = x & 3;
    y = y & 3;

    int base = 6 + (y * 4 + x) * 3;

    uint32_t r0 = regs[base + 0];
    uint32_t r1 = regs[base + 1];
    uint32_t r2 = regs[base + 2];

    if (value < r0)
        return 0x00;
    else if (value < r1)
        return 0x40;
    else if (value < r2)
        return 0x80;
    return 0xC0;
}

void getCapturedFrame(gb_t *gb)
{
    mbc_cam_t *cam = (mbc_cam_t *)gb->mbc.data;
    u8 *regs = cam->regs;
    int i, j;
    // Get configuration
    // -----------------

    // Register 0
    uint32_t P_bits = 0;
    uint32_t M_bits = 0;

    switch ((regs[0] >> 1) & 3)
    {
    case 0:
        P_bits = 0x00;
        M_bits = 0x01;
        break;
    case 1:
        P_bits = 0x01;
        M_bits = 0x00;
        break;
    case 2:
    case 3:
        P_bits = 0x01;
        M_bits = 0x02;
        break;
    default:
        break;
    }

    // Register 1
    uint32_t N_bit = (regs[1] & BIT(7)) >> 7;
    uint32_t VH_bits = (regs[1] & (BIT(6) | BIT(5))) >> 5;

    // Registers 2 and 3
    uint32_t EXPOSURE_bits = regs[3] | (regs[2] << 8);

    // Register 4
    const float edge_ratio_lut[8] = {0.50, 0.75, 1.00, 1.25, 2.00, 3.00, 4.00, 5.00};

    float EDGE_alpha = edge_ratio_lut[(regs[4] & 0x70) >> 4];

    uint32_t E3_bit = (regs[4] & BIT(7)) >> 7;
    uint32_t I_bit = (regs[4] & BIT(3)) >> 3;

    //------------------------------------------------

    // Sensor handling
    // ---------------

    // Copy webcam buffer to sensor buffer applying color correction and exposure time
    int gb_cam_retina_output_buf[GBCAM_SENSOR_W][GBCAM_SENSOR_H];
    for (i = 0; i < GBCAM_SENSOR_W; i++)
        for (j = 0; j < GBCAM_SENSOR_H; j++)
        {
            int col = cam->frame[i + j * GBCAM_W];
            int r = (col >> 16) & 0xFF;
            int g = (col >> 8) & 0xFF;
            int b = col & 0xFF;
            int value = 0.2126 * r + 0.7152 * g + 0.0722 * b;
            value = ((value * EXPOSURE_bits) / 0x300); // 0x0300 could be other values
            value = 128 + (((value - 128) * 1) / 8);   // "adapt" to "3.1"/5.0 V
            gb_cam_retina_output_buf[i][j] = clamp(0, value, 255);
        }

    if (I_bit) // Invert image
    {
        for (i = 0; i < GBCAM_SENSOR_W; i++)
            for (j = 0; j < GBCAM_SENSOR_H; j++)
            {
                gb_cam_retina_output_buf[i][j] = 255 - gb_cam_retina_output_buf[i][j];
            }
    }

    // Make signed
    for (i = 0; i < GBCAM_SENSOR_W; i++)
        for (j = 0; j < GBCAM_SENSOR_H; j++)
        {
            gb_cam_retina_output_buf[i][j] = gb_cam_retina_output_buf[i][j] - 128;
        }

    int temp_buf[GBCAM_SENSOR_W][GBCAM_SENSOR_H];

    uint32_t filtering_mode = (N_bit << 3) | (VH_bits << 1) | E3_bit;
    switch (filtering_mode)
    {
    case 0x0: // 1-D filtering
    {
        for (i = 0; i < GBCAM_SENSOR_W; i++)
            for (j = 0; j < GBCAM_SENSOR_H; j++)
            {
                temp_buf[i][j] = gb_cam_retina_output_buf[i][j];
            }
        for (i = 0; i < GBCAM_SENSOR_W; i++)
            for (j = 0; j < GBCAM_SENSOR_H; j++)
            {
                int ms = temp_buf[i][min(j + 1, GBCAM_SENSOR_H - 1)];
                int px = temp_buf[i][j];

                int value = 0;
                if (P_bits & BIT(0))
                    value += px;
                if (P_bits & BIT(1))
                    value += ms;
                if (M_bits & BIT(0))
                    value -= px;
                if (M_bits & BIT(1))
                    value -= ms;
                gb_cam_retina_output_buf[i][j] = clamp(-128, value, 127);
            }
        break;
    }
    case 0x2: // 1-D filtering + Horiz. enhancement : P + {2P-(MW+ME)} * alpha
    {
        for (i = 0; i < GBCAM_SENSOR_W; i++)
            for (j = 0; j < GBCAM_SENSOR_H; j++)
            {
                int mw = gb_cam_retina_output_buf[max(0, i - 1)][j];
                int me = gb_cam_retina_output_buf[min(i + 1, GBCAM_SENSOR_W - 1)][j];
                int px = gb_cam_retina_output_buf[i][j];

                temp_buf[i][j] = clamp(0, px + ((2 * px - mw - me) * EDGE_alpha), 255);
            }
        for (i = 0; i < GBCAM_SENSOR_W; i++)
            for (j = 0; j < GBCAM_SENSOR_H; j++)
            {
                int ms = temp_buf[i][min(j + 1, GBCAM_SENSOR_H - 1)];
                int px = temp_buf[i][j];

                int value = 0;
                if (P_bits & BIT(0))
                    value += px;
                if (P_bits & BIT(1))
                    value += ms;
                if (M_bits & BIT(0))
                    value -= px;
                if (M_bits & BIT(1))
                    value -= ms;
                gb_cam_retina_output_buf[i][j] = clamp(-128, value, 127);
            }
        break;
    }
    case 0xE: // 2D enhancement : P + {4P-(MN+MS+ME+MW)} * alpha
    {
        for (i = 0; i < GBCAM_SENSOR_W; i++)
            for (j = 0; j < GBCAM_SENSOR_H; j++)
            {
                int ms = gb_cam_retina_output_buf[i][min(j + 1, GBCAM_SENSOR_H - 1)];
                int mn = gb_cam_retina_output_buf[i][max(0, j - 1)];
                int mw = gb_cam_retina_output_buf[max(0, i - 1)][j];
                int me = gb_cam_retina_output_buf[min(i + 1, GBCAM_SENSOR_W - 1)][j];
                int px = gb_cam_retina_output_buf[i][j];

                temp_buf[i][j] = clamp(-128, px + ((4 * px - mw - me - mn - ms) * EDGE_alpha), 127);
            }
        for (i = 0; i < GBCAM_SENSOR_W; i++)
            for (j = 0; j < GBCAM_SENSOR_H; j++)
            {
                gb_cam_retina_output_buf[i][j] = temp_buf[i][j];
            }
        break;
    }
    case 0x1:
    {
        // In my GB Camera cartridge this is always the same color. The datasheet of the
        // sensor doesn't have this configuration documented. Maybe this is a bug?
        for (i = 0; i < GBCAM_SENSOR_W; i++)
            for (j = 0; j < GBCAM_SENSOR_H; j++)
            {
                gb_cam_retina_output_buf[i][j] = 0;
            }
        break;
    }
    default:
    {
        // Ignore filtering
        printf("Unsupported GB Cam mode: 0x%X\n"
               "%02X %02X %02X %02X %02X %02X",
               filtering_mode,
               regs[0], regs[1], regs[2],
               regs[3], regs[4], regs[5]);
        break;
    }
    }

    // Make unsigned
    for (i = 0; i < GBCAM_SENSOR_W; i++)
        for (j = 0; j < GBCAM_SENSOR_H; j++)
        {
            gb_cam_retina_output_buf[i][j] = gb_cam_retina_output_buf[i][j] + 128;
        }

    //------------------------------------------------

    // Controller handling
    // -------------------

    int fourcolorsbuffer[GBCAM_W][GBCAM_H]; // buffer after controller matrix

    // Convert to Game Boy colors using the controller matrix
    for (i = 0; i < GBCAM_W; i++)
        for (j = 0; j < GBCAM_H; j++)
            fourcolorsbuffer[i][j] =
                gb_cam_matrix_process(regs, gb_cam_retina_output_buf[i][j + (GBCAM_SENSOR_EXTRA_LINES / 2)], i, j);

    // Convert to tiles
    u8 finalbuffer[14][16][16]; // final buffer
    memset(finalbuffer, 0, sizeof(finalbuffer));
    for (i = 0; i < GBCAM_W; i++)
        for (j = 0; j < GBCAM_H; j++)
        {
            uint8_t outcolor = 3 - (fourcolorsbuffer[i][j] >> 6);

            uint8_t *tile_base = finalbuffer[j >> 3][i >> 3];
            tile_base = &tile_base[(j & 7) * 2];

            if (outcolor & 1)
                tile_base[0] |= 1 << (7 - (7 & i));
            if (outcolor & 2)
                tile_base[1] |= 1 << (7 - (7 & i));
        }

    // Copy to cart ram...
    memcpy(&gb->ERAM[0x0100], finalbuffer, sizeof(finalbuffer));

    regs[0] &= 0xFE;
}