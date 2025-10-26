#ifndef __SERIALIZER_H__
#define __SERIALIZER_H__

#include "types.h"

#include "utils/vec.h"

DEFINE_VEC(byte_vec, u8);

#define GET_MACRO(_1, _2, _3, _4, _5, NAME, ...) NAME

/* SERIALIZER */

#define SERIALIZE_IS_ARRAY_0_IS_SAVE_0(type, name, isRecursive)
#define SERIALIZE_IS_ARRAY_1_IS_SAVE_0(type, name, arrayLen, isRecursive)

#define SERIALIZE_IS_ARRAY_0_IS_SAVE_1(type, name, isRecursive) SERIALIZE_IS_ARRAY_0_IS_RECURSIVE_ ## isRecursive (type, name)
#define SERIALIZE_IS_ARRAY_1_IS_SAVE_1(type, name, arrayLen, isRecursive) SERIALIZE_IS_ARRAY_1_IS_RECURSIVE_ ## isRecursive (type, name, arrayLen)

#define SERIALIZE_IS_ARRAY_0_IS_RECURSIVE_0(type, name) byte_vec_push_array(vec, (u8*)&obj->name, sizeof(obj->name));
#define SERIALIZE_IS_ARRAY_0_IS_RECURSIVE_1(type, name) serialize_##type(&obj->name, vec);

#define SERIALIZE_IS_ARRAY_1_IS_RECURSIVE_0(type, name, arrayLen) byte_vec_push_array(vec, (u8*)obj->name, sizeof(obj->name));
#define SERIALIZE_IS_ARRAY_1_IS_RECURSIVE_1(type, name, arrayLen) for(int i = 0; i < arrayLen; i++) serialize_##type(&obj->name[i], vec);

#define SERIALIZE_IS_ARRAY_0(type, name, isSave, isRecursive) SERIALIZE_IS_ARRAY_0_IS_SAVE_ ## isSave (type, name, isRecursive)
#define SERIALIZE_IS_ARRAY_1(type, name, arrayLen, isSave, isRecursive) SERIALIZE_IS_ARRAY_1_IS_SAVE_ ## isSave (type, name, arrayLen, isRecursive)

#define SERIALIZE(...) GET_MACRO(__VA_ARGS__, SERIALIZE_IS_ARRAY_1, SERIALIZE_IS_ARRAY_0)(__VA_ARGS__)

/* DESERIALIZER */

#define DESERIALIZE_IS_ARRAY_0_IS_SAVE_0(type, name, isRecursive)
#define DESERIALIZE_IS_ARRAY_1_IS_SAVE_0(type, name, arrayLen, isRecursive)

#define DESERIALIZE_IS_ARRAY_0_IS_SAVE_1(type, name, isRecursive) DESERIALIZE_IS_ARRAY_0_IS_RECURSIVE_ ## isRecursive (type, name)
#define DESERIALIZE_IS_ARRAY_1_IS_SAVE_1(type, name, arrayLen, isRecursive) DESERIALIZE_IS_ARRAY_1_IS_RECURSIVE_ ## isRecursive (type, name, arrayLen)

#define DESERIALIZE_IS_ARRAY_0_IS_RECURSIVE_0(type, name) memcpy(&obj->name, data, sizeof(obj->name)); data += sizeof(obj->name);
#define DESERIALIZE_IS_ARRAY_0_IS_RECURSIVE_1(type, name) data = deserialize_##type(&obj->name, data);

#define DESERIALIZE_IS_ARRAY_1_IS_RECURSIVE_0(type, name, arrayLen) memcpy(obj->name, data, sizeof(obj->name)); data += sizeof(obj->name);
#define DESERIALIZE_IS_ARRAY_1_IS_RECURSIVE_1(type, name, arrayLen) for(int i = 0; i < arrayLen; i++) data = deserialize_##type(&obj->name[i], data);

#define DESERIALIZE_IS_ARRAY_0(type, name, isSave, isRecursive) DESERIALIZE_IS_ARRAY_0_IS_SAVE_ ## isSave (type, name, isRecursive)
#define DESERIALIZE_IS_ARRAY_1(type, name, arrayLen, isSave, isRecursive) DESERIALIZE_IS_ARRAY_1_IS_SAVE_ ## isSave (type, name, arrayLen, isRecursive)

#define DESERIALIZE(...) GET_MACRO(__VA_ARGS__, DESERIALIZE_IS_ARRAY_1, DESERIALIZE_IS_ARRAY_0)(__VA_ARGS__)

#define DECLARE_STRUCT(name, fields) \
typedef struct name##_t { \
    fields \
} name##_t;

#define CREATE_SERIALIZER(name, fields) \
static void serialize_##name##_t(name##_t* obj, byte_vec_t* vec) { \
    fields \
}

#define CREATE_DESERIALIZER(name, fields) \
static u8* deserialize_##name##_t(name##_t* obj, u8* data) { \
    fields \
    return data; \
}

/* DECLARATOR */

#define DECLARE(...) GET_MACRO(__VA_ARGS__, DECLARE_IS_ARRAY_1, DECLARE_IS_ARRAY_0)(__VA_ARGS__)

#define DECLARE_IS_ARRAY_0(type, name, isSave, isRecursive) type name;
#define DECLARE_IS_ARRAY_1(type, name, arrayLen, isSave, isRecursive) type name[arrayLen];

/*
    USAGE:
    #define MY_STRUCT(X) \
        X(type, name, isArray, arrayLen, isSave, isRecursive) \
        ...

    DECLARE_SERIALIZABLE_STRUCT(MyStruct, MY_STRUCT)
*/
#define DECLARE_SERIALIZABLE_STRUCT(name, fields) \
DECLARE_STRUCT(name, fields(DECLARE)) \
CREATE_SERIALIZER(name, fields(SERIALIZE)) \
CREATE_DESERIALIZER(name, fields(DESERIALIZE))

#endif