#pragma once

#include "aplatform.h"
#include "aengineconfig.h"
// leave this here, avoid auto sorting incorrect order, platform always comes first

// Window dimensions
#define DEFAULT_WINDOW_WIDTH  800
#define DEFAULT_WINDOW_HEIGHT 600

#ifdef ALMOND_USING_WINMAIN

// Max string length for title and class name
#define MAX_LOADSTRING 100

#include <shellapi.h>

namespace almondnamespace::core 
{

    // Forward declarations of Win32 functions
    ATOM RegisterWindowClass(HINSTANCE hInstance, LPCWSTR window_name);
    HWND InitWindowInstance(HINSTANCE hInstance, int nCmdShow, LPCWSTR szWindowClass, LPCWSTR szTitle, int32_t windowWidth, int32_t windowHeight);
    LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
    void PrintLastWin32Error(const wchar_t* lpszFunction);
    void ShowConsole();
}

#endif

namespace almondnamespace::core { void RunEngine(); }
