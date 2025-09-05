#ifndef __SKIP_BOOTROM_H__
#define __SKIP_BOOTROM_H__

typedef struct gb_t gb_t;

void gb_skipDmgBootrom(gb_t*);
void gb_skipCgbBootrom(gb_t*);
void gb_hleDmgColorization(gb_t*);

#endif