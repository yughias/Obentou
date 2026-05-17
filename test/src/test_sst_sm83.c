#include "cpus/sm83.h"
#include "types.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "cJSON.h"

// STOP AND HALT OPCODES ARE SKIPPED

static u8 memory[1 << 16];
static u8 final_mem[1 << 16];

static u8 read_mem(void *ctx, u16 addr) { return memory[addr]; }
static void write_mem(void *ctx, u16 addr, u8 byte) { memory[addr] = byte; }
static void tick_system(void *ctx, int cycles) { ((sm83_t*)ctx)->cycles += cycles / 4; }

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

static bool compare_sm83(sm83_t *a, sm83_t *b)
{
    if (a->AF   != b->AF)   return false;
    if (a->BC   != b->BC)   return false;
    if (a->DE   != b->DE)   return false;
    if (a->HL   != b->HL)   return false;
    if (a->PC   != b->PC)   return false;
    if (a->SP   != b->SP)   return false;
    if (a->cycles != b->cycles) return false;
    return true;
}

static void set_sm83(sm83_t *sm83, cJSON *init)
{
    memset(sm83, 0, sizeof(sm83_t));
    memset(memory, 0, sizeof(memory));
    sm83->writeByte = write_mem;
    sm83->readByte  = read_mem;
    sm83->tickSystem = tick_system;
    sm83->ctx = sm83;

    sm83->PC               = cJSON_GetObjectItem(init, "pc")->valuedouble;
    sm83->SP               = cJSON_GetObjectItem(init, "sp")->valuedouble;
    sm83->A                = cJSON_GetObjectItem(init, "a")->valuedouble;
    sm83->B                = cJSON_GetObjectItem(init, "b")->valuedouble;
    sm83->C                = cJSON_GetObjectItem(init, "c")->valuedouble;
    sm83->D                = cJSON_GetObjectItem(init, "d")->valuedouble;
    sm83->E                = cJSON_GetObjectItem(init, "e")->valuedouble;
    sm83->F                = cJSON_GetObjectItem(init, "f")->valuedouble;
    sm83->H                = cJSON_GetObjectItem(init, "h")->valuedouble;
    sm83->L                = cJSON_GetObjectItem(init, "l")->valuedouble;

    cJSON *ram = cJSON_GetObjectItem(init, "ram");
    cJSON *entry;
    cJSON_ArrayForEach(entry, ram) {
        int addr = cJSON_GetArrayItem(entry, 0)->valuedouble;
        int byte = cJSON_GetArrayItem(entry, 1)->valuedouble;
        memory[addr] = byte;
    }
}

static void runTest(cJSON *data)
{
    int total = cJSON_GetArraySize(data);
    cJSON *test = data->child;
    for (int i = 0; i < total; i++, test = test->next) {
        cJSON *init  = cJSON_GetObjectItem(test, "initial");
        cJSON *end   = cJSON_GetObjectItem(test, "final");

        sm83_t sm83, sm83_end;

        set_sm83(&sm83, init);
        sm83_stepCPU(&sm83);

        memcpy(final_mem, memory, sizeof(memory));

        set_sm83(&sm83_end, end);


        cJSON *cycles_arr = cJSON_GetObjectItem(test, "cycles");
        sm83_end.cycles = cJSON_GetArraySize(cycles_arr);

        if (!compare_sm83(&sm83, &sm83_end)) {
            printf("%d/%d REG ERROR\n", i + 1, total);
            printf("GOT:\n");
            sm83_infoCPU(&sm83);
            printf("EXPECTED:\n");
            sm83_infoCPU(&sm83_end);
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
        // skip stop opcode
        if (!tableName && i == 0x10)
            continue;
        // skip halt opcode
        if (!tableName && i == 0x76)
            continue;
        char filename[0x100];
        snprintf(filename, sizeof(filename), "test/assets/sst_sm83/v1/%s%02x.json", prefix, i);

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
    return 0;
}