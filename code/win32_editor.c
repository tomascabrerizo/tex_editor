#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <memory.h>

#include "editor.h"
#include "editor.c"

#include <Windows.h>
#include "win32_editor.h"

// TODO(tomi): Do something with this globals
global_varible Win32_Backbuffer global_backbuffer;

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

internal Bitmap 
win32_create_letter_bitmap(HDC device_context, const char *letter)
{
    HBITMAP win32_bitmap = CreateCompatibleBitmap(device_context, 1024, 1024);
    SelectObject(device_context, win32_bitmap);
    BOOL result = TextOutA(device_context, 0, 0, letter, 1);
    TEXTMETRIC font_metric;
    GetTextMetrics(device_context, &font_metric);
    SIZE font_size;
    GetTextExtentPoint32A(device_context, letter, 1, &font_size);
    
    Bitmap font_bitmap = {0};
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

internal void win32_load_complete_font(HDC device_context, Editor_State *state)
{
    for(int i = 0; i < 127; ++i)
    {
        state->font[i] = win32_create_letter_bitmap(device_context, &((char)i));
    }
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
            
            Backbuffer backbuffer = {0};
            backbuffer.width = global_backbuffer.width;
            backbuffer.height = global_backbuffer.height;
            backbuffer.bytes_per_pixel = global_backbuffer.bytes_per_pixel;
            backbuffer.memory = global_backbuffer.memory;
            draw_editor(&backbuffer, &global_state);
            
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
            bool is_shift = ((GetKeyState(VK_LSHIFT) >> 15) != 0);
            
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
                    remove_character(&global_state);
                }
                else if(keycode == VK_SPACE)
                {
                    add_character(&global_state, ' ');
                }
                else if(keycode == VK_RETURN)
                {
                    add_character(&global_state, '\n');
                }
                else if(keycode == VK_TAB)
                {
                    add_character(&global_state, ' ');
                    add_character(&global_state, ' ');
                    add_character(&global_state, ' ');
                    add_character(&global_state, ' ');
                }
                else if(keycode == VK_LEFT)
                {
                    move_cursor_left(&global_state);
                }
                else if(keycode == VK_RIGHT)
                {
                    move_cursor_right(&global_state);
                }
                else if(keycode == VK_SHIFT)
                {
                    break;
                }
                else
                {
                    if(!is_shift & keycode >= 65 && keycode <= 90) keycode += 32;
                    add_character(&global_state, (keycode));
                }
                win32_clear_backbuffer(&global_backbuffer);

                Backbuffer backbuffer = {0};
                backbuffer.width = global_backbuffer.width;
                backbuffer.height = global_backbuffer.height;
                backbuffer.bytes_per_pixel = global_backbuffer.bytes_per_pixel;
                backbuffer.memory = global_backbuffer.memory;
                draw_editor(&backbuffer, &global_state);

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

// NOTE(tomi): Simple test entry point
int main(int argc, char** argv)
{
    // NOTE(tomi): Dynamic array testing 
    D_Array line_array;
    Line line0 = { 1 };
    PUSH_LINE(&line_array, &line0);
    Line line1 = { 2 };
    PUSH_LINE(&line_array, &line1);
    Line line2 = { 3 };
    PUSH_LINE(&line_array, &line2);
    Line line3 = { 4 };
    PUSH_LINE(&line_array, &line3);
    Line line4 = { 5 };
    PUSH_LINE(&line_array, &line4);
    Line line5 = { 6 };
    PUSH_LINE(&line_array, &line5);
    
    for(int i = 0; i < line_array.capacity; ++i)
    {
        Line *line = (Line *)line_array.memory + i;
        printf("Line number: %d!\n", line->number);
    }

    system("pause");
    return 0;
}

int WINAPI 
wWinMain_(HINSTANCE instance, HINSTANCE prev_instance, PWSTR cmd_line, int cmd_show)
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
            win32_load_complete_font(device_context, &global_state);
            
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

                // NOTE(tomi): Bitmap Clipping test
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
