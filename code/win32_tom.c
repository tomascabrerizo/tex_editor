#include <Windows.h>

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <memory.h>

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

#define FILE_SIZE (10 * 1024)

// TODO(tomi): This should be a pointer or and index on the array
// for now because the array is fixed size i'll use index
// TODO(tomi): Do something with this globals
global_varible u32 cursor_index; 
global_varible u32 last_index;
global_varible char file_buffer[FILE_SIZE];

internal void
move_cursor_left()
{
    if(cursor_index <= 0) return;
    cursor_index--;
}

internal void
move_cursor_right()
{
    if(cursor_index >= FILE_SIZE) return;
    cursor_index++;
}

internal void
add_character(char character)
{
    assert(cursor_index < FILE_SIZE);
    assert(last_index < FILE_SIZE);
    
    if(cursor_index <= last_index)
    {
        char *src_buffer = &file_buffer[cursor_index];
        char *des_buffer = (src_buffer + 1);
        int count = last_index - cursor_index;
        // TODO(tomi): Use MoveMemory in win32 
        if(count > 0) memmove(des_buffer, src_buffer, count);
    }
    file_buffer[cursor_index] = character;
    cursor_index++;
    last_index++;
}

internal void
remove_character()
{
    if(cursor_index >= 1)
    {
        char *src_buffer = &file_buffer[cursor_index];
        char *des_buffer = (src_buffer - 1);
        int count = last_index - cursor_index;
        // TODO(tomi): Use MoveMemory in win32 
        if(count > 0) memmove(des_buffer, src_buffer, count);
        cursor_index--;
        last_index--;
    }
}

internal void
print_file_buffer()
{
    printf("{ ");
    for(int i = 0; i < last_index; ++i)
    {
        char c = file_buffer[i];
        if(c == '\n')
        {
            printf("\n");
        }
        else if(i == cursor_index)
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

// TODO(tomi): Do something with this globals
global_varible int bytes_per_pixel = 4;
global_varible void* win32_backbuffer;
global_varible BITMAPINFO win32_bitmapinfo;
global_varible int backbuffer_width;
global_varible int backbuffer_height;

typedef struct
{
    int width;
    int height;
    int pitch;
    int bytes_per_pixel;
    void* buffer;
} Win32_Bitmap;

global_varible Win32_Bitmap win32_letters[256];

internal void
win32_resize_backbuffer(int width, int height)
{
    if(win32_backbuffer)
    {
        VirtualFree(win32_backbuffer, 0, MEM_RELEASE);
    }

    backbuffer_width = width;
    backbuffer_height = height;
    
    win32_bitmapinfo.bmiHeader.biSize = sizeof(win32_bitmapinfo.bmiHeader);
    win32_bitmapinfo.bmiHeader.biWidth = width;
    win32_bitmapinfo.bmiHeader.biHeight = -height;
    win32_bitmapinfo.bmiHeader.biPlanes = 1;
    win32_bitmapinfo.bmiHeader.biBitCount = 32;
    win32_bitmapinfo.bmiHeader.biCompression = BI_RGB;

    SIZE_T backbuffer_size = (width*height)*bytes_per_pixel;
    win32_backbuffer = VirtualAlloc(0, backbuffer_size, MEM_COMMIT, PAGE_READWRITE); 
}

internal void
win32_update_backbuffer(HDC device_context, int width, int height)
{
    StretchDIBits(device_context,
                  0, 0, width, height,
                  0, 0, backbuffer_width, backbuffer_height,
                  win32_backbuffer, &win32_bitmapinfo, 
                  DIB_RGB_COLORS, SRCCOPY);
}

internal void
win32_draw_bitmap(Win32_Bitmap bitmap, int x, int y)
{
    int pitch = (backbuffer_width * bytes_per_pixel);
    u8 *des_row = (u8 *)win32_backbuffer + (pitch*y);
    u8 *src_row = (u8 *)bitmap.buffer;
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
    font_bitmap.buffer = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
                                   font_bitmap.pitch*font_bitmap.height);

    u8 *row = (u8 *)font_bitmap.buffer;
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
win32_draw_file_buffer()
{
    int height = 0;
    int total_w = 0;
    for(int i = 0; i < last_index; ++i)
    {
        char c = file_buffer[i];
        if(c == '\n')
        {
            height += 20;
            total_w = 0;
        }
        else
        {
            printf("%c, ", c);
            win32_draw_bitmap(win32_letters[c], total_w, height);
            total_w += win32_letters[c].width;
        }

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
            win32_resize_backbuffer(width, height);
        }break;
        case WM_PAINT:
        {
            PAINTSTRUCT paint = {0};
            HDC device_context = BeginPaint(window, &paint);
            int width = paint.rcPaint.right - paint.rcPaint.left;
            int height = paint.rcPaint.bottom - paint.rcPaint.top;
            win32_draw_file_buffer();
            win32_update_backbuffer(device_context, width, height);
            EndPaint(window, &paint);
        }break;
        case WM_CLOSE:
        {
            OutputDebugString("Close Message!\n");
            PostQuitMessage(0);
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
            
            add_character('T');
            add_character('o');
            add_character('m');
            add_character('i');
            add_character(' ');
            add_character('C');
            add_character('a');
            add_character('b');
            add_character('r');
            add_character('e');
            add_character('r');
            add_character('i');
            add_character('z');
            add_character('o');
            add_character('\n');
            add_character('M');
            add_character('a');
            add_character('n');
            add_character('u');
            add_character(' ');
            add_character('C');
            add_character('a');
            add_character('b');
            add_character('r');
            add_character('e');
            add_character('r');
            add_character('i');
            add_character('z');
            add_character('o');
            add_character('\n');

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
