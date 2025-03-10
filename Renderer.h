#pragma once
#include <Windows.h>
#include <d3d11.h>
#include <DirectXMath.h>
#include <string>
#include <vector>  // Add missing include for std::vector
#include "Vectors.h"

#pragma comment(lib, "d3d11.lib")

struct Color
{
    float r, g, b, a;

    Color() : r(255.f), g(255.f), b(255.f), a(255.f) {}
    Color(float r, float g, float b, float a = 255.f) : r(r), g(g), b(b), a(a) {}

    static Color Red() { return Color(255, 0, 0); }
    static Color Green() { return Color(0, 255, 0); }
    static Color Blue() { return Color(0, 0, 255); }
    static Color Yellow() { return Color(255, 255, 0); }
    static Color Orange() { return Color(255, 165, 0); }
    static Color Purple() { return Color(128, 0, 128); }
    static Color White() { return Color(255, 255, 255); }
    static Color Black() { return Color(0, 0, 0); }
    static Color Gray() { return Color(128, 128, 128); }
};

struct VertexData
{
    DirectX::XMFLOAT3 position;
    DirectX::XMFLOAT4 color;
};

class Renderer
{
public:
    Renderer();
    ~Renderer();

    bool Initialize(HWND hWnd);
    void BeginScene();
    void EndScene();
    void Cleanup();

    void DrawLine(float x1, float y1, float x2, float y2, const Color& color, float thickness = 1.0f);
    void DrawRect(float x, float y, float width, float height, const Color& color, float thickness = 1.0f);
    void DrawFilledRect(float x, float y, float width, float height, const Color& color);
    void DrawCircle(float x, float y, float radius, const Color& color, int segments = 16, float thickness = 1.0f);
    void DrawFilledCircle(float x, float y, float radius, const Color& color, int segments = 16);
    void DrawText(float x, float y, const std::string& text, const Color& color, bool centered = false);

    // Helper functions for ESP rendering
    void DrawBoundingBox(float x, float y, float width, float height, const Color& color, float thickness = 1.0f);
    void DrawHealthBar(float x, float y, float width, float height, int health, int maxHealth, bool vertical = true);
    void DrawArmorBar(float x, float y, float width, float height, int armor, int maxArmor, bool vertical = true);
    void DrawOutlinedText(float x, float y, const std::string& text, const Color& textColor, const Color& outlineColor, bool centered = false);

    // World to screen conversion
    bool WorldToScreen(const Vector3& world, Vector2& screen, const Matrix4x4& viewMatrix, int screenWidth, int screenHeight);

    // Get window dimensions
    int GetScreenWidth() const { return m_ScreenWidth; }
    int GetScreenHeight() const { return m_ScreenHeight; }

private:
    bool InitializeD3D();
    bool InitializeShaders();
    bool InitializeBuffers();
    void RenderBufferedItems();

    HWND m_hWnd;
    int m_ScreenWidth;
    int m_ScreenHeight;

    // Direct3D resources
    ID3D11Device* m_pDevice;
    ID3D11DeviceContext* m_pDeviceContext;
    IDXGISwapChain* m_pSwapChain;
    ID3D11RenderTargetView* m_pRenderTargetView;
    ID3D11VertexShader* m_pVertexShader;
    ID3D11PixelShader* m_pPixelShader;
    ID3D11InputLayout* m_pInputLayout;
    ID3D11Buffer* m_pVertexBuffer;
    ID3D11BlendState* m_pBlendState;

    // Buffered geometry for batch rendering
    std::vector<VertexData> m_VertexBuffer;
};

extern Renderer g_Renderer;