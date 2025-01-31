
#include "pch.h"

#include "aengine.h"
#include "acontextwindow.h"
#include "aversion.h"

#include <filesystem>
#include <iostream>

int width = 800;
int height = 600;

namespace almondnamespace::core {
    std::filesystem::path myexepath;

    void printEngineInfo() {
        std::cout << almondnamespace::GetEngineName() << " v" << almondnamespace::GetEngineVersion() << '\n';
    }

    void handleCommandLine(int argc, char* argv[]) {
        std::cout << "Command-line arguments for: ";
        for (int i = 0; i < argc; ++i) {
            if (i > 0) {
                std::cout << "  [" << i << "]: " << argv[i] << '\n';
            }
            else {
                myexepath = argv[i];
                std::string filename = myexepath.filename().string();
                std::cout << filename << '\n';
            }
        }
        std::cout << '\n'; // End arguments with a new line before printing anything else

        // Handle specific commands
        for (int i = 1; i < argc; ++i) {
            std::string arg = argv[i];

            if (arg == "--help" || arg == "-h") {
                std::cout << "Available commands:\n"
                    << "  --help, -h            Show this help message\n"
                    << "  --version, -v         Display the engine version\n"
                    << "  --fullscreen          Run the engine in fullscreen mode\n"
                    << "  --windowed            Run the engine in windowed mode\n"
                    << "  --width [value]       Set the window width (e.g., --width 1920)\n"
                    << "  --height [value]      Set the window height (e.g., --height 1080)\n";
            }
            else if (arg == "--version" || arg == "-v") {
                printEngineInfo();
            }
            else if (arg == "--fullscreen") {
                std::cout << "Running in fullscreen mode.\n";
                // Set fullscreen mode flag
            }
            else if (arg == "--windowed") {
                std::cout << "Running in windowed mode.\n";
                // Set windowed mode flag
            }
            else if (arg == "--width" && i + 1 < argc) {
                width = std::stoi(argv[++i]);
                std::cout << "Window width set to: " << width << '\n';
            }
            else if (arg == "--height" && i + 1 < argc) {
                height = std::stoi(argv[++i]);
                std::cout << "Window height set to: " << height << '\n';
            }
            else {
                std::cerr << "Unknown argument: " << arg << '\n';
            }
        }
    }

    static void StartEngine(int argc, char* argv[]) {
       // handleCommandLine(argc, argv);

        // Main engine loop
        RunEngine();
    }

#ifdef ALMOND_USING_WINMAIN

    // Define global variables here
    WCHAR szTitle[MAX_LOADSTRING] = L"EntryPoint Example";
    WCHAR szWindowClass[MAX_LOADSTRING] = L"SampleWindowClass";

    // Register window class function
    ATOM RegisterWindowClass(HINSTANCE hInstance, LPCWSTR window_name) {
        WNDCLASSW wc = {};
        wc.lpfnWndProc = WndProc;  // Window procedure function.
        wc.hInstance = hInstance;  // Instance handle.
        wc.lpszClassName = window_name;  // Window class name.
        wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
        wc.hIcon = static_cast<HICON>(LoadImageW(nullptr, L"icon.ico", IMAGE_ICON, 0, 0, LR_LOADFROMFILE));
        wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);

        ATOM result = RegisterClassW(&wc);
        if (!result) {
            PrintLastWin32Error(L"RegisterWindowClass");
        }
        return result;
    }

    // Window procedure function to handle messages
    LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
        switch (message) {
        case WM_DESTROY:
            PostQuitMessage(0);
            break;
        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
        }
        return 0;
    }

    // Print Win32 errors
    void PrintLastWin32Error(const wchar_t* lpszFunction) {
        DWORD error = GetLastError();
        if (error) {
            LPWSTR buffer = nullptr;
            FormatMessageW(
                FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                NULL, error, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                (LPWSTR)&buffer, 0, NULL);

            std::wcout << L"Error in " << lpszFunction << L": " << buffer;
            LocalFree(buffer);
        }
    }

    // Initialize and create the window instance
    HWND InitWindowInstance(HINSTANCE hInstance, int nCmdShow, LPCWSTR szWindowClass, LPCWSTR szTitle, int32_t windowWidth, int32_t windowHeight) {
        if (!RegisterWindowClass(hInstance, szWindowClass)) {
            return nullptr;
        }

        HWND window_handle = CreateWindowW(
            szWindowClass,
            szTitle,
            WS_OVERLAPPEDWINDOW,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            (windowWidth != 0) ? windowWidth : CW_USEDEFAULT,
            (windowHeight != 0) ? windowHeight : CW_USEDEFAULT,
            nullptr,
            nullptr,
            hInstance,
            nullptr);

        if (!window_handle) {
            PrintLastWin32Error(L"CreateWindowW");
            return nullptr;
        }

        ShowWindow(window_handle, nCmdShow);
        UpdateWindow(window_handle);
        return window_handle;
    }

    // Function to show the console in debug mode
    void ShowConsole() {
        AllocConsole();

        FILE* fp = nullptr;
        freopen_s(&fp, "CONIN$", "r", stdin);
        freopen_s(&fp, "CONOUT$", "w", stdout);
        freopen_s(&fp, "CONOUT$", "w", stderr);
    }
}

// Define wWinMain
int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR lpCmdLine,
    _In_ int nCmdShow) {
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

#ifdef _DEBUG
    almondnamespace::core::ShowConsole();
#endif

    try {
        // Command line processing
        int argc = __argc;
        char** argv = __argv;

        // Convert command line to wide string array (LPWSTR)
        int32_t numCmdLineArgs = 0;
        LPWSTR* pCommandLineArgvArray = CommandLineToArgvW(GetCommandLineW(), &numCmdLineArgs);

        if (pCommandLineArgvArray == nullptr) {
            MessageBoxW(NULL, L"Command line parsing failed!", L"Error", MB_ICONERROR | MB_OK);
            return -1;
        }

        // Free the argument array after use
        LocalFree(pCommandLineArgvArray);

        almondnamespace::core::handleCommandLine(argc, argv);

        LPCWSTR window_name = L"Almond Example Window";
        almondnamespace::core::RegisterWindowClass(hInstance, window_name);

        HWND window_handle = almondnamespace::core::InitWindowInstance(hInstance, nCmdShow, almondnamespace::core::szWindowClass, almondnamespace::core::szTitle, width, height);
        if (!window_handle) {
            return -1;
        }

        // Set the HWND globally so it can be accessed anywhere in the engine
        almondnamespace::contextwindow::WindowContext::setWindowHandle(window_handle);

        // std::cout << "Hello World!\n";
        almondnamespace::core::StartEngine(argc, argv);


        // Unfortunately this never gets run until after, nevermind we'll fix it later, for now we'll use glfw
        MSG msg = {};
        while (GetMessage(&msg, nullptr, 0, 0)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        return static_cast<int>(msg.wParam);

    }
    catch (const std::exception& ex) {
        MessageBoxA(nullptr, ex.what(), "Error", MB_ICONERROR | MB_OK);
        return -1;
    }

    return 0;
}

#endif

// Crossplatform Entry Point - the main entry point of the engine
int main(int argc, char* argv[]) {
    // Call the Windows entry point function (WinMain) using the command-line args
#ifdef ALMOND_USING_WINMAIN
        // Get the command line string
    LPWSTR pCommandLine = GetCommandLineW();
    // Call the Windows entry point function (wWinMain) using parsed command-line args
    int result = wWinMain(GetModuleHandle(NULL), NULL, pCommandLine, SW_SHOWNORMAL);

#else
    // Platform-independent logic (e.g., for non-Windows platforms)
    almondnamespace::core::handleCommandLine(argc, argv);
    almondnamespace::core::StartEngine(argc, argv);  // Replace with actual engine logic
#endif

    return 0;
}
