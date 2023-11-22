#include <stdio.h>
#include <windows.h>

#include "cpu.h"

// Function prototype
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    // Redirect stdout to a file called stdout.txt in the current directory
    freopen("stdout.txt", "w", stdout);

    // Create the CPU
    cpu_t cpu;
    memset(&cpu, 0, sizeof(cpu_t));
    
    // Create the memory (heap allocated)
    cpu.memory = (char*)malloc(sizeof(memory_t));
    //cpu.memory_struct = (memory_t*)cpu.memory;


    const char* rom_file = "C:\\Users\\seanf\\Desktop\\Games\\GBA\\Pokemon - Fire Red.gba";
    const char* bios_file = "C:\\Users\\seanf\\Desktop\\Games\\GBA\\gba_bios.bin";

    // Read the BIOS file
    FILE* bios = fopen(bios_file, "rb");
    if (bios == NULL) {
        printf("Failed to open BIOS file\n");
        return 1;
    }

    // Copy the BIOS into memory
    fread(((memory_t*)cpu.memory)->bios, sizeof(((memory_t*)cpu.memory)->bios), 1, bios);   
    fclose(bios);

    // Read the ROM file
    FILE* rom = fopen(rom_file, "rb");
    if (rom == NULL) {
        printf("Failed to open ROM file\n");
        return 1;
    }

    // Copy the ROM into memory
    fread(((memory_t*)cpu.memory)->rom, sizeof(((memory_t*)cpu.memory)->rom), 1, rom);
    fclose(rom);

    // Run the CPU
    cpu_run(&cpu);

    return 0;

    // // Step 1: Register the window class
    // const char CLASS_NAME[] = "MyWindowClass";

    // WNDCLASS wc = {0};

    // wc.lpfnWndProc = WindowProc;
    // wc.hInstance = hInstance;
    // wc.lpszClassName = CLASS_NAME;

    // RegisterClass(&wc);

    // // Step 2: Create the window
    // HWND hwnd = CreateWindowEx(
    //     0,                              // Optional window styles
    //     CLASS_NAME,                     // Window class
    //     "My Window",                    // Window text
    //     WS_OVERLAPPEDWINDOW,            // Window style

    //     // Size and position
    //     CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,

    //     NULL,                           // Parent window
    //     NULL,                           // Menu
    //     hInstance,                      // Instance handle
    //     NULL                            // Additional application data
    // );

    // if (hwnd == NULL) {
    //     return 0;
    // }

    // // Step 3: Show the window
    // ShowWindow(hwnd, nCmdShow);

    // // Step 4: Run the message loop
    // MSG msg = {0};

    // while (GetMessage(&msg, NULL, 0, 0)) {
    //     TranslateMessage(&msg);
    //     DispatchMessage(&msg);
    // }

    // return msg.wParam;
}

// Step 5: Window procedure
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg) {
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;

    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);

        FillRect(hdc, &ps.rcPaint, (HBRUSH)(COLOR_WINDOW + 1));

        EndPaint(hwnd, &ps);
    }
        return 0;
    }

    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}
