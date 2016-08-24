/* ===============================================================
 *
 * this header file re-defines the interger/float/double data types
 *
 * =============================================================== */
#ifndef KIWI_DATA_TYPES_H
#define KIWI_DATA_TYPES_H

#include <stdint.h>

typedef int8_t S8;
typedef uint8_t U8;
typedef int16_t S16;
typedef uint16_t U16;
typedef int32_t S32;
typedef uint32_t U32;
typedef int64_t S64;
typedef uint64_t U64;

typedef unsigned char BOOLEAN;

// no need to define true and false
// it is a little bit dangerous to use true
#ifndef false
  #define false 0
  #define true 1
#endif

#endif
