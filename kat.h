/**
 * kat.h
 */

#include <stdint.h>  /* For int types */

#ifndef __KAT_H
#define __KAT_H

/* NOTE: There is no build-in boolean in C */
#define TRUE  1
#define FALSE 0

/* NOTE: Disambiguate static keyword */
#define global static
#define internal static
#define persist static

typedef int8_t   int8;
typedef int16_t  int16;
typedef int32_t  int32;
typedef int64_t  int64;
typedef int32    bool32;
typedef int64    bool64;

typedef uint8_t  uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

typedef float    real32;
typedef double   real64;

typedef int8     i8;
typedef int16    i16;
typedef int32    i32;
typedef int64    i64;
typedef bool32   b32;
typedef bool64   b64;

typedef uint8    u8;
typedef uint16   u16;
typedef uint32   u32;
typedef uint64   u64;

typedef real32   r32;
typedef real64   r64;

#define pi32     3.14159265359f

#define MAX_PATH 4096

/* FEDEC Denominations */
#define Kilobytes(Value) ((Value)*1024LL)
#define Megabytes(Value) (Kilobytes(Value)*1024LL)
#define Gigabytes(Value) (Megabytes(Value)*1024LL)
#define Terabytes(Value) (Gigabytes(Value)*1024LL)

#endif /*__KAT_H */
