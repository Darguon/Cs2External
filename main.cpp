#include <Windows.h>
#include <iostream>
#include <fstream>
#include "CS2ESP.h"

// Debug version indicator
#define DEBUG_VERSION "1.0 Debug"

int main()
{
    // Console setup
    SetConsoleTitle(L"CS2 External ESP (Debug)");
    std::cout << "CS2 External ESP " << DEBUG_VERSION << " by DragonBurn" << std::endl;
    std::cout << "Debugging enabled - check cs2esp_debug.log for details" << std::endl;
    std::cout << "Press END key to exit" << std::endl;
    std::cout << "---------------------------" << std::endl;

    // Clear debug log at startup
    std::ofstream clearLog("cs2esp_debug.log", std::ios::trunc);
    if (clearLog.is_open())
    {
        clearLog << "CS2 External ESP Debug Log - Version " << DEBUG_VERSION << std::endl;
        clearLog << "------------------------------------------" << std::endl;
        clearLog.close();
    }

    // Initialize ESP
    if (!g_ESP.Initialize())
    {
        std::cerr << "Failed to initialize ESP. Make sure CS2 is running and you're running as Administrator." << std::endl;
        std::cout << "Press any key to exit..." << std::endl;
        std::cin.get();
        return 1;
    }

    std::cout << "ESP initialized successfully!" << std::endl;
    std::cout << "Debug mode - test shape should appear in center of screen" << std::endl;
    std::cout << "Overlay active - press END key to exit" << std::endl;

    // Main message loop
    MSG msg = { 0 };
    while (g_ESP.IsRunning() && msg.message != WM_QUIT)
    {
        // Process window messages
        if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        else
        {
            // Render ESP
            g_ESP.Render();

            // Small sleep to reduce CPU usage
            Sleep(5);
        }
    }

    // Shutdown ESP
    g_ESP.Shutdown();

    std::cout << "Application exited successfully." << std::endl;
    std::cout << "Press any key to close..." << std::endl;
    std::cin.get();

    return 0;
}