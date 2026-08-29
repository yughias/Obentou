#include "chips/z80.h"
#include "types.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "cJSON.h"

static u8 memory[1 << 16];
static u8 final_mem[1 << 16];
static u8 io[1 << 16];

static u8 read_mem(void *ctx, u16 addr) { return memory[addr]; }
static void write_mem(void *ctx, u16 addr, u8 byte) { memory[addr] = byte; }

static u8 read_io(void *ctx, u16 addr) { return io[addr]; }
static void write_io(void *ctx, u16 addr, u8 byte) { io[addr] = byte; }

static char *read_file(const char *path)
{
    FILE *f = fopen(path, "rb");
    if (!f) return NULL;

    fseek(f, 0, SEEK_END);
    long len = ftell(f);
    rewind(f);

    char *buf = malloc(len + 1);
    if (!buf) { fclose(f); return NULL; }
    fread(buf, 1, len, f);
    buf[len] = '\0';
    fclose(f);
    return buf;
}

static bool compareZ80(z80_t *a, z80_t *b)
{
    if (a->AF   != b->AF)   return false;
    if (a->AF_  != b->AF_)  return false;
    if (a->BC   != b->BC)   return false;
    if (a->BC_  != b->BC_)  return false;
    if (a->DE   != b->DE)   return false;
    if (a->DE_  != b->DE_)  return false;
    if (a->HL   != b->HL)   return false;
    if (a->HL_  != b->HL_)  return false;
    if (a->IX   != b->IX)   return false;
    if (a->IY   != b->IY)   return false;
    if (a->PC   != b->PC)   return false;
    if (a->SP   != b->SP)   return false;
    if (a->I    != b->I)    return false;
    if (a->R    != b->R)    return false;
    if (a->INTERRUPT_MODE != b->INTERRUPT_MODE) return false;
    if (a->IFF1 != b->IFF1) return false;
    if (a->IFF2 != b->IFF2) return false;
    if (a->cycles != b->cycles) return false;
    if (a->WZ   != b->WZ)   return false;
    return true;
}

static void setZ80(z80_t *z80, cJSON *init)
{
    memset(z80, 0, sizeof(z80_t));
    memset(memory, 0, sizeof(memory));
    z80->readIO      = read_io;
    z80->writeIO     = write_io;
    z80->writeMemory = write_mem;
    z80->readMemory  = read_mem;

    z80->PC               = cJSON_GetObjectItem(init, "pc")->valuedouble;
    z80->SP               = cJSON_GetObjectItem(init, "sp")->valuedouble;
    z80->A                = cJSON_GetObjectItem(init, "a")->valuedouble;
    z80->B                = cJSON_GetObjectItem(init, "b")->valuedouble;
    z80->C                = cJSON_GetObjectItem(init, "c")->valuedouble;
    z80->D                = cJSON_GetObjectItem(init, "d")->valuedouble;
    z80->E                = cJSON_GetObjectItem(init, "e")->valuedouble;
    z80->F                = cJSON_GetObjectItem(init, "f")->valuedouble;
    z80->H                = cJSON_GetObjectItem(init, "h")->valuedouble;
    z80->L                = cJSON_GetObjectItem(init, "l")->valuedouble;
    z80->I                = cJSON_GetObjectItem(init, "i")->valuedouble;
    z80->R                = cJSON_GetObjectItem(init, "r")->valuedouble;
    z80->INTERRUPT_DELAY  = cJSON_GetObjectItem(init, "ei")->valuedouble;
    z80->IFF1             = cJSON_GetObjectItem(init, "iff1")->valuedouble;
    z80->IFF2             = cJSON_GetObjectItem(init, "iff2")->valuedouble;
    z80->IX               = cJSON_GetObjectItem(init, "ix")->valuedouble;
    z80->IY               = cJSON_GetObjectItem(init, "iy")->valuedouble;
    z80->AF_              = cJSON_GetObjectItem(init, "af_")->valuedouble;
    z80->BC_              = cJSON_GetObjectItem(init, "bc_")->valuedouble;
    z80->DE_              = cJSON_GetObjectItem(init, "de_")->valuedouble;
    z80->HL_              = cJSON_GetObjectItem(init, "hl_")->valuedouble;
    z80->Q                = cJSON_GetObjectItem(init, "q")->valuedouble;
    z80->WZ               = cJSON_GetObjectItem(init, "wz")->valuedouble;
    z80->INTERRUPT_MODE   = cJSON_GetObjectItem(init, "im")->valuedouble;

    cJSON *ram = cJSON_GetObjectItem(init, "ram");
    cJSON *entry;
    cJSON_ArrayForEach(entry, ram) {
        int addr = cJSON_GetArrayItem(entry, 0)->valuedouble;
        int byte = cJSON_GetArrayItem(entry, 1)->valuedouble;
        memory[addr] = byte;
    }
}

static void set_io(cJSON *test)
{
    cJSON *ports = cJSON_GetObjectItem(test, "ports");
    if (!ports) return;

    cJSON *entry;
    cJSON_ArrayForEach(entry, ports) {
        int addr = cJSON_GetArrayItem(entry, 0)->valuedouble;
        int byte = cJSON_GetArrayItem(entry, 1)->valuedouble;
        io[addr] = byte;
    }
}

static void runTest(cJSON *data)
{
    int total = cJSON_GetArraySize(data);
    cJSON *test = data->child;
    for (int i = 0; i < total; i++, test = test->next) {
        cJSON *init  = cJSON_GetObjectItem(test, "initial");
        cJSON *end   = cJSON_GetObjectItem(test, "final");

        z80_t z80, z80_end;

        set_io(test);
        setZ80(&z80, init);
        z80_step(&z80);

        memcpy(final_mem, memory, sizeof(memory));

        setZ80(&z80_end, end);

        cJSON *cycles_arr = cJSON_GetObjectItem(test, "cycles");
        z80_end.cycles = cJSON_GetArraySize(cycles_arr);

        if (!compareZ80(&z80, &z80_end)) {
            printf("%d/%d REG ERROR\n", i + 1, total);
            exit(EXIT_FAILURE);
        }

        if (memcmp(final_mem, memory, sizeof(memory))) {
            printf("%d/%d\n MEM ERROR", i + 1, total);
            exit(EXIT_FAILURE);
        }
    }
    printf("%d/%d OK\n", total, total);
}

static void testTable(const char *tableName)
{
    char prefix[0x100];
    if (!tableName) {
        prefix[0] = '\0';
    } else {
        strcpy(prefix, tableName);
        strcat(prefix, " ");
    }

    for (int i = 0x00; i < 0x100; i++) {
        char filename[0x100];
        snprintf(filename, sizeof(filename), "test/assets/sst_z80/v1/%s%02x.json", prefix, i);

        char *text = read_file(filename);
        if (!text) continue;

        printf("TEST %s ", filename);

        cJSON *data = cJSON_Parse(text);
        free(text);

        if (!data) {
            fprintf(stderr, "JSON parse error in %s: %s\n",
                    filename, cJSON_GetErrorPtr());
            exit(EXIT_FAILURE);
        }

        runTest(data);
        cJSON_Delete(data);
    }
}

int main() {
    testTable(NULL);
    testTable("cb");
    testTable("ed");
    testTable("dd");
    testTable("fd");
    testTable("dd cb __");
    testTable("fd cb __");
    return 0;
}