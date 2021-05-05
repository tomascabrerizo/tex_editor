#ifndef EDITOR_H
#define EDITOR_H

#include <stdint.h>

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;

typedef float r32;
typedef double r64;

#define internal static
#define global_varible static

// TODO(tomi): Define DEBUG compiler flag
#define assert(condition) if(!(condition)) {*(u32*)0 = 0;}
#define array_count(array) ((int)sizeof(array)/(int)sizeof(array[0]))

typedef struct
{
    int width;
    int height;
    int bytes_per_pixel;
    void* memory;
} Backbuffer;

typedef struct
{
    int width;
    int height;
    int bytes_per_pixel;
    int pitch;
    void* memory;
} Bitmap;

// TODO(tomi): This should be a pointer or and index on the array
// for now because the array is fixed size i'll use index
#define FILE_SIZE (10 * 1024)

typedef struct
{
    int number;
    // TODO(tomi): Dinamyc chunk of bytes
} Line;

typedef struct
{
    int number_of_lines;
    int line_width[256];
    // TODO(tomi): Dinamyc chunk of Lines 
    char memory[FILE_SIZE];
} File;

typedef struct
{
    u32 index; 
    // TODO(tomi): Maybe last index should be part of file
    u32 last_index;
    int pos_x;
    int pos_y;
    int line_number;
} Cursor;


typedef struct
{
    File file;
    Cursor cursor;
    Bitmap font[256];

} Editor_State;


#endif //EDITOR_H
