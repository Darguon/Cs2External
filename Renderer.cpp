#include "Renderer.h"
#include <d3dcompiler.h>
#include <vector>
#include <cmath>

#pragma comment(lib, "d3dcompiler.lib")

Renderer g_Renderer;

// Simple vertex shader
const char* VertexShaderCode = R"(
struct VS_INPUT
{
    float3 Pos : POSITION;
    float4 Col : COLOR;
};

struct PS_INPUT
{
    float4 Pos : SV_POSITION;
    float4 Col : COLOR;
};

PS_INPUT VS(VS_INPUT input)
{
    PS_INPUT output;
    output.Pos = float4(input.Pos.xy, 0.f, 1.0f);
    output.Col = input.Col;
    return output;
}
)";

// Simple pixel shader
const char* PixelShaderCode = R"(
struct PS_INPUT
{
    float4 Pos : SV_POSITION;
    float4 Col : COLOR;
};

float4 PS(PS_INPUT input) : SV_Target
{
    return input.Col;
}
)";

Renderer::Renderer() :
    m_hWnd(NULL),
    m_ScreenWidth(0),
    m_ScreenHeight(0),
    m_pDevice(nullptr),
    m_pDeviceContext(nullptr),
    m_pSwapChain(nullptr),
    m_pRenderTargetView(nullptr),
    m_pVertexShader(nullptr),
    m_pPixelShader(nullptr),
    m_pInputLayout(nullptr),
    m_pVertexBuffer(nullptr),
    m_pBlendState(nullptr)
{
}

Renderer::~Renderer()
{
    Cleanup();
}

bool Renderer::Initialize(HWND hWnd)
{
    m_hWnd = hWnd;

    // Get window dimensions
    RECT rect;
    GetClientRect(hWnd, &rect);
    m_ScreenWidth = rect.right - rect.left;
    m_ScreenHeight = rect.bottom - rect.top;

    if (!InitializeD3D())
        return false;

    if (!InitializeShaders())
        return false;

    if (!InitializeBuffers())
        return false;

    return true;
}

void Renderer::BeginScene()
{
    // Clear the render target
    float clearColor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
    m_pDeviceContext->ClearRenderTargetView(m_pRenderTargetView, clearColor);

    // Set up the pipeline
    m_pDeviceContext->IASetInputLayout(m_pInputLayout);
    m_pDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    m_pDeviceContext->VSSetShader(m_pVertexShader, nullptr, 0);
    m_pDeviceContext->PSSetShader(m_pPixelShader, nullptr, 0);

    // Clear vertex buffer for new frame
    m_VertexBuffer.clear();
}

void Renderer::EndScene()
{
    // Render any buffered geometry
    RenderBufferedItems();

    // Present the frame
    m_pSwapChain->Present(1, 0);
}

void Renderer::Cleanup()
{
    // Release DirectX resources
    if (m_pBlendState) m_pBlendState->Release();
    if (m_pVertexBuffer) m_pVertexBuffer->Release();
    if (m_pInputLayout) m_pInputLayout->Release();
    if (m_pPixelShader) m_pPixelShader->Release();
    if (m_pVertexShader) m_pVertexShader->Release();
    if (m_pRenderTargetView) m_pRenderTargetView->Release();
    if (m_pSwapChain) m_pSwapChain->Release();
    if (m_pDeviceContext) m_pDeviceContext->Release();
    if (m_pDevice) m_pDevice->Release();

    m_pBlendState = nullptr;
    m_pVertexBuffer = nullptr;
    m_pInputLayout = nullptr;
    m_pPixelShader = nullptr;
    m_pVertexShader = nullptr;
    m_pRenderTargetView = nullptr;
    m_pSwapChain = nullptr;
    m_pDeviceContext = nullptr;
    m_pDevice = nullptr;
}

void Renderer::RenderBufferedItems()
{
    if (m_VertexBuffer.empty())
        return;

    // Map the vertex buffer
    D3D11_MAPPED_SUBRESOURCE mappedResource;
    HRESULT hr = m_pDeviceContext->Map(m_pVertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
    if (FAILED(hr))
        return;

    // Copy vertices to the buffer
    VertexData* pData = (VertexData*)mappedResource.pData;
    memcpy(pData, m_VertexBuffer.data(), m_VertexBuffer.size() * sizeof(VertexData));
    m_pDeviceContext->Unmap(m_pVertexBuffer, 0);

    // Set vertex buffer
    UINT stride = sizeof(VertexData);
    UINT offset = 0;
    m_pDeviceContext->IASetVertexBuffers(0, 1, &m_pVertexBuffer, &stride, &offset);

    // Draw vertices
    m_pDeviceContext->Draw((UINT)m_VertexBuffer.size(), 0);

    // Clear the vertex buffer for the next batch
    m_VertexBuffer.clear();
}

bool Renderer::InitializeD3D()
{
    // Setup swap chain
    DXGI_SWAP_CHAIN_DESC swapChainDesc = { 0 };
    swapChainDesc.BufferCount = 1;
    swapChainDesc.BufferDesc.Width = m_ScreenWidth;
    swapChainDesc.BufferDesc.Height = m_ScreenHeight;
    swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.OutputWindow = m_hWnd;
    swapChainDesc.SampleDesc.Count = 1;
    swapChainDesc.SampleDesc.Quality = 0;
    swapChainDesc.Windowed = TRUE;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    // Create device and swap chain
    HRESULT hr = D3D11CreateDeviceAndSwapChain(
        nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0, nullptr, 0,
        D3D11_SDK_VERSION, &swapChainDesc, &m_pSwapChain, &m_pDevice, nullptr, &m_pDeviceContext);

    if (FAILED(hr))
        return false;

    // Create render target view
    ID3D11Texture2D* pBackBuffer = nullptr;
    hr = m_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&pBackBuffer);
    if (FAILED(hr))
        return false;

    hr = m_pDevice->CreateRenderTargetView(pBackBuffer, nullptr, &m_pRenderTargetView);
    pBackBuffer->Release();
    if (FAILED(hr))
        return false;

    m_pDeviceContext->OMSetRenderTargets(1, &m_pRenderTargetView, nullptr);

    // Setup viewport
    D3D11_VIEWPORT viewport = { 0 };
    viewport.Width = (float)m_ScreenWidth;
    viewport.Height = (float)m_ScreenHeight;
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;
    m_pDeviceContext->RSSetViewports(1, &viewport);

    // Create blend state for alpha blending
    D3D11_BLEND_DESC blendDesc = { 0 };
    blendDesc.AlphaToCoverageEnable = FALSE;
    blendDesc.IndependentBlendEnable = FALSE;
    blendDesc.RenderTarget[0].BlendEnable = TRUE;
    blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
    blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
    blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
    blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
    blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

    hr = m_pDevice->CreateBlendState(&blendDesc, &m_pBlendState);
    if (FAILED(hr))
        return false;

    // Set blend state
    float blendFactor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
    m_pDeviceContext->OMSetBlendState(m_pBlendState, blendFactor, 0xffffffff);

    return true;
}

bool Renderer::InitializeShaders()
{
    // Compile vertex shader
    ID3DBlob* pVSBlob = nullptr;
    ID3DBlob* pErrorBlob = nullptr;

    HRESULT hr = D3DCompile(VertexShaderCode, strlen(VertexShaderCode), nullptr, nullptr, nullptr,
        "VS", "vs_4_0", 0, 0, &pVSBlob, &pErrorBlob);

    if (FAILED(hr))
    {
        if (pErrorBlob)
        {
            OutputDebugStringA((char*)pErrorBlob->GetBufferPointer());
            pErrorBlob->Release();
        }
        return false;
    }

    // Create vertex shader
    hr = m_pDevice->CreateVertexShader(pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), nullptr, &m_pVertexShader);
    if (FAILED(hr))
    {
        pVSBlob->Release();
        return false;
    }

    // Define input layout
    D3D11_INPUT_ELEMENT_DESC layout[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 }
    };

    // Create input layout
    hr = m_pDevice->CreateInputLayout(layout, 2, pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), &m_pInputLayout);
    pVSBlob->Release();
    if (FAILED(hr))
        return false;

    // Compile pixel shader
    ID3DBlob* pPSBlob = nullptr;
    hr = D3DCompile(PixelShaderCode, strlen(PixelShaderCode), nullptr, nullptr, nullptr,
        "PS", "ps_4_0", 0, 0, &pPSBlob, &pErrorBlob);

    if (FAILED(hr))
    {
        if (pErrorBlob)
        {
            OutputDebugStringA((char*)pErrorBlob->GetBufferPointer());
            pErrorBlob->Release();
        }
        return false;
    }

    // Create pixel shader
    hr = m_pDevice->CreatePixelShader(pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), nullptr, &m_pPixelShader);
    pPSBlob->Release();
    if (FAILED(hr))
        return false;

    return true;
}

bool Renderer::InitializeBuffers()
{
    // Create vertex buffer
    D3D11_BUFFER_DESC bd = {};
    bd.Usage = D3D11_USAGE_DYNAMIC;
    bd.ByteWidth = sizeof(VertexData) * 5000; // Buffer for up to 5000 vertices
    bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

    HRESULT hr = m_pDevice->CreateBuffer(&bd, nullptr, &m_pVertexBuffer);
    if (FAILED(hr))
        return false;

    // Reserve space for vertices
    m_VertexBuffer.reserve(5000);

    return true;
}

void Renderer::DrawLine(float x1, float y1, float x2, float y2, const Color& color, float thickness)
{
    // Convert screen coordinates to normalized device coordinates
    float ndcX1 = 2.0f * x1 / m_ScreenWidth - 1.0f;
    float ndcY1 = 1.0f - 2.0f * y1 / m_ScreenHeight;
    float ndcX2 = 2.0f * x2 / m_ScreenWidth - 1.0f;
    float ndcY2 = 1.0f - 2.0f * y2 / m_ScreenHeight;

    // Calculate the line direction
    float dirX = ndcX2 - ndcX1;
    float dirY = ndcY2 - ndcY1;
    float length = sqrt(dirX * dirX + dirY * dirY);

    if (length < 0.00001f)
        return;

    // Normalize the direction
    dirX /= length;
    dirY /= length;

    // Calculate the perpendicular direction
    float perpX = -dirY;
    float perpY = dirX;

    // Calculate half thickness in NDC space
    float halfThicknessX = perpX * (thickness / m_ScreenWidth);
    float halfThicknessY = perpY * (thickness / m_ScreenHeight);

    // Create the vertices for a quad (two triangles)
    VertexData v[6];

    // Vertex colors
    DirectX::XMFLOAT4 vertexColor = { color.r / 255.0f, color.g / 255.0f, color.b / 255.0f, color.a / 255.0f };

    // First triangle
    v[0].position = DirectX::XMFLOAT3(ndcX1 + halfThicknessX, ndcY1 + halfThicknessY, 0.0f);
    v[0].color = vertexColor;

    v[1].position = DirectX::XMFLOAT3(ndcX1 - halfThicknessX, ndcY1 - halfThicknessY, 0.0f);
    v[1].color = vertexColor;

    v[2].position = DirectX::XMFLOAT3(ndcX2 + halfThicknessX, ndcY2 + halfThicknessY, 0.0f);
    v[2].color = vertexColor;

    // Second triangle
    v[3].position = DirectX::XMFLOAT3(ndcX2 + halfThicknessX, ndcY2 + halfThicknessY, 0.0f);
    v[3].color = vertexColor;

    v[4].position = DirectX::XMFLOAT3(ndcX1 - halfThicknessX, ndcY1 - halfThicknessY, 0.0f);
    v[4].color = vertexColor;

    v[5].position = DirectX::XMFLOAT3(ndcX2 - halfThicknessX, ndcY2 - halfThicknessY, 0.0f);
    v[5].color = vertexColor;

    // Add vertices to the buffer
    for (int i = 0; i < 6; i++)
        m_VertexBuffer.push_back(v[i]);
}

void Renderer::DrawRect(float x, float y, float width, float height, const Color& color, float thickness)
{
    // Draw the four sides of the rectangle
    DrawLine(x, y, x + width, y, color, thickness);                 // Top
    DrawLine(x, y + height, x + width, y + height, color, thickness); // Bottom
    DrawLine(x, y, x, y + height, color, thickness);                 // Left
    DrawLine(x + width, y, x + width, y + height, color, thickness);  // Right
}

void Renderer::DrawFilledRect(float x, float y, float width, float height, const Color& color)
{
    // Convert to NDC coordinates
    float ndcX1 = 2.0f * x / m_ScreenWidth - 1.0f;
    float ndcY1 = 1.0f - 2.0f * y / m_ScreenHeight;
    float ndcX2 = 2.0f * (x + width) / m_ScreenWidth - 1.0f;
    float ndcY2 = 1.0f - 2.0f * (y + height) / m_ScreenHeight;

    // Vertex colors
    DirectX::XMFLOAT4 vertexColor = { color.r / 255.0f, color.g / 255.0f, color.b / 255.0f, color.a / 255.0f };

    // Create the vertices for a quad (two triangles)
    VertexData v[6];

    // First triangle
    v[0].position = DirectX::XMFLOAT3(ndcX1, ndcY1, 0.0f);
    v[0].color = vertexColor;

    v[1].position = DirectX::XMFLOAT3(ndcX1, ndcY2, 0.0f);
    v[1].color = vertexColor;

    v[2].position = DirectX::XMFLOAT3(ndcX2, ndcY1, 0.0f);
    v[2].color = vertexColor;

    // Second triangle
    v[3].position = DirectX::XMFLOAT3(ndcX2, ndcY1, 0.0f);
    v[3].color = vertexColor;

    v[4].position = DirectX::XMFLOAT3(ndcX1, ndcY2, 0.0f);
    v[4].color = vertexColor;

    v[5].position = DirectX::XMFLOAT3(ndcX2, ndcY2, 0.0f);
    v[5].color = vertexColor;

    // Add vertices to the buffer
    for (int i = 0; i < 6; i++)
        m_VertexBuffer.push_back(v[i]);
}

void Renderer::DrawCircle(float x, float y, float radius, const Color& color, int segments, float thickness)
{
    float angleStep = 2.0f * 3.14159f / segments;

    for (int i = 0; i < segments; i++)
    {
        float angle1 = i * angleStep;
        float angle2 = (i + 1) * angleStep;

        float x1 = x + radius * cos(angle1);
        float y1 = y + radius * sin(angle1);
        float x2 = x + radius * cos(angle2);
        float y2 = y + radius * sin(angle2);

        DrawLine(x1, y1, x2, y2, color, thickness);
    }
}

void Renderer::DrawFilledCircle(float x, float y, float radius, const Color& color, int segments)
{
    float angleStep = 2.0f * 3.14159f / segments;

    // Convert center to NDC
    float ndcX = 2.0f * x / m_ScreenWidth - 1.0f;
    float ndcY = 1.0f - 2.0f * y / m_ScreenHeight;

    // Convert radius to NDC (use average of width and height)
    float ndcRadius = radius * 2.0f / (m_ScreenWidth + m_ScreenHeight);

    // Vertex colors
    DirectX::XMFLOAT4 vertexColor = { color.r / 255.0f, color.g / 255.0f, color.b / 255.0f, color.a / 255.0f };

    for (int i = 0; i < segments; i++)
    {
        float angle1 = i * angleStep;
        float angle2 = (i + 1) * angleStep;

        // Create a triangle
        VertexData v[3];

        v[0].position = DirectX::XMFLOAT3(ndcX, ndcY, 0.0f);
        v[0].color = vertexColor;

        v[1].position = DirectX::XMFLOAT3(ndcX + ndcRadius * cos(angle1), ndcY - ndcRadius * sin(angle1), 0.0f);
        v[1].color = vertexColor;

        v[2].position = DirectX::XMFLOAT3(ndcX + ndcRadius * cos(angle2), ndcY - ndcRadius * sin(angle2), 0.0f);
        v[2].color = vertexColor;

        // Add vertices to the buffer
        for (int j = 0; j < 3; j++)
            m_VertexBuffer.push_back(v[j]);
    }
}

// Placeholder for text rendering - would need to implement a font system
void Renderer::DrawText(float x, float y, const std::string& text, const Color& color, bool centered)
{
    // Text rendering requires a font system or sprite font
    // This is a placeholder that doesn't actually render text
}

// Helper functions for ESP drawing
void Renderer::DrawBoundingBox(float x, float y, float width, float height, const Color& color, float thickness)
{
    DrawRect(x, y, width, height, color, thickness);
}

void Renderer::DrawHealthBar(float x, float y, float width, float height, int health, int maxHealth, bool vertical)
{
    // Background (dark gray)
    Color bgColor(40, 40, 40, 200);

    if (vertical)
    {
        // Vertical health bar (typically on the left/right side of a bounding box)
        DrawFilledRect(x, y, 3.0f + 2.0f, height, bgColor);

        // Calculate health bar height based on percentage
        float healthPercentage = (float)health / (float)maxHealth;
        float healthBarHeight = height * healthPercentage;

        // Health bar color (gradient from red to green based on health)
        Color healthColor(
            (1.0f - healthPercentage) * 255.0f,  // Red component
            healthPercentage * 255.0f,           // Green component
            0.0f,                                // Blue component
            200.0f                               // Alpha
        );

        // Draw the health bar from bottom to top
        DrawFilledRect(x, y + height - healthBarHeight, 3.0f + 2.0f, healthBarHeight, healthColor);
    }
    else
    {
        // Horizontal health bar (typically below a bounding box)
        DrawFilledRect(x, y, width, 3.0f + 2.0f, bgColor);

        // Calculate health bar width based on percentage
        float healthPercentage = (float)health / (float)maxHealth;
        float healthBarWidth = width * healthPercentage;

        // Health bar color (gradient from red to green based on health)
        Color healthColor(
            (1.0f - healthPercentage) * 255.0f,  // Red component
            healthPercentage * 255.0f,           // Green component
            0.0f,                                // Blue component
            200.0f                               // Alpha
        );

        // Draw the health bar from left to right
        DrawFilledRect(x, y, healthBarWidth, 3.0f + 2.0f, healthColor);
    }
}

void Renderer::DrawArmorBar(float x, float y, float width, float height, int armor, int maxArmor, bool vertical)
{
    // Similar to health bar but with blue color
    // Background (dark gray)
    Color bgColor(40, 40, 40, 200);

    if (vertical)
    {
        // Vertical armor bar
        DrawFilledRect(x, y, 3.0f + 2.0f, height, bgColor);

        // Calculate armor bar height based on percentage
        float armorPercentage = (float)armor / (float)maxArmor;
        float armorBarHeight = height * armorPercentage;

        // Armor bar color (blue)
        Color armorColor(0, 120, 255, 200);

        // Draw the armor bar from bottom to top
        DrawFilledRect(x, y + height - armorBarHeight, 3.0f + 2.0f, armorBarHeight, armorColor);
    }
    else
    {
        // Horizontal armor bar
        DrawFilledRect(x, y, width, 3.0f + 2.0f, bgColor);

        // Calculate armor bar width based on percentage
        float armorPercentage = (float)armor / (float)maxArmor;
        float armorBarWidth = width * armorPercentage;

        // Armor bar color (blue)
        Color armorColor(0, 120, 255, 200);

        // Draw the armor bar from left to right
        DrawFilledRect(x, y, armorBarWidth, 3.0f + 2.0f, armorColor);
    }
}

// Placeholder for outlined text - would need a font system
void Renderer::DrawOutlinedText(float x, float y, const std::string& text, const Color& textColor, const Color& outlineColor, bool centered)
{
    // Text rendering requires a font system or sprite font
    // This is a placeholder that doesn't actually render text
}

bool Renderer::WorldToScreen(const Vector3& world, Vector2& screen, const Matrix4x4& viewMatrix, int screenWidth, int screenHeight)
{
    // Calculate clip coordinates
    float clipX = world.x * viewMatrix.m[0][0] + world.y * viewMatrix.m[0][1] + world.z * viewMatrix.m[0][2] + viewMatrix.m[0][3];
    float clipY = world.x * viewMatrix.m[1][0] + world.y * viewMatrix.m[1][1] + world.z * viewMatrix.m[1][2] + viewMatrix.m[1][3];
    float clipZ = world.x * viewMatrix.m[2][0] + world.y * viewMatrix.m[2][1] + world.z * viewMatrix.m[2][2] + viewMatrix.m[2][3];
    float clipW = world.x * viewMatrix.m[3][0] + world.y * viewMatrix.m[3][1] + world.z * viewMatrix.m[3][2] + viewMatrix.m[3][3];

    // Check if the point is behind the camera
    if (clipW < 0.1f)
        return false;

    // Normalize by w component
    float ndcX = clipX / clipW;
    float ndcY = clipY / clipW;

    // Map to window coordinates
    screen.x = (screenWidth / 2.0f) * ndcX + (ndcX + screenWidth / 2.0f);
    screen.y = -(screenHeight / 2.0f) * ndcY + (ndcY + screenHeight / 2.0f);

    return true;
}