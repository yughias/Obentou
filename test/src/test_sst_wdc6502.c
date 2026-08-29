#include "chips/w65c02.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "cJSON.h"

typedef struct {
    char type;
    u16  addr;
    u8   byte;
} cycle_t;

static cycle_t cycles[256];
static u8      memory[0x10000];
static u8      final_mem[0x10000];

static u8 read_mem(void *ctx, u16 addr)
{
    w65c02_t *cpu = (w65c02_t *)ctx;
    cycle_t  *c   = &cycles[cpu->cycles];
    c->byte = memory[addr];
    c->addr = addr;
    c->type = 'r';
    return memory[addr];
}

static void write_mem(void *ctx, u16 addr, u8 val)
{
    w65c02_t *cpu = (w65c02_t *)ctx;
    cycle_t  *c   = &cycles[cpu->cycles];
    c->byte = val;
    c->addr = addr;
    c->type = 'w';
    memory[addr] = val;
}

static const int illegal_opcodes[] = {
    0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0,
    0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0,
    0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0,
    0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0,
    0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0,
    0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0,
    0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0,
    0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0,
    0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0,
    0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0,
    0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0,
    0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0,
    0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0,
    0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0,
    0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0,
    0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0
};

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

static int compare6502(const w65c02_t *a, const w65c02_t *b)
{
    return (a->pc     == b->pc) &&
           (a->a      == b->a)  &&
           (a->s      == b->s)  &&
           (a->p      == b->p)  &&
           (a->x      == b->x)  &&
           (a->y      == b->y)  &&
           (a->cycles == b->cycles);
}

static void set6502(w65c02_t *cpu, cJSON *state)
{
    memset(cpu, 0, sizeof(w65c02_t));
    memset(memory, 0, sizeof(memory));

    cpu->write = write_mem;
    cpu->read  = read_mem;
    cpu->ctx   = (void *)cpu;

    cpu->pc = cJSON_GetObjectItem(state, "pc")->valuedouble;
    cpu->s  = cJSON_GetObjectItem(state, "s")->valuedouble;
    cpu->p  = cJSON_GetObjectItem(state, "p")->valuedouble;
    cpu->a  = cJSON_GetObjectItem(state, "a")->valuedouble;
    cpu->x  = cJSON_GetObjectItem(state, "x")->valuedouble;
    cpu->y  = cJSON_GetObjectItem(state, "y")->valuedouble;

    cJSON *entry = cJSON_GetObjectItem(state, "ram")->child;
    while (entry) {
        int addr = (int)entry->child->valuedouble;
        int byte = (int)entry->child->next->valuedouble;
        memory[addr] = (u8)byte;
        entry = entry->next;
    }
}

static int checkCycles(cJSON *j_cycles, int n)
{
    int i = 0;
    cJSON *entry = j_cycles->child;
    while (entry) {
        cJSON *e0    = entry->child;
        int  j_addr  = (int)e0->valuedouble;
        int  j_byte  = (int)e0->next->valuedouble;
        char j_type  = e0->next->next->valuestring[0];

        cycle_t *c = &cycles[i];
        if (j_type != c->type || j_addr != c->addr || j_byte != c->byte) {
            printf("  cycle %d: expected [%04X %02X %c] got [%04X %02X %c]\n",
                   i, j_addr, j_byte, j_type, c->addr, c->byte, c->type);
            int j = 0;
            cJSON *e = j_cycles->child;
            while (e) {
                cJSON *a = e->child;
                printf("%d: got [%04X %02X %c]  exp [%04X %02X %c]\n",
                       j,
                       cycles[j].addr, cycles[j].byte, cycles[j].type,
                       (int)a->valuedouble,
                       (int)a->next->valuedouble,
                       a->next->next->valuestring[0]);
                e = e->next; j++;
            }
            return 0;
        }
        entry = entry->next;
        i++;
    }
    return 1;
}

static void runTest(cJSON *data)
{
    int   total = cJSON_GetArraySize(data);
    cJSON *test = data->child;

    for (int i = 0; i < total; i++, test = test->next) {
        cJSON *init     = cJSON_GetObjectItem(test, "initial");
        cJSON *end      = cJSON_GetObjectItem(test, "final");
        cJSON *j_cycles = cJSON_GetObjectItem(test, "cycles");
        int    n        = cJSON_GetArraySize(j_cycles);

        w65c02_t cpu, cpu_end;

        set6502(&cpu, init);
        w65c02_step(&cpu);
        memcpy(final_mem, memory, sizeof(memory));

        set6502(&cpu_end, end);
        cpu_end.cycles = n;

        if (!compare6502(&cpu, &cpu_end)) {
            printf("%d/%d REG ERROR\n", i + 1, total);
            w65c02_print(&cpu);
            printf("EXPECTED:\n");
            w65c02_print(&cpu_end);
            printf("INITIAL:\n");
            set6502(&cpu, init);
            w65c02_print(&cpu);
            exit(EXIT_FAILURE);
        }

        if (memcmp(final_mem, memory, sizeof(memory))) {
            printf("%d/%d MEM ERROR\n", i + 1, total);
            for (int j = 0; j < (int)sizeof(memory); j++)
                if (memory[j] != final_mem[j])
                    printf("  addr %04X: expected %02X got %02X\n",
                           j, final_mem[j], memory[j]);
            exit(EXIT_FAILURE);
        }

        if (!checkCycles(j_cycles, n)) {
            printf("%d/%d CYCLE ERROR\n", i + 1, total);
            exit(EXIT_FAILURE);
        }
    }
    printf("%d/%d OK\n", total, total);
}

int main(void)
{
    for (int i = 0x00; i < 0x100; i++) {
        if (illegal_opcodes[i])
            continue;

        char filename[0x100];
        snprintf(filename, sizeof(filename),
                 "test/assets/sst_65X02/wdc65c02/v1/%02x.json", i);

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
    printf("TEST COMPLETED!\n");
    return 0;
}