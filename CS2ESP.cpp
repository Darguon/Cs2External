#include "CS2ESP.h"
#include <dwmapi.h>
#include <iostream>
#include <fstream>
#include <sstream>

#pragma comment(lib, "dwmapi.lib")

CS2ESP g_ESP;
std::ofstream g_DebugFile; // Debug log file

// Structure to share data with the callback
struct WindowSearchInfo
{
    DWORD processId;
    HWND window;
};

// Global variable for window search
WindowSearchInfo g_WindowInfo;

// Debug logging
void DebugLog(const std::string& message)
{
    if (!g_DebugFile.is_open())
    {
        g_DebugFile.open("cs2esp_debug.log", std::ios::app);
    }

    if (g_DebugFile.is_open())
    {
        g_DebugFile << message << std::endl;
        g_DebugFile.flush();
    }

    std::cout << message << std::endl;
}

// Debug vector output
std::string VecToString(const Vector3& vec)
{
    std::stringstream ss;
    ss << "(" << vec.x << ", " << vec.y << ", " << vec.z << ")";
    return ss.str();
}

// Static callback function for EnumWindows
static BOOL CALLBACK EnumWindowsProc(HWND hwnd, LPARAM lParam)
{
    DWORD windowProcessId = 0;
    GetWindowThreadProcessId(hwnd, &windowProcessId);

    if (windowProcessId == g_WindowInfo.processId)
    {
        // Check if this is a visible, non-child window
        if (IsWindowVisible(hwnd) && GetWindow(hwnd, GW_OWNER) == NULL)
        {
            // Get window title to verify it's not a launcher or auxiliary window
            wchar_t title[256] = { 0 };
            GetWindowTextW(hwnd, title, 255);

            // Log window found
            std::wstring wTitle(title);
            std::string sTitle(wTitle.begin(), wTitle.end());
            DebugLog("Found window with title: " + sTitle);

            // We want a window with a title and reasonable size
            RECT rect;
            if (GetClientRect(hwnd, &rect) &&
                rect.right > 100 && rect.bottom > 100 &&
                wcslen(title) > 0)
            {
                g_WindowInfo.window = hwnd;
                DebugLog("Selected window: " + sTitle + " with dimensions: " +
                    std::to_string(rect.right) + "x" + std::to_string(rect.bottom));
                return FALSE; // Stop enumeration
            }
        }
    }
    return TRUE; // Continue enumeration
}

CS2ESP::CS2ESP() :
    m_GameWindow(NULL),
    m_OverlayWindow(NULL),
    m_Running(false),
    m_LocalPlayerController(0),
    m_LocalPlayerPawn(0),
    m_LocalPlayerTeam(0)
{
    ZeroMemory(&m_GameRect, sizeof(m_GameRect));
    DebugLog("CS2ESP constructor called");
}

CS2ESP::~CS2ESP()
{
    Shutdown();
    DebugLog("CS2ESP destructor called");

    if (g_DebugFile.is_open())
    {
        g_DebugFile.close();
    }
}

bool CS2ESP::Initialize()
{
    DebugLog("Initializing CS2ESP...");

    // Initialize offsets
    if (!g_Offsets.Initialize())
    {
        DebugLog("Failed to initialize offsets");
        return false;
    }

    // Attach to the game
    if (!AttachToGame())
    {
        DebugLog("Failed to attach to CS2");
        return false;
    }

    // Create overlay window
    if (!CreateOverlayWindow())
    {
        DebugLog("Failed to create overlay window");
        return false;
    }

    // Initialize renderer
    if (!g_Renderer.Initialize(m_OverlayWindow))
    {
        DebugLog("Failed to initialize renderer");
        return false;
    }

    m_Running = true;
    DebugLog("CS2ESP initialization successful");
    return true;
}

HWND CS2ESP::FindMainWindowByProcessId(DWORD processId)
{
    DebugLog("Searching for window by process ID: " + std::to_string(processId));

    // Setup the search info
    g_WindowInfo.processId = processId;
    g_WindowInfo.window = NULL;

    // Enumerate all top-level windows
    EnumWindows(EnumWindowsProc, 0);

    if (g_WindowInfo.window)
        DebugLog("Window found successfully");
    else
        DebugLog("No suitable window found for process ID");

    return g_WindowInfo.window;
}

bool CS2ESP::AttachToGame()
{
    DebugLog("Attempting to attach to CS2 game...");

    // Find and attach to CS2 process   
    if (!g_Memory.Initialize(L"cs2.exe", L"client.dll"))
    {
        DebugLog("Failed to attach to CS2 process");
        return false;
    }

    // Find CS2 window - try multiple methods
    DebugLog("Searching for CS2 window...");

    // Method 1: Try standard window class and title
    m_GameWindow = FindWindow(L"Valve001", L"Counter-Strike 2");
    if (!m_GameWindow)
    {
        DebugLog("Standard window search failed, trying alternates...");

        // Method 2: Try alternative window titles
        m_GameWindow = FindWindow(L"Valve001", L"Counter-Strike");

        if (!m_GameWindow)
        {
            // Method 3: Try finding by process ID (most reliable)
            DebugLog("Searching by process ID...");
            m_GameWindow = FindMainWindowByProcessId(g_Memory.GetProcessId());

            if (!m_GameWindow)
            {
                DebugLog("Failed to find CS2 window using all methods");
                return false;
            }
        }
    }

    // Get window dimensions
    GetClientRect(m_GameWindow, &m_GameRect);
    if (m_GameRect.right <= 0 || m_GameRect.bottom <= 0)
    {
        DebugLog("Invalid window dimensions");
        return false;
    }

    DebugLog("Successfully attached to CS2");
    DebugLog("Window dimensions: " + std::to_string(m_GameRect.right) + "x" + std::to_string(m_GameRect.bottom));

    // Get window title for verification
    wchar_t title[256] = { 0 };
    GetWindowTextW(m_GameWindow, title, 255);
    std::wstring wTitle(title);
    std::string sTitle(wTitle.begin(), wTitle.end());
    DebugLog("Window title: " + sTitle);

    return true;
}

bool CS2ESP::CreateOverlayWindow()
{
    DebugLog("Creating overlay window...");

    // Register window class
    WNDCLASSEX wc = { 0 };
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.lpszClassName = L"CS2OverlayClass";

    if (!RegisterClassEx(&wc))
    {
        DebugLog("Failed to register window class");
        return false;
    }

    // Get window position
    POINT point = { 0 };
    ClientToScreen(m_GameWindow, &point);
    DebugLog("Game window position: " + std::to_string(point.x) + ", " + std::to_string(point.y));

    // Create the overlay window
    m_OverlayWindow = CreateWindowEx(
        WS_EX_TOPMOST | WS_EX_TRANSPARENT | WS_EX_LAYERED,
        L"CS2OverlayClass",
        L"CS2 ESP Overlay",
        WS_POPUP,
        point.x, point.y,
        m_GameRect.right, m_GameRect.bottom,
        NULL, NULL, GetModuleHandle(NULL), NULL);

    if (!m_OverlayWindow)
    {
        DebugLog("Failed to create overlay window, error code: " + std::to_string(GetLastError()));
        return false;
    }

    // Setup layered window attributes for transparency
    // NOTE: Comment this line to check if rendering works without transparency
    SetLayeredWindowAttributes(m_OverlayWindow, RGB(0, 0, 0), 0, LWA_COLORKEY);

    // Extend the window frame into the client area
    MARGINS margins = { -1, -1, -1, -1 };
    DwmExtendFrameIntoClientArea(m_OverlayWindow, &margins);

    // Show the window
    ShowWindow(m_OverlayWindow, SW_SHOW);
    UpdateWindow(m_OverlayWindow);

    DebugLog("Overlay window created successfully");
    return true;
}

LRESULT CALLBACK CS2ESP::WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    case WM_KEYDOWN:
        if (wParam == VK_END) // End key exits the application
            PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

void CS2ESP::Render()
{
    static int frameCount = 0;
    frameCount++;

    // Only log every 100 frames to avoid log spam
    bool shouldLog = (frameCount % 100 == 0);

    if (!m_Running)
        return;

    // Check if game window is still valid
    if (!IsWindow(m_GameWindow))
    {
        DebugLog("Game window no longer valid");
        m_Running = false;
        return;
    }

    // Update overlay position if game window moved
    RECT gameRect;
    POINT point = { 0 };
    GetClientRect(m_GameWindow, &gameRect);
    ClientToScreen(m_GameWindow, &point);

    if (gameRect.right != m_GameRect.right || gameRect.bottom != m_GameRect.bottom ||
        point.x != m_GameRect.left || point.y != m_GameRect.top)
    {
        if (shouldLog)
            DebugLog("Updating overlay position");

        m_GameRect = gameRect;
        SetWindowPos(m_OverlayWindow, HWND_TOPMOST, point.x, point.y,
            m_GameRect.right, m_GameRect.bottom, SWP_SHOWWINDOW);
    }

    // Read game data
    if (!ReadGameData())
    {
        if (shouldLog)
            DebugLog("Failed to read game data");
    }
    else
    {
        // Process players to get their information
        ProcessPlayers();

        if (shouldLog)
            DebugLog("Found " + std::to_string(m_Players.size()) + " players to draw");
    }

    // Begin rendering
    g_Renderer.BeginScene();

    // Draw ESP
    DrawESP();

    // End rendering
    g_Renderer.EndScene();
}

void CS2ESP::Shutdown()
{
    m_Running = false;
    DebugLog("Shutting down ESP...");

    // Clean up renderer
    g_Renderer.Cleanup();

    // Destroy overlay window
    if (m_OverlayWindow)
    {
        DestroyWindow(m_OverlayWindow);
        m_OverlayWindow = NULL;
    }

    // Unregister window class
    UnregisterClass(L"CS2OverlayClass", GetModuleHandle(NULL));

    DebugLog("ESP shutdown complete");
}

bool CS2ESP::ReadGameData()
{
    if (!g_Memory.IsValid())
        return false;

    // Read local player controller address
    uintptr_t moduleBase = g_Memory.GetModuleBase();
    m_LocalPlayerController = g_Memory.Read<uintptr_t>(moduleBase + g_Offsets.LocalPlayerController);
    if (!m_LocalPlayerController)
    {
        static bool loggedOnce = false;
        if (!loggedOnce) {
            DebugLog("Failed to read LocalPlayerController");
            loggedOnce = true;
        }
        return false;
    }

    // Read local player pawn
    m_LocalPlayerPawn = GetLocalPlayerPawn();
    if (!m_LocalPlayerPawn)
    {
        static bool loggedOnce = false;
        if (!loggedOnce) {
            DebugLog("Failed to read LocalPlayerPawn");
            loggedOnce = true;
        }
        return false;
    }

    // Read local player team
    m_LocalPlayerTeam = GetLocalPlayerTeam();

    // Read view matrix
    m_ViewMatrix = ReadViewMatrix();

    return true;
}

Matrix4x4 CS2ESP::ReadViewMatrix()
{
    uintptr_t moduleBase = g_Memory.GetModuleBase();
    return g_Memory.Read<Matrix4x4>(moduleBase + g_Offsets.Matrix);
}

uintptr_t CS2ESP::GetLocalPlayerPawn()
{
    if (!m_LocalPlayerController)
        return 0;

    // In CS2, the player pawn is found via a handle that needs to be resolved
    unsigned long pawnHandle = g_Memory.Read<unsigned long>(m_LocalPlayerController + g_Offsets.Entity.PlayerPawn);
    if (!pawnHandle)
    {
        DebugLog("Failed to read LocalPlayerController->PlayerPawn");
        return 0;
    }

    // Resolve the handle to get the actual pawn address
    uintptr_t entityList = g_Memory.GetModuleBase() + g_Offsets.EntityList;

    // Handle is not a pointer, needs to be converted
    uintptr_t listEntry = g_Memory.Read<uintptr_t>(entityList + 8 * ((pawnHandle & 0x7FFF) >> 9) + 16);
    if (!listEntry)
    {
        DebugLog("Failed to resolve pawn handle (listEntry)");
        return 0;
    }

    uintptr_t pawn = g_Memory.Read<uintptr_t>(listEntry + 8 * (pawnHandle & 0x1FF));

    if (pawn) {
        static bool loggedOnce = false;
        if (!loggedOnce) {
            DebugLog("Successfully resolved LocalPlayerPawn: 0x" + std::to_string(pawn));
            loggedOnce = true;
        }
    }

    return pawn;
}

int CS2ESP::GetLocalPlayerTeam()
{
    if (!m_LocalPlayerPawn)
        return 0;

    int team = g_Memory.Read<int>(m_LocalPlayerPawn + g_Offsets.Pawn.iTeamNum);
    static bool loggedOnce = false;
    if (!loggedOnce) {
        DebugLog("Local player team: " + std::to_string(team));
        loggedOnce = true;
    }
    return team;
}

void CS2ESP::ProcessPlayers()
{
    // Clear previous players list
    m_Players.clear();

    uintptr_t entityList = g_Memory.GetModuleBase() + g_Offsets.EntityList;
    int validPlayersFound = 0;
    int totalControllersChecked = 0;

    // Loop through possible player slots (typically 64 for CS2)
    for (int i = 0; i < 64; i++)
    {
        // Get controller at index i
        uintptr_t listEntry = g_Memory.Read<uintptr_t>(entityList + 8 * ((i & 0x7FFF) >> 9) + 16);
        if (!listEntry)
            continue;

        uintptr_t controller = g_Memory.Read<uintptr_t>(listEntry + 8 * (i & 0x1FF));
        if (!controller)
            continue;

        totalControllersChecked++;

        if (controller == m_LocalPlayerController)
            continue;

        // Read player information
        PlayerInfo playerInfo;
        if (GetPlayerInfo(controller, playerInfo))
        {
            // Only add valid players
            if (playerInfo.isValid)
            {
                m_Players.push_back(playerInfo);
                validPlayersFound++;
            }
        }
    }

    static int prevValidPlayers = -1;
    if (validPlayersFound != prevValidPlayers) {
        DebugLog("ProcessPlayers checked " + std::to_string(totalControllersChecked) +
            " controllers, found " + std::to_string(validPlayersFound) + " valid players");
        prevValidPlayers = validPlayersFound;
    }
}

bool CS2ESP::GetPlayerInfo(uintptr_t controller, PlayerInfo& info)
{
    // Initialize the player info
    info = {};
    info.isValid = false;

    // Check if player is alive
    bool isAlive = g_Memory.Read<bool>(controller + g_Offsets.Entity.IsAlive);
    if (!isAlive)
        return false;

    // Get player pawn handle
    unsigned long pawnHandle = g_Memory.Read<unsigned long>(controller + g_Offsets.Entity.PlayerPawn);
    if (!pawnHandle)
        return false;

    // Resolve the handle to get the actual pawn address
    uintptr_t entityList = g_Memory.GetModuleBase() + g_Offsets.EntityList;
    uintptr_t listEntry = g_Memory.Read<uintptr_t>(entityList + 8 * ((pawnHandle & 0x7FFF) >> 9) + 16);
    if (!listEntry)
        return false;

    uintptr_t pawn = g_Memory.Read<uintptr_t>(listEntry + 8 * (pawnHandle & 0x1FF));
    if (!pawn)
        return false;

    // Read player position
    info.position = g_Memory.Read<Vector3>(pawn + g_Offsets.Pawn.Pos);

    // Convert world position to screen position
    if (!WorldToScreen(info.position, info.screenPosition))
        return false;

    // Read health
    info.health = g_Memory.Read<int>(pawn + g_Offsets.Pawn.CurrentHealth);
    if (info.health <= 0 || info.health > 100)
        return false;

    // Read armor
    info.armor = g_Memory.Read<int>(pawn + g_Offsets.Pawn.CurrentArmor);

    // Read team
    info.team = g_Memory.Read<int>(pawn + g_Offsets.Pawn.iTeamNum);

    // Read player name
    info.name = GetPlayerName(controller);

    // Read additional states
    info.isScoped = g_Memory.Read<bool>(pawn + g_Offsets.Pawn.isScoped);
    info.isDefusing = g_Memory.Read<bool>(pawn + g_Offsets.Pawn.isDefusing);

    // Check if player is visible (could use spotted mask or other methods)
    // For simplicity, we're just assuming all players are visible
    info.isVisible = true;

    // Mark as valid
    info.isValid = true;

    return true;
}

std::string CS2ESP::GetPlayerName(uintptr_t controller)
{
    std::string name = "Player";

    // Read player name
    uintptr_t namePtr = controller + g_Offsets.Entity.iszPlayerName;
    if (namePtr)
    {
        std::string playerName;
        if (g_Memory.ReadString(namePtr, playerName, 32) && !playerName.empty())
            name = playerName;
    }

    return name;
}

bool CS2ESP::WorldToScreen(const Vector3& position, Vector2& screen)
{
    int screenWidth = m_GameRect.right - m_GameRect.left;
    int screenHeight = m_GameRect.bottom - m_GameRect.top;

    return g_Renderer.WorldToScreen(position, screen, m_ViewMatrix, screenWidth, screenHeight);
}

void CS2ESP::DrawESP()
{
    // Log ESP drawing info occasionally
    static int frameCount = 0;
    frameCount++;
    bool shouldLog = (frameCount % 100 == 0);

    if (shouldLog)
        DebugLog("Drawing ESP for " + std::to_string(m_Players.size()) + " players");

    // Draw debug info
    int screenWidth = m_GameRect.right - m_GameRect.left;
    int screenHeight = m_GameRect.bottom - m_GameRect.top;

    // Draw watermark
    g_Renderer.DrawFilledRect(10, 10, 200, 30, Color(0, 0, 0, 180));

    // Try to draw text (if implemented)
    g_Renderer.DrawText(20, 20, "CS2 ESP - " + std::to_string(m_Players.size()) + " players", Color::White());

    // Draw test rectangle to verify rendering is working
    g_Renderer.DrawFilledRect(screenWidth / 2 - 50, screenHeight / 2 - 50, 100, 100, Color(255, 0, 0, 128));
    g_Renderer.DrawRect(screenWidth / 2 - 60, screenHeight / 2 - 60, 120, 120, Color::Yellow(), 2.0f);

    // Draw players
    for (const auto& player : m_Players)
    {
        // Check if player is on screen
        if (player.screenPosition.x < 0 || player.screenPosition.x > m_GameRect.right ||
            player.screenPosition.y < 0 || player.screenPosition.y > m_GameRect.bottom)
            continue;

        // Determine if player is enemy or friendly
        bool isEnemy = (player.team != m_LocalPlayerTeam);

        if (shouldLog)
        {
            DebugLog("Drawing player at screen pos: (" +
                std::to_string(player.screenPosition.x) + ", " +
                std::to_string(player.screenPosition.y) + "), Team: " +
                std::to_string(player.team) + ", IsEnemy: " +
                std::to_string(isEnemy));
        }

        // Draw player ESP elements
        DrawPlayerBox(player, isEnemy);
    }
}

void CS2ESP::DrawPlayerBox(const PlayerInfo& player, bool isEnemy)
{
    // Calculate box dimensions based on distance
    float distance = m_LocalPlayerPawn ? Vector3().DistanceTo(player.position) / 100.0f : 10.0f;
    float boxHeight = 60.0f / distance;
    float boxWidth = 30.0f / distance;

    // Ensure minimum size
    boxHeight = max(boxHeight, 20.0f);
    boxWidth = max(boxWidth, 10.0f);

    // Calculate box coordinates
    float x = player.screenPosition.x - boxWidth / 2;
    float y = player.screenPosition.y - boxHeight;

    // Draw player box
    Color boxColor = isEnemy ? Color::Red() : Color::Green();

    // Draw straight lines instead of using helper function to test basic rendering
    g_Renderer.DrawLine(x, y, x + boxWidth, y, boxColor, 2.0f);                 // Top
    g_Renderer.DrawLine(x, y + boxHeight, x + boxWidth, y + boxHeight, boxColor, 2.0f); // Bottom
    g_Renderer.DrawLine(x, y, x, y + boxHeight, boxColor, 2.0f);                 // Left
    g_Renderer.DrawLine(x + boxWidth, y, x + boxWidth, y + boxHeight, boxColor, 2.0f);  // Right

    // Draw health bar (simplified for testing)
    float healthBarHeight = (boxHeight * player.health) / 100.0f;
    g_Renderer.DrawFilledRect(x - 8, y + boxHeight - healthBarHeight, 5, healthBarHeight, Color(0, 255, 0, 200));

    // Draw filled circle at player position to ensure it's visible
    g_Renderer.DrawFilledCircle(player.screenPosition.x, player.screenPosition.y, 5,
        isEnemy ? Color(255, 0, 0, 200) : Color(0, 255, 0, 200));
}