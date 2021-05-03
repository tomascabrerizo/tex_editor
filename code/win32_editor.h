#ifndef WIN32_EDITOR_H
#define WIN32_EDITOR_H

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

// TODO(tomi): Clean up this code plissss!

// TODO(tomi): Define DEBUG compiler flag
#define assert(condition) if(!(condition)) {*(u32*)0 = 0;}
#define array_count(array) ((int)sizeof(array)/(int)sizeof(array[0]))

typedef struct
{
    int width;
    int height;
    int pitch;
    int bytes_per_pixel;
    void* memory;
} Win32_Bitmap;

global_varible Win32_Bitmap win32_letters[256];

// TODO(tomi): This should be a pointer or and index on the array
// for now because the array is fixed size i'll use index
#define FILE_SIZE (10 * 1024)

typedef struct
{
    int number_of_lines;
    int line_width[256];
    char memory[FILE_SIZE];
} File;

typedef struct
{
    u32 index; 
    // TODO(tomi): Maybe last index should be part of file
    u32 last_index;
    int pos_x;
    int pos_y;
} Cursor;

typedef struct
{
    BITMAPINFO info;
    int width;
    int height;
    int bytes_per_pixel;
    void* memory;

} Win32_Backbuffer;


#endif //WIN32_EDITOR_H
