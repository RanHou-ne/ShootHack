//
// Created by Administrator on 2024/9/17.
//

#include "include/DX11.h"
#include "Engine/Engine.h"
#include "Drawing/Menu.h"
#include "Drawing/MenuSwitches.h"
#include "Drawing/Draw.h"
#include "globals.h"
#include "Config.h"
#include "imgui/imgui.h"
#include "ImGui/imgui_impl_dx11.h"
#include "ImGui/imgui_impl_win32.h"
#include <d3d11.h>
#include <wincodec.h>
#include <vector>
#include "mofang/font.h"
#include "mofang/image.h"

#pragma comment(lib, "windowscodecs.lib")

namespace font
{
    ImFont* pPoppinsMedium = nullptr;
    ImFont* pPoppinsMediumLow = nullptr;
    ImFont* pTabIcon = nullptr;
    ImFont* pChicons = nullptr;
    ImFont* pTahomaBold = nullptr;
    ImFont* pTahomaBold2 = nullptr;
}
namespace image
{
    ID3D11ShaderResourceView* bg = nullptr;
    ID3D11ShaderResourceView* logo = nullptr;
    ID3D11ShaderResourceView* logo_general = nullptr;

    ID3D11ShaderResourceView* arrow = nullptr;
    ID3D11ShaderResourceView* bell_notify = nullptr;
    ID3D11ShaderResourceView* roll = nullptr;
}

static bool FileExistsA(const char* path)
{
    DWORD attrs = GetFileAttributesA(path);
    return (attrs != INVALID_FILE_ATTRIBUTES) && !(attrs & FILE_ATTRIBUTE_DIRECTORY);
}

static bool LoadTextureFromFileWIC(
    ID3D11Device* device,
    const wchar_t* filePath,
    ID3D11ShaderResourceView** outSrv)
{
    if (device == nullptr || outSrv == nullptr || filePath == nullptr)
        return false;

    *outSrv = nullptr;

    const HRESULT coInitHr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    if (FAILED(coInitHr) && coInitHr != RPC_E_CHANGED_MODE)
        return false;

    IWICImagingFactory* factory = nullptr;
    IWICBitmapDecoder* decoder = nullptr;
    IWICBitmapFrameDecode* frame = nullptr;
    IWICFormatConverter* converter = nullptr;

    bool success = false;

    do
    {
        if (FAILED(CoCreateInstance(
            CLSID_WICImagingFactory,
            nullptr,
            CLSCTX_INPROC_SERVER,
            IID_PPV_ARGS(&factory))))
            break;

        if (FAILED(factory->CreateDecoderFromFilename(
            filePath,
            nullptr,
            GENERIC_READ,
            WICDecodeMetadataCacheOnLoad,
            &decoder)))
            break;

        if (FAILED(decoder->GetFrame(0, &frame)))
            break;

        UINT width = 0;
        UINT height = 0;
        if (FAILED(frame->GetSize(&width, &height)) || width == 0 || height == 0)
            break;

        if (FAILED(factory->CreateFormatConverter(&converter)))
            break;

        if (FAILED(converter->Initialize(
            frame,
            GUID_WICPixelFormat32bppRGBA,
            WICBitmapDitherTypeNone,
            nullptr,
            0.0,
            WICBitmapPaletteTypeCustom)))
            break;

        std::vector<unsigned char> pixels(static_cast<size_t>(width) * static_cast<size_t>(height) * 4);
        const UINT stride = width * 4;
        const UINT imageSize = stride * height;
        if (FAILED(converter->CopyPixels(nullptr, stride, imageSize, pixels.data())))
            break;

        D3D11_TEXTURE2D_DESC textureDesc = {};
        textureDesc.Width = width;
        textureDesc.Height = height;
        textureDesc.MipLevels = 1;
        textureDesc.ArraySize = 1;
        textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        textureDesc.SampleDesc.Count = 1;
        textureDesc.Usage = D3D11_USAGE_DEFAULT;
        textureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

        D3D11_SUBRESOURCE_DATA subresource = {};
        subresource.pSysMem = pixels.data();
        subresource.SysMemPitch = stride;

        ID3D11Texture2D* texture = nullptr;
        if (FAILED(device->CreateTexture2D(&textureDesc, &subresource, &texture)) || texture == nullptr)
            break;

        D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Format = textureDesc.Format;
        srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MipLevels = 1;

        const HRESULT srvHr = device->CreateShaderResourceView(texture, &srvDesc, outSrv);
        texture->Release();

        if (FAILED(srvHr) || *outSrv == nullptr)
            break;

        success = true;
    } while (false);

    if (converter) converter->Release();
    if (frame) frame->Release();
    if (decoder) decoder->Release();
    if (factory) factory->Release();

    if (!success)
        *outSrv = nullptr;

    return success;
}



/*
DX11 Hook 总流程
1) 先创建一个临时 SwapChain，拿到它的虚函数表地址。
2) 从虚表取出 Present / ResizeBuffers 的原函数地址。
3) 改写虚表，让游戏调用我们自己的 hkPresent / hkResize。
4) 在首次进入 hkPresent 时初始化 ImGui（Win32 + DX11 后端）。
5) 每帧在 hkPresent 里绘制菜单，再调用原始 Present 保持游戏画面正常输出。

【修复说明】
- 【修复】移除与 globals.cpp 重复的静态变量定义，统一使用 globals.h 的 extern 变量
- 【修复】添加 hkPresent 重入保护，防止递归调用导致崩溃
- 【修复】修复 hkResize 中向已释放虚表写入的严重崩溃
- 【修复】添加字体合并时的 nullptr 保护
- 【修复】添加渲染就绪标志，防止竞态条件
- 【修复】Config::Initialize 增加 try-catch 异常保护
*/


// 【修复】重入保护标志：防止 hkPresent 被递归调用导致崩溃
static bool g_isInPresent = false;
// 【修复】ImGui 渲染就绪标志：在 ImGui 完全初始化前不进行渲染
static bool g_imguiReadyForRender = false;
static bool g_autoConfigInitialized = false;
static double g_lastAutoSaveTime = 0.0;

// 【修复】以下变量已由 globals.h extern 声明、globals.cpp 定义，此处不再重复定义
// 原本这里定义了：
//   static ID3D11Device* g_pd3dDevice = nullptr; (已移除)
//   static ID3D11DeviceContext* g_pd3dDeviceContext = nullptr; (已移除)
//   static ID3D11RenderTargetView* g_mainRenderTargetView = nullptr; (已移除)
//   static bool g_imguiContextInitialized = false; (未使用，已移除)
//   static bool g_imguiDx11Initialized = false; (未使用，已移除)
//   static bool g_hookInstalled = false; (已移除)
//
// Present / WndProc / Resize 类型别名和函数指针也已在 globals.h 中定义

// 前向声明
LRESULT WINAPI hkWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// ==================== 资源管理 ====================

static void CleanupRenderTarget()
{
    if (g_mainRenderTargetView != nullptr)
    {
        g_mainRenderTargetView->Release();
        g_mainRenderTargetView = nullptr;
    }
}

static void ReleaseDxObjects()
{
    CleanupRenderTarget();

    if (g_pd3dDeviceContext != nullptr)
    {
        g_pd3dDeviceContext->Release();
        g_pd3dDeviceContext = nullptr;
    }

    if (g_pd3dDevice != nullptr)
    {
        g_pd3dDevice->Release();
        g_pd3dDevice = nullptr;
    }
}

static bool CreateRenderTarget()
{
    if (g_pSwapChain == nullptr || g_pd3dDevice == nullptr)
        return false;

    ID3D11Texture2D* backBuffer = nullptr;
    const HRESULT hr = g_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&backBuffer));
    if (FAILED(hr) || backBuffer == nullptr)
        return false;

    const HRESULT rtvHr = g_pd3dDevice->CreateRenderTargetView(backBuffer, nullptr, &g_mainRenderTargetView);
    backBuffer->Release();
    return SUCCEEDED(rtvHr) && g_mainRenderTargetView != nullptr;
}

static bool GetDX11ptr(IDXGISwapChain* This)
{
    ReleaseDxObjects();
    g_pSwapChain = This;

    const HRESULT deviceHr = g_pSwapChain->GetDevice(__uuidof(ID3D11Device), reinterpret_cast<void**>(&g_pd3dDevice));
    if (FAILED(deviceHr) || g_pd3dDevice == nullptr)
        return false;

    g_pd3dDevice->GetImmediateContext(&g_pd3dDeviceContext);
    if (g_pd3dDeviceContext == nullptr)
        return false;

    return CreateRenderTarget();
}

HRESULT STDMETHODCALLTYPE hkPresent(IDXGISwapChain* This, UINT SyncInterval, UINT Flags)
{
    // 【修复】重入保护：如果已经在 hkPresent 中，直接调用原始函数返回
    if (g_isInPresent)
    {
        if (g_oPresent == nullptr) return E_FAIL;
        return g_oPresent(This, SyncInterval, Flags);
    }
    g_isInPresent = true;

    if (This == nullptr)
    {
        printf("[-] hkPresent: This指针为NULL\n");
        g_isInPresent = false;
        return E_POINTER;
    }

    // 切换菜单（INSERT 键）
    if ((GetAsyncKeyState(VK_INSERT) & 1) != 0)
    {
        Menu::ToggleMenu();
    }

    // 第一次调用时初始化
    if (!g_imguiState.dx11BackendInitialized)
    {
        printf("[*] hkPresent首次调用，进行初始化...\n");

        if (!GetDX11ptr(This))
        {
            printf("[-] GetDX11ptr 失败\n");
            if (g_oPresent == nullptr) return E_FAIL;
            return g_oPresent(This, SyncInterval, Flags);
        }

        // 初始化ImGui上下文
        if (!g_imguiState.imguiInitialized)
        {
            printf("[*] 初始化ImGui上下文...\n");
            
            if ((g_hWnd == nullptr || !IsWindow(g_hWnd)) && This != nullptr)
            {
                DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
                if (SUCCEEDED(This->GetDesc(&swapChainDesc)) && swapChainDesc.OutputWindow != nullptr)
                {
                    g_hWnd = swapChainDesc.OutputWindow;
                    printf("[+] 从SwapChain获取窗口句柄: 0x%p\n", g_hWnd);
                }
            }

            if (g_hWnd != nullptr && IsWindow(g_hWnd) && g_oWndProc == nullptr)
            {
                g_oWndProc = reinterpret_cast<WndProc>(
                    SetWindowLongPtr(g_hWnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(hkWndProc))
                );
            }

            if (g_hWnd == nullptr || !IsWindow(g_hWnd))
            {
                printf("[-] ImGui初始化跳过：窗口句柄无效\n");
                g_isInPresent = false;
                if (g_oPresent == nullptr) return E_FAIL;
                return g_oPresent(This, SyncInterval, Flags);
            }

            IMGUI_CHECKVERSION();
            ImGui::CreateContext();
            ImGuiIO& io = ImGui::GetIO();
            io.Fonts->Clear();
            io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
            io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
            io.Fonts->TexDesiredWidth = 4096;
            
            // Add primary font (Chinese fallback)
            const ImWchar* chineseRange = io.Fonts->GetGlyphRangesChineseFull();
            const char* fontCandidates[] = {
                "C:\\Windows\\Fonts\\msyh.ttc",
                "C:\\Windows\\Fonts\\msyh.ttf",
                "C:\\Windows\\Fonts\\simhei.ttf",
                "C:\\Windows\\Fonts\\simsun.ttc",
                "C:\\Windows\\Fonts\\Deng.ttf"
            };

            bool fontLoaded = false;
            const char* selectedFontPath = nullptr;
            for (const char* fontPath : fontCandidates)
            {
                if (!FileExistsA(fontPath)) continue;
                ImFont* f = io.Fonts->AddFontFromFileTTF(fontPath, 18.0f, nullptr, chineseRange);
                if (f)
                {
                    fontLoaded = true;
                    selectedFontPath = fontPath;
                    io.FontDefault = f;
                    printf("[+] 字体加载成功: %s\n", fontPath);
                    break;
                }
            }

            if (!fontLoaded)
            {
                printf("[*] 使用默认字体\n");
                io.FontDefault = io.Fonts->AddFontDefault();
            }

            // Load custom beautiful fonts from memory
            ImFontConfig cfg = {}; // 强制清零初始化
            cfg.FontBuilderFlags = 0; 
            cfg.FontDataOwnedByAtlas = false; // 显式禁止 ImGui 释放静态内存，防止清理时崩溃

            font::pPoppinsMedium = io.Fonts->AddFontFromMemoryTTF(::poppins_medium, sizeof(::poppins_medium), 17.f, &cfg, io.Fonts->GetGlyphRangesDefault());
            font::pPoppinsMediumLow = io.Fonts->AddFontFromMemoryTTF(::poppins_medium, sizeof(::poppins_medium), 15.f, &cfg, io.Fonts->GetGlyphRangesDefault());
            font::pTabIcon = io.Fonts->AddFontFromMemoryTTF(::tabs_icons, sizeof(::tabs_icons), 24.f, &cfg, io.Fonts->GetGlyphRangesCyrillic());
            font::pTahomaBold = io.Fonts->AddFontFromMemoryTTF(::tahoma_bold, sizeof(::tahoma_bold), 17.f, &cfg, io.Fonts->GetGlyphRangesDefault());
            font::pTahomaBold2 = io.Fonts->AddFontFromMemoryTTF(::tahoma_bold, sizeof(::tahoma_bold), 28.f, &cfg, io.Fonts->GetGlyphRangesDefault());
            font::pChicons = io.Fonts->AddFontFromMemoryTTF(::chicon, sizeof(::chicon), 20.f, &cfg, io.Fonts->GetGlyphRangesCyrillic());

            // 内容文字默认使用 io.FontDefault（中文主字体），装饰字体仅用于英文标题/图标。

            // 【修复】添加 nullptr 保护：如果自定义字体加载失败，全部回退到默认字体
            if (font::pPoppinsMedium == nullptr) font::pPoppinsMedium = io.FontDefault;
            if (font::pPoppinsMediumLow == nullptr) font::pPoppinsMediumLow = io.FontDefault;
            if (font::pTabIcon == nullptr) font::pTabIcon = io.FontDefault;
            if (font::pTahomaBold == nullptr) font::pTahomaBold = io.FontDefault;
            if (font::pTahomaBold2 == nullptr) font::pTahomaBold2 = io.FontDefault;
            if (font::pChicons == nullptr) font::pChicons = io.FontDefault;

            // Load textures (skipped because d3dx11.h is missing)
            image::bg = nullptr;
            image::logo = nullptr;
            image::logo_general = nullptr;
            image::arrow = nullptr;
            image::bell_notify = nullptr;
            image::roll = nullptr;

            if (LoadTextureFromFileWIC(g_pd3dDevice, L".\\assets\\logo.png", &image::logo))
            {
                printf("[+] 已加载 logo: .\\assets\\logo.png\n");
            }
            else if (LoadTextureFromFileWIC(g_pd3dDevice, L".\\logo.png", &image::logo))
            {
                printf("[+] 已加载 logo: .\\logo.png\n");
            }
            else
            {
                printf("[*] 未找到 logo 贴图，使用文本头部\n");
            }

            // 侧边栏收起按钮图标（roll）
            if (LoadTextureFromFileWIC(g_pd3dDevice, L".\\assets\\roll.png", &image::roll))
            {
                printf("[+] 已加载 roll: .\\assets\\roll.png\n");
            }
            else if (LoadTextureFromFileWIC(g_pd3dDevice, L".\\roll.png", &image::roll))
            {
                printf("[+] 已加载 roll: .\\roll.png\n");
            }
            else
            {
                printf("[*] 未找到 roll 贴图，使用文本箭头兜底\n");
            }

                        ImGui::StyleColorsDark();

            // 【修复】双重保险：即使 dx11BackendInitialized 意外为 false，
            // 也确保 ImGui_ImplWin32_Init 只调用一次，防止断言崩溃
            if (!g_imguiState.imguiInitialized)
            {
                ImGui_ImplWin32_Init(g_hWnd);
                g_imguiState.imguiInitialized = true;
                printf("[+] ImGui上下文初始化完成\n");
            }
            else
            {
                printf("[!] 跳过重复的 ImGui_ImplWin32_Init（已在之前初始化）\n");
            }
        }

        // 初始化ImGui DX11后端
        if (!g_imguiState.dx11BackendInitialized && g_pd3dDevice != nullptr && g_pd3dDeviceContext != nullptr)
        {
            printf("[*] 初始化ImGui DX11后端...\n");
            if (!ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext))
            {
                printf("[-] ImGui DX11后端初始化失败\n");
                g_isInPresent = false;
                if (g_oPresent == nullptr) return E_FAIL;
                return g_oPresent(This, SyncInterval, Flags);
            }
            g_imguiState.dx11BackendInitialized = true;
            printf("[+] ImGui DX11后端初始化完成！\n");
            
            // 【修复】标记渲染就绪（在所有初始化完成后才允许渲染）
            g_imguiReadyForRender = true;
            
            // 初始化菜单系统和配置
            Menu::Initialize();
            // 【修复】Config::Initialize 增加 try-catch 异常保护
            try
            {
                Config::Initialize(".\\config");

                bool autoLoad = MenuSwitches::g_Toggles.AutoLoadPreview;
                bool autoSave = MenuSwitches::g_Toggles.AutoSavePreview;
                if (Config::LoadAutoSettings("settings_auto.json", autoLoad, autoSave))
                {
                    MenuSwitches::g_Toggles.AutoLoadPreview = autoLoad;
                    MenuSwitches::g_Toggles.AutoSavePreview = autoSave;
                    printf("[+] 自动配置开关加载成功: AutoLoad=%d AutoSave=%d\n", autoLoad ? 1 : 0, autoSave ? 1 : 0);
                }

                if (MenuSwitches::g_Toggles.AutoLoadPreview)
                {
                    if (Config::LoadConfig("settings.json"))
                        printf("[+] 启动自动读取配置成功: config\\settings.json\n");
                    else
                        printf("[-] 启动自动读取配置失败: config\\settings.json\n");
                }

                g_autoConfigInitialized = true;
                g_lastAutoSaveTime = ImGui::GetTime();
            }
            catch (const std::exception& ex)
            {
                printf("[-] Config::Initialize 异常: %s\n", ex.what());
            }
        }
    }

    if (g_autoConfigInitialized && g_imguiState.dx11BackendInitialized && MenuSwitches::g_Toggles.AutoSavePreview)
    {
        const double now = ImGui::GetTime();
        if (now - g_lastAutoSaveTime >= 10.0)
        {
            if (Config::SaveConfig("settings.json"))
            {
                g_lastAutoSaveTime = now;
                printf("[+] 自动保存配置成功: config\\settings.json\n");
            }
            else
            {
                printf("[-] 自动保存配置失败: config\\settings.json\n");
            }
        }
    }

    // 【修复】正常渲染路径：使用 g_imguiReadyForRender 确保所有资源已就绪
    if (g_imguiReadyForRender && g_imguiState.dx11BackendInitialized && g_mainRenderTargetView != nullptr && Menu::IsMenuOpen)
    {
        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();
        ImGui::GetIO().MouseDrawCursor = Menu::IsMenuOpen;

        Menu::Render();

        ImGui::Render();
        g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, nullptr);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
    }
    
        if (g_oPresent == nullptr)
    {
        printf("[-] hkPresent: g_oPresent为NULL\n");
        g_isInPresent = false;
        return E_POINTER;
    }
    
    HRESULT result = g_oPresent(This, SyncInterval, Flags);
    g_isInPresent = false;
    return result;
}

// 让 ImGui 能处理鼠标/键盘消息。
// 核心思想：先把消息交给 ImGui 处理；若 ImGui 不消费，再交还给游戏原窗口过程。
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT WINAPI hkWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (Menu::IsMenuOpen && ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return TRUE;

    if (g_oWndProc == nullptr)
        return DefWindowProcW(hWnd, msg, wParam, lParam);

    return CallWindowProcW(g_oWndProc, hWnd, msg, wParam, lParam);
}








HRESULT WINAPI hkResize(IDXGISwapChain* This, UINT BufferCount, UINT Width, UINT Height, DXGI_FORMAT NewFormat, UINT SwapChainFlags)
{
    // 【修复】先关闭 ImGui DX11 后端
    if (g_imguiState.dx11BackendInitialized)
    {
        ImGui_ImplDX11_Shutdown();
        g_imguiState.dx11BackendInitialized = false;
    }
    g_imguiReadyForRender = false;

    // 【修复】清理 RenderTarget（在 Resize 前清理，因为 Resize 会销毁旧的 back buffer）
    if (g_mainRenderTargetView != nullptr)
    {
        g_mainRenderTargetView->Release();
        g_mainRenderTargetView = nullptr;
    }
    
    // 【修复】不再调用 ReleaseDxObjects()（游戏还需要 Device/Context）
    // 【修复】不再写入旧虚表（旧 SwapChain 可能已被销毁，写入会崩溃！）

    // 调用原始 Resize
    HRESULT hr = g_oResize(This, BufferCount, Width, Height, NewFormat, SwapChainFlags);

    // 【修复】Resize 成功后更新全局 SwapChain 指针
    if (SUCCEEDED(hr))
    {
        g_pSwapChain = This;
    }

    return hr;
}

bool DX11HOOK()
{
    if (g_hookInstalled)
        return true;

    for (int i = 0; i < 100 && g_hWnd == nullptr; ++i)
    {
        g_hWnd = FindWindowA("UnityWndClass", nullptr);
        if (g_hWnd == nullptr)
            Sleep(100);
    }

    if (g_hWnd == nullptr)
    {
        printf("[-] DX11HOOK: 未找到Unity窗口\n");
        return false;
    }

    printf("[+] 找到Unity窗口: 0x%p\n", g_hWnd);

    DXGI_SWAP_CHAIN_DESC sd;
    ZeroMemory(&sd, sizeof(sd));
    sd.BufferCount = 2;
    sd.BufferDesc.Width = 0;
    sd.BufferDesc.Height = 0;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = g_hWnd;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    UINT createDeviceFlags = 0;
    D3D_FEATURE_LEVEL featureLevel;
    const D3D_FEATURE_LEVEL featureLevelArray[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0 };
    IDXGISwapChain* tempSwapChain = nullptr;
    ID3D11Device* tempDevice = nullptr;
    ID3D11DeviceContext* tempContext = nullptr;

    printf("[*] 创建临时DX11设备和SwapChain...\n");
    const HRESULT hr = D3D11CreateDeviceAndSwapChain(
        nullptr,
        D3D_DRIVER_TYPE_HARDWARE,
        nullptr,
        createDeviceFlags,
        featureLevelArray,
        2,
        D3D11_SDK_VERSION,
        &sd,
        &tempSwapChain,
        &tempDevice,
        &featureLevel,
        &tempContext);

    if (FAILED(hr) || tempSwapChain == nullptr)
    {
        printf("[-] 创建DX11设备失败: 0x%x\n", hr);
        return false;
    }

    printf("[+] DX11设备创建成功\n");

    g_vtableSwapChain = *reinterpret_cast<DWORD64**>(tempSwapChain);
    if (g_vtableSwapChain == nullptr)
    {
        printf("[-] 获取虚表失败\n");
        tempContext->Release();
        tempDevice->Release();
        tempSwapChain->Release();
        return false;
    }

    printf("[+] 获取虚表成功: 0x%p\n", g_vtableSwapChain);

    g_oPresent = reinterpret_cast<Present>(g_vtableSwapChain[8]);
    g_oResize = reinterpret_cast<Resize>(g_vtableSwapChain[13]);

    if (g_oPresent == nullptr || g_oResize == nullptr)
    {
        printf("[-] 获取原始函数指针失败\n");
        tempContext->Release();
        tempDevice->Release();
        tempSwapChain->Release();
        return false;
    }

    printf("[+] 原始函数: Present = 0x%p, Resize = 0x%p\n", g_oPresent, g_oResize);

    printf("[*] 修改虚表...\n");
    DWORD protect = 0;
    if (!VirtualProtect(g_vtableSwapChain, sizeof(void*) * 14, PAGE_EXECUTE_READWRITE, &protect))
    {
        printf("[-] 改变虚表保护属性失败\n");
        tempContext->Release();
        tempDevice->Release();
        tempSwapChain->Release();
        return false;
    }

    g_vtableSwapChain[8] = reinterpret_cast<DWORD64>(hkPresent);
    g_vtableSwapChain[13] = reinterpret_cast<DWORD64>(hkResize);
    VirtualProtect(g_vtableSwapChain, sizeof(void*) * 14, protect, &protect);

    printf("[+] 虚表修改完成\n");

    tempContext->Release();
    tempDevice->Release();
    tempSwapChain->Release();

    // 【修复】在虚表修改后立即初始化全局状态。
    // 必须要先初始化，再释放临时资源，因为 InitializeGlobals()
    // 会重置 g_imguiState，如果放在后面（或放在修改虚表之前），
    // 一旦游戏在 DX11HOOK() 返回前调用了 Present 并触发了 hkPresent，
    // hkPresent 中设置的 g_imguiState 会被随后的 InitializeGlobals() 重置，
    // 导致第二次 Present 时重复初始化 ImGui 后端而触发断言崩溃。
    InitializeGlobals();
    
    // 【修复】使用全局 g_hookInstalled（已由 globals.h extern 声明）
    g_hookInstalled = true;
    
    // 【修复】将 Engine 缓存的窗口句柄同步到全局
    if (g_hWnd == nullptr || !IsWindow(g_hWnd))
    {
        g_hWnd = Engine::GetHwnd();
    }
    
    printf("[+] DX11 Hook 安装成功！\n");
    printf("    • Present 已 Hook 到 hkPresent\n");
    printf("    • 首次调用时将初始化 ImGui\n");
    printf("    • 按 INSERT 键开关菜单\n");
    return true;
}




