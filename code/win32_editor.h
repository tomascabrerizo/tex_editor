#ifndef WIN32_EDITOR_H
#define WIN32_EDITOR_H

#include "editor.h"

typedef struct
{
    BITMAPINFO info;
    int width;
    int height;
    int bytes_per_pixel;
    void* memory;
} Win32_Backbuffer;

// NOTE(tomi): Functions from editor to Platform
void init_editor(Editor_State *state);
void draw_editor(Backbuffer *buffer, Editor_State *state);

#endif //WIN32_EDITOR_H
