#pragma once
#include <Windows.h>
#include <string>
#include <vector>
#include "Vectors.h"
#include "Offsets.h"
#include "Memory.h"
#include "Renderer.h"

struct PlayerInfo
{
    Vector3 position;
    Vector2 screenPosition;
    int health;
    int armor;
    int team;
    std::string name;
    bool isVisible;
    bool isDefusing;
    bool isScoped;
    bool isValid;
};

class CS2ESP
{
public:
    CS2ESP();
    ~CS2ESP();

    bool Initialize();
    void Render();
    void Shutdown();

    // Getters
    bool IsRunning() const { return m_Running; }
    HWND GetOverlayWindow() const { return m_OverlayWindow; }

private:
    bool CreateOverlayWindow();
    bool AttachToGame();
    bool ReadGameData();
    void ProcessPlayers();
    bool GetPlayerInfo(uintptr_t controller, PlayerInfo& info);
    bool WorldToScreen(const Vector3& position, Vector2& screen);
    Matrix4x4 ReadViewMatrix();
    std::string GetPlayerName(uintptr_t controller);
    uintptr_t GetLocalPlayerPawn();
    int GetLocalPlayerTeam();
    void DrawESP();
    void DrawPlayerBox(const PlayerInfo& player, bool isEnemy);

    // Helper function to find window by process ID
    static HWND FindMainWindowByProcessId(DWORD processId);

    // Window procedure
    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

    HWND m_GameWindow;
    HWND m_OverlayWindow;
    RECT m_GameRect;
    bool m_Running;

    uintptr_t m_LocalPlayerController;
    uintptr_t m_LocalPlayerPawn;
    int m_LocalPlayerTeam;
    std::vector<PlayerInfo> m_Players;
    Matrix4x4 m_ViewMatrix;
};

extern CS2ESP g_ESP;
