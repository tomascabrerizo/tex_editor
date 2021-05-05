#include "editor.h"
#include <stdlib.h>

// TODO(tomi): Do something with this global
global_varible Editor_State global_state;

internal void
push_struct(D_Array *array, void* data, int size_of_data_in_bytes)
{
    // TODO(tomi): Maybe we do want to crash HERE!!! 
    if(array && data)
    {
        if(array->size == 0)
        {
            size_t memory_size = 5;
            size_t memory_size_in_bytes = (memory_size*size_of_data_in_bytes);
            array->memory = (u8 *)malloc(memory_size_in_bytes);
            array->size = memory_size;
            array->capacity = 0;
            array->memory_offset = 0;
        }
        else if(array->capacity >= array->size)
        {
            size_t memory_size = array->size*2;
            size_t memory_size_in_bytes = (memory_size*size_of_data_in_bytes);
            array->memory = (u8 *)realloc((u8 *)array->memory, memory_size_in_bytes);
            array->size = memory_size;
        }
        
        void *dest = array->memory + array->memory_offset;
        memcpy(dest, data, size_of_data_in_bytes);
        array->memory_offset += size_of_data_in_bytes;
        ++array->capacity;
     }
}

internal void
move_cursor_left(Editor_State *state)
{
    if(state->cursor.index <= 0) return;
    
    Cursor *cursor = &state->cursor;
    char c = state->file.memory[cursor->index];
    int cursor_width = state->font[c].width;
    cursor->pos_x -= cursor_width; 
    cursor->index--;
}

internal void
move_cursor_right(Editor_State *state)
{
    if(state->cursor.index-1 >= FILE_SIZE) return;
    
    Cursor *cursor = &state->cursor;
    char c = state->file.memory[cursor->index+1];
    int cursor_width = state->font[c].width;
    cursor->pos_x += cursor_width; 
    cursor->index++;
}

internal void
add_character(Editor_State *state, char character)
{
    Cursor *cursor = &state->cursor;
    File *file = &state->file;
    assert(cursor->index < FILE_SIZE);
    assert(cursor->last_index < FILE_SIZE);
    
    if(cursor->index <= cursor->last_index)
    {
        char *src_buffer = &file->memory[cursor->index];
        char *des_buffer = (src_buffer + 1);
        int count = cursor->last_index - cursor->index;
        if(count > 0) memmove(des_buffer, src_buffer, count);
    }
    file->memory[cursor->index] = character;
    cursor->index++;
    cursor->last_index++;
    
    // TODO(tomi): Maybe separate the logic from the render code
    // this get the width of the cursor into the screen
    if(character == '\n')
    {
        file->line_width[file->number_of_lines] = cursor->pos_x;
        file->number_of_lines++;
        cursor->pos_y += 20;
        cursor->pos_x = 0;
        cursor->line_number++;
    }
    else
    {
        cursor->pos_x += state->font[character].width;
    }
}

internal void
remove_character(Editor_State *state)
{
    Cursor *cursor = &state->cursor;
    File *file = &state->file;
    if(cursor->index > 0)
    {
        char *src_buffer = &file->memory[cursor->index];
        char *des_buffer = (src_buffer - 1);
        int count = cursor->last_index - cursor->index;
        
        if(*des_buffer == '\n')
        {
            file->number_of_lines--;
            cursor->pos_x = file->line_width[file->number_of_lines];
            cursor->pos_y -= 20;
        }
        else
        {
            cursor->pos_x -= state->font[*des_buffer].width;
        }

        if(count > 0) memmove(des_buffer, src_buffer, count);
        cursor->index--;
        cursor->last_index--;
    }
}

internal void
draw_bitmap(Backbuffer *buffer, Bitmap bitmap, int x, int y)
{
    
    int min_x = x;
    int min_y = y;
    int max_x = min_x + bitmap.width;
    int max_y = min_y + bitmap.height;
    
    // TODO:(tomi) Finish Test bitmap clipping
    int clip_x = 0;
    int clip_y = 0;

    if(min_x < 0)
    {
        clip_x = (-min_x);
        min_x = 0;
    }
    if(min_y < 0)
    {
        clip_y = (-min_y);
        min_y = 0;
    }
    if(max_x > buffer->width)
    {
        max_x = buffer->width;
    }
    if(max_y > buffer->height)
    {
        max_y = buffer->height;
    }

    int pitch = (buffer->width * buffer->bytes_per_pixel);
    u8 *des_row = (u8 *)buffer->memory + (min_y*pitch); 
    u8 *src_row = (u8 *)bitmap.memory + (clip_y * bitmap.pitch);
    for(int dy = min_y; dy < max_y; ++dy)
    {
        u32 *des_pixel = (u32 *)des_row + min_x;
        u32 *src_pixel = (u32 *)src_row + clip_x;
        for(int dx = min_x; dx < max_x; ++dx)
        {
            *des_pixel++ = *src_pixel++;
        }
        
        src_row += bitmap.pitch;
        des_row += pitch;
    }

}

internal void
draw_rect(Backbuffer *buffer, int x, int y, int width, int height, u32 color)
{
    int min_x = x;
    int min_y = y;
    int max_x = min_x + width;
    int max_y = min_y + height;
    
    if(min_x < 0)
    {
        min_x = 0;
    }
    if(min_y < 0)
    {
        min_y = 0;
    }
    if(max_x > buffer->width)
    {
        max_x = buffer->width;
    }
    if(max_y > buffer->height)
    {
        max_y = buffer->height;
    }

    int pitch = (buffer->width * buffer->bytes_per_pixel);
    u8 *des_row = (u8 *)buffer->memory + (min_y*pitch); 
    for(int dy = min_y; dy < max_y; ++dy)
    {
        u32 *des_pixel = (u32 *)des_row + min_x;
        for(int dx = min_x; dx < max_x; ++dx)
        {
            *des_pixel++ = color;
        }
        
        des_row += pitch;
    }
}

internal void
draw_file_buffer(Backbuffer *buffer, Bitmap *font, File *file, Cursor* cursor)
{
    int height = 0;
    int total_w = 0;
    for(int i = 0; i < cursor->last_index; ++i)
    {
        char c = file->memory[i];
        if(c == '\n')
        {
            height += 20;
            total_w = 0;
        }
        else
        {
            printf("%c, ", c);
            draw_bitmap(buffer, font[c], total_w, height);
            total_w += font[c].width;
        }

    }
}

internal void
draw_cursor(Backbuffer *buffer, Cursor *cursor)
{
   draw_rect(buffer, cursor->pos_x, cursor->pos_y, 6, 20, 0xFF00FF00);
}

void init_editor(Editor_State *state)
{
}

void draw_editor(Backbuffer *buffer, Editor_State *state)
{
    draw_file_buffer(buffer, state->font, &state->file, &state->cursor);
    draw_cursor(buffer, &state->cursor);
}
