#ifndef __DB_H__
#define __DB_H__

#include "types.h"

typedef u8 db_hash;

#define DB_NOT_FOUND 0xFF

db_hash gba_db_search(u8* gamecode);
u16 gba_db_get_id_code(db_hash hash);
size_t gba_db_get_size(db_hash hash);

#endif