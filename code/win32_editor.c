#include <Windows.h>

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <memory.h>

#include "win32_editor.h"

// TODO(tomi): Do something with this globals
global_varible Win32_Backbuffer global_backbuffer;
global_varible File global_file;
global_varible Cursor global_cursor;

internal void
move_cursor_left(Cursor *cursor)
{
    if(cursor->index <= 0) return;
    cursor->index--;
}

internal void
move_cursor_right(Cursor *cursor)
{
    if(cursor->index >= FILE_SIZE) return;
    cursor->index++;
}

internal void
add_character(File *file, Cursor *cursor, char character)
{
    assert(cursor->index < FILE_SIZE);
    assert(cursor->last_index < FILE_SIZE);
    
    if(cursor->index <= cursor->last_index)
    {
        char *src_buffer = &file->memory[cursor->index];
        char *des_buffer = (src_buffer + 1);
        int count = cursor->last_index - cursor->index;
        // TODO(tomi): Use MoveMemory in win32 
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
    }
    else
    {
        cursor->pos_x += win32_letters[character].width;
    }
}

internal void
remove_character(File *file, Cursor *cursor)
{
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
            cursor->pos_x -= win32_letters[*des_buffer].width;
        }

        // TODO(tomi): Use MoveMemory in win32 
        if(count > 0) memmove(des_buffer, src_buffer, count);
        cursor->index--;
        cursor->last_index--;
    }
}

internal void
print_file_buffer(File *file, Cursor* cursor)
{
    printf("{ ");
    for(int i = 0; i < cursor->last_index; ++i)
    {
        char c = file->memory[i];
        if(c == '\n')
        {
            printf("\n");
        }
        else if(i == cursor->index)
        {
            printf("*%c, ", c);
        }
        else
        {
            printf("%c, ", c);
        }

    }
    printf(" }\n");
}

// ================== Start Win32 Code ========================

internal void
win32_resize_backbuffer(Win32_Backbuffer *buffer, int width, int height)
{
    if(buffer->memory)
    {
        VirtualFree(buffer->memory, 0, MEM_RELEASE);
    }

    buffer->width = width;
    buffer->height = height;
    buffer->bytes_per_pixel = 4;
    
    buffer->info.bmiHeader.biSize = sizeof(buffer->info.bmiHeader);
    buffer->info.bmiHeader.biWidth = width;
    buffer->info.bmiHeader.biHeight = -height;
    buffer->info.bmiHeader.biPlanes = 1;
    buffer->info.bmiHeader.biBitCount = 32;
    buffer->info.bmiHeader.biCompression = BI_RGB;

    SIZE_T backbuffer_size = (buffer->width*buffer->height)*buffer->bytes_per_pixel;
    buffer->memory = VirtualAlloc(0, backbuffer_size, MEM_COMMIT, PAGE_READWRITE); 
}

internal void
win32_update_backbuffer(Win32_Backbuffer *buffer, HDC device_context, int width, int height)
{
    StretchDIBits(device_context,
                  0, 0, width, height,
                  0, 0, buffer->width, buffer->height,
                  buffer->memory, &buffer->info, 
                  DIB_RGB_COLORS, SRCCOPY);
}

internal void
win32_clear_backbuffer(Win32_Backbuffer *buffer)
{
    int backbuffer_size = (buffer->width * buffer->height) * buffer->bytes_per_pixel;
    ZeroMemory(buffer->memory, backbuffer_size);
}

internal void
win32_draw_bitmap(Win32_Backbuffer *buffer, Win32_Bitmap bitmap, int x, int y)
{
    int pitch = (buffer->width * buffer->bytes_per_pixel);
    u8 *des_row = (u8 *)buffer->memory + (pitch*y);
    u8 *src_row = (u8 *)bitmap.memory;
    for(int y = 0; y < bitmap.height; ++y)
    {
        u32 *des_pixel = (u32 *)des_row + x;
        u32 *src_pixel = (u32 *)src_row;
        for(int x = 0; x < bitmap.width; ++x)
        {
            *des_pixel++ = *src_pixel++;
        }
        des_row += pitch;
        src_row += bitmap.pitch;
    }
}

internal void
win32_draw_rect(Win32_Backbuffer *buffer, int x, int y, int width, int height, u32 color)
{
    int pitch = (buffer->width * buffer->bytes_per_pixel);
    u8 *des_row = (u8 *)buffer->memory + (pitch*y);
    for(int y = 0; y < height; ++y)
    {
        u32 *des_pixel = (u32 *)des_row + x;
        for(int x = 0; x < width; ++x)
        {
            *des_pixel++ = color;
        }
        des_row += pitch;
    } 
}

internal Win32_Bitmap 
win32_create_letter_bitmap(HDC device_context, const char *letter)
{
    HBITMAP win32_bitmap = CreateCompatibleBitmap(device_context, 1024, 1024);
    SelectObject(device_context, win32_bitmap);
    BOOL result = TextOutA(device_context, 0, 0, letter, 1);
    TEXTMETRIC font_metric;
    GetTextMetrics(device_context, &font_metric);
    SIZE font_size;
    GetTextExtentPoint32A(device_context, letter, 1, &font_size);
    
    Win32_Bitmap font_bitmap = {0};
    font_bitmap.width = font_size.cx;
    font_bitmap.height = font_size.cy;
    font_bitmap.bytes_per_pixel = 4;
    font_bitmap.pitch = font_bitmap.bytes_per_pixel * font_bitmap.width;
    font_bitmap.memory = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
                                   font_bitmap.pitch*font_bitmap.height);
    
    u8 *row = (u8 *)font_bitmap.memory;
    for(int y = 0; y < font_bitmap.height; ++y)
    {
        u32 *pixel = (u32 *)row;
        for(int x = 0; x < font_bitmap.width; ++x)
        {
            u8 font_pixel = (u8)GetPixel(device_context, x, y);
            u8 inv_font_pixel = 255 - font_pixel;
            *pixel = (255 << 24) | (inv_font_pixel << 16) | 
                     (inv_font_pixel << 8) | (inv_font_pixel << 0);
            pixel++;
        }
        row += font_bitmap.pitch;
    }
    DeleteObject(win32_bitmap);    

    return font_bitmap;
}

internal void win32_load_complete_font(HDC device_context)
{
    for(int i = 0; i < 127; ++i)
    {
        win32_letters[i] = win32_create_letter_bitmap(device_context, &((char)i));
    }
}

internal void
win32_draw_file_buffer(Win32_Backbuffer *buffer, File *file, Cursor* cursor)
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
            win32_draw_bitmap(buffer, win32_letters[c], total_w, height);
            total_w += win32_letters[c].width;
        }

    }
}

internal void
win32_draw_cursor(Win32_Backbuffer *buffer, Cursor *cursor)
{
    win32_draw_rect(buffer, cursor->pos_x, cursor->pos_y, 6, 20, 0xFF00FF00);
}

LRESULT CALLBACK 
win32_window_proc(HWND window, UINT message, WPARAM w_param, LPARAM l_param)
{
    LRESULT result = 0;
    switch(message)
    {
        case WM_SIZE: 
        {
            RECT client_rect;
            GetClientRect(window, &client_rect);
            int width = client_rect.right - client_rect.left;
            int height = client_rect.bottom - client_rect.top;
            win32_resize_backbuffer(&global_backbuffer, width, height);
        }break;
        case WM_PAINT:
        {
            PAINTSTRUCT paint = {0};
            HDC device_context = BeginPaint(window, &paint);
            int width = paint.rcPaint.right - paint.rcPaint.left;
            int height = paint.rcPaint.bottom - paint.rcPaint.top;
            win32_clear_backbuffer(&global_backbuffer);
            win32_draw_file_buffer(&global_backbuffer, &global_file, &global_cursor);
            win32_draw_cursor(&global_backbuffer, &global_cursor);
            win32_update_backbuffer(&global_backbuffer, device_context, width, height);
            EndPaint(window, &paint);
        }break;
        case WM_CLOSE:
        {
            OutputDebugString("Close Message!\n");
            PostQuitMessage(0);
        }break;
        case WM_SYSKEYDOWN:
        case WM_SYSKEYUP:
        case WM_KEYDOWN:
        case WM_KEYUP:
        {
            u32 keycode = w_param;
            bool was_down = ((l_param & (1 << 30)) != 0);
            bool is_down = ((l_param & (1 << 31)) == 0);
            
            if(is_down)
            {
                RECT client_rect;
                GetClientRect(window, &client_rect);
                int width = client_rect.right - client_rect.left;
                int height = client_rect.bottom - client_rect.top;
                HDC device_context = GetDC(window);
                // TODO(tomi): Finish to add all keys and keys modifiables
                if(keycode == VK_BACK)
                {
                    remove_character(&global_file, &global_cursor);
                }
                else if(keycode == VK_RETURN)
                {
                    add_character(&global_file, &global_cursor, '\n');
                }
                else
                {
                    add_character(&global_file, &global_cursor, keycode);
                }
                win32_clear_backbuffer(&global_backbuffer);
                win32_draw_file_buffer(&global_backbuffer, &global_file, &global_cursor);
                win32_draw_cursor(&global_backbuffer, &global_cursor);
                win32_update_backbuffer(&global_backbuffer, device_context, width, height);
            }
        }break;
        default: 
        {
            result = DefWindowProc(window, message, w_param, l_param);
        }break;
    }
    return result;
}

int WINAPI 
wWinMain(HINSTANCE instance, HINSTANCE prev_instance, PWSTR cmd_line, int cmd_show)
{
    WNDCLASSEXA window_class = {0};
    window_class.cbSize = sizeof(WNDCLASSEXA);
    window_class.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
    window_class.lpfnWndProc = win32_window_proc;
    window_class.hInstance = instance;
    window_class.lpszClassName = "tomi_window_class";
    
    if(RegisterClassEx(&window_class))
    {
        HWND window = CreateWindowEx(0, window_class.lpszClassName,
                                     "Text Editor",
                                     WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                                     CW_USEDEFAULT, CW_USEDEFAULT,
                                     CW_USEDEFAULT, CW_USEDEFAULT,
                                     0, 0, instance, 0);
        if(window)
        {
            HDC device_context = GetDC(window);
            // TODO(tomi): Increase speed of font loading
            win32_load_complete_font(device_context);

            // TODO(tomi): Study best way of creating a message loop
            // for a text editor
            MSG message;
            for(;;)
            {
                BOOL res = res = GetMessage(&message, 0, 0, 0);
                if(res > 0)
                {
                    TranslateMessage(&message);
                    DispatchMessage(&message);
                }
                else
                {
                    break;
                }
            }

        }
        else
        {
            // TODO(tomi): Loggin error here!
        }
    }
    else
    {
        // TODO(tomi): Loggin error here!
    }

    return 0;
}
