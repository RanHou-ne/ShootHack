// ============================================================================
// 文件：src/Hook/DX11Hook.cpp
// 模块：Hook 层 - DX11 虚表 Hook 核心实现
// ============================================================================

#include "DX11Hook.h"
#include "Core/globals.h"
#include "Core/FpsCounter.h"  // FPS计数器
#include "UI/Menu/Menu.h"
#include "UI/Menu/MenuSwitches.h"
#include "Config/Config.h"
#include "Engine/class.h"
#include "ESP/ESP.h"
#include "ImGui/imgui.h"
#include "ImGui/imgui_impl_dx11.h"
#include "ImGui/imgui_impl_win32.h"
#include <d3d11.h>
#include <wincodec.h>
#include <imm.h>        // IME (输入法编辑器) 支持
#pragma comment(lib, "imm32.lib")
#include <vector>
#include <cstdio>

// 字体/图片内存数据（资源文件）
#include "Assets/font.h"
#include "Assets/image.h"

// 启用内置图标
#define USE_EMBEDDED_IMAGES



// ============================================================================
// font / image 全局变量定义
// ============================================================================
// Menu.cpp 中通过 extern 声明引用这些变量

namespace font
{
    ImFont* pPoppinsMedium    = nullptr;
    ImFont* pPoppinsMediumLow = nullptr;
    ImFont* pTabIcon          = nullptr;
    ImFont* pChicons          = nullptr;
    ImFont* pTahomaBold       = nullptr;
    ImFont* pTahomaBold2      = nullptr;
}

namespace image
{
    ID3D11ShaderResourceView* bg           = nullptr;
    ID3D11ShaderResourceView* logo         = nullptr;
    ID3D11ShaderResourceView* logo_general = nullptr;
    ID3D11ShaderResourceView* arrow        = nullptr;
    ID3D11ShaderResourceView* bell_notify  = nullptr;
    ID3D11ShaderResourceView* roll         = nullptr;
}

// ============================================================================
// 内部状态
// ============================================================================

static bool   g_isInPresent          = false;  // 重入保护
static bool   g_imguiReadyForRender  = false;  // ImGui 完全初始化后才渲染
static bool   g_autoConfigInitialized = false;
static double g_lastAutoSaveTime     = 0.0;

// FPS 计数器
static FpsCounter g_gameFpsCounter(60);      // 游戏FPS（60帧平滑）
static FpsCounter g_overlayFpsCounter(60);   // 覆盖层FPS（60帧平滑）

// ============================================================================
// 工具函数
// ============================================================================

static bool FileExistsA(const char* path)
{
    DWORD attrs = GetFileAttributesA(path);
    return (attrs != INVALID_FILE_ATTRIBUTES) && !(attrs & FILE_ATTRIBUTE_DIRECTORY);
}

// 前向声明
static void RenderFpsDisplay();

static bool LoadTextureFromFileWIC(
    ID3D11Device* device,
    const wchar_t* filePath,
    ID3D11ShaderResourceView** outSrv)
{
    if (!device || !outSrv || !filePath) return false;
    *outSrv = nullptr;

    const HRESULT coInitHr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    if (FAILED(coInitHr) && coInitHr != RPC_E_CHANGED_MODE) return false;

    IWICImagingFactory*    factory   = nullptr;
    IWICBitmapDecoder*     decoder   = nullptr;
    IWICBitmapFrameDecode* frame     = nullptr;
    IWICFormatConverter*   converter = nullptr;
    bool success = false;

    do {
        if (FAILED(CoCreateInstance(CLSID_WICImagingFactory, nullptr,
            CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&factory)))) break;
        if (FAILED(factory->CreateDecoderFromFilename(filePath, nullptr,
            GENERIC_READ, WICDecodeMetadataCacheOnLoad, &decoder))) break;
        if (FAILED(decoder->GetFrame(0, &frame))) break;

        UINT width = 0, height = 0;
        if (FAILED(frame->GetSize(&width, &height)) || !width || !height) break;
        if (FAILED(factory->CreateFormatConverter(&converter))) break;
        if (FAILED(converter->Initialize(frame, GUID_WICPixelFormat32bppRGBA,
            WICBitmapDitherTypeNone, nullptr, 0.0, WICBitmapPaletteTypeCustom))) break;

        std::vector<unsigned char> pixels(static_cast<size_t>(width) * height * 4);
        const UINT stride = width * 4;
        if (FAILED(converter->CopyPixels(nullptr, stride, stride * height, pixels.data()))) break;

        D3D11_TEXTURE2D_DESC td = {};
        td.Width = width; td.Height = height;
        td.MipLevels = 1; td.ArraySize = 1;
        td.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        td.SampleDesc.Count = 1;
        td.Usage = D3D11_USAGE_DEFAULT;
        td.BindFlags = D3D11_BIND_SHADER_RESOURCE;

        D3D11_SUBRESOURCE_DATA sr = {};
        sr.pSysMem = pixels.data(); sr.SysMemPitch = stride;

        ID3D11Texture2D* tex = nullptr;
        if (FAILED(device->CreateTexture2D(&td, &sr, &tex)) || !tex) break;

        D3D11_SHADER_RESOURCE_VIEW_DESC srvd = {};
        srvd.Format = td.Format;
        srvd.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
        srvd.Texture2D.MipLevels = 1;

        const HRESULT srvHr = device->CreateShaderResourceView(tex, &srvd, outSrv);
        tex->Release();
        if (FAILED(srvHr) || !*outSrv) break;
        success = true;
    } while (false);

    if (converter) converter->Release();
    if (frame)     frame->Release();
    if (decoder)   decoder->Release();
    if (factory)   factory->Release();
    if (!success)  *outSrv = nullptr;
    return success;
}

// ============================================================================
// LoadTextureFromMemory —— 从内存数据加载纹理
// ============================================================================
// 用于加载内嵌在代码中的图片数据（如 image.h 中的字节数组）
static bool LoadTextureFromMemory(
    ID3D11Device* device,
    const unsigned char* imageData,
    size_t imageSize,
    ID3D11ShaderResourceView** outSrv)
{
    if (!device || !outSrv || !imageData || imageSize == 0) return false;
    *outSrv = nullptr;

    const HRESULT coInitHr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    if (FAILED(coInitHr) && coInitHr != RPC_E_CHANGED_MODE) return false;

    IWICImagingFactory*    factory   = nullptr;
    IWICStream*            stream    = nullptr;
    IWICBitmapDecoder*     decoder   = nullptr;
    IWICBitmapFrameDecode* frame     = nullptr;
    IWICFormatConverter*   converter = nullptr;
    bool success = false;

    do {
        if (FAILED(CoCreateInstance(CLSID_WICImagingFactory, nullptr,
            CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&factory)))) break;
        
        // 创建内存流
        if (FAILED(factory->CreateStream(&stream))) break;
        if (FAILED(stream->InitializeFromMemory(
            const_cast<unsigned char*>(imageData), 
            static_cast<DWORD>(imageSize)))) break;
        
        // 从流创建解码器
        if (FAILED(factory->CreateDecoderFromStream(stream, nullptr,
            WICDecodeMetadataCacheOnLoad, &decoder))) break;
        if (FAILED(decoder->GetFrame(0, &frame))) break;

        UINT width = 0, height = 0;
        if (FAILED(frame->GetSize(&width, &height)) || !width || !height) break;
        if (FAILED(factory->CreateFormatConverter(&converter))) break;
        if (FAILED(converter->Initialize(frame, GUID_WICPixelFormat32bppRGBA,
            WICBitmapDitherTypeNone, nullptr, 0.0, WICBitmapPaletteTypeCustom))) break;

        std::vector<unsigned char> pixels(static_cast<size_t>(width) * height * 4);
        const UINT stride = width * 4;
        if (FAILED(converter->CopyPixels(nullptr, stride, stride * height, pixels.data()))) break;

        D3D11_TEXTURE2D_DESC td = {};
        td.Width = width; td.Height = height;
        td.MipLevels = 1; td.ArraySize = 1;
        td.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        td.SampleDesc.Count = 1;
        td.Usage = D3D11_USAGE_DEFAULT;
        td.BindFlags = D3D11_BIND_SHADER_RESOURCE;

        D3D11_SUBRESOURCE_DATA sr = {};
        sr.pSysMem = pixels.data(); sr.SysMemPitch = stride;

        ID3D11Texture2D* tex = nullptr;
        if (FAILED(device->CreateTexture2D(&td, &sr, &tex)) || !tex) break;

        D3D11_SHADER_RESOURCE_VIEW_DESC srvd = {};
        srvd.Format = td.Format;
        srvd.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
        srvd.Texture2D.MipLevels = 1;

        const HRESULT srvHr = device->CreateShaderResourceView(tex, &srvd, outSrv);
        tex->Release();
        if (FAILED(srvHr) || !*outSrv) break;
        success = true;
    } while (false);

    if (converter) converter->Release();
    if (frame)     frame->Release();
    if (decoder)   decoder->Release();
    if (stream)    stream->Release();
    if (factory)   factory->Release();
    if (!success)  *outSrv = nullptr;
    return success;
}

// ============================================================================
// 资源管理
// ============================================================================

static void CleanupRenderTarget()
{
    if (g_mainRenderTargetView) { g_mainRenderTargetView->Release(); g_mainRenderTargetView = nullptr; }
}

static bool CreateRenderTarget()
{
    if (!g_pSwapChain || !g_pd3dDevice) return false;
    ID3D11Texture2D* bb = nullptr;
    if (FAILED(g_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&bb))) || !bb)
        return false;
    const HRESULT hr = g_pd3dDevice->CreateRenderTargetView(bb, nullptr, &g_mainRenderTargetView);
    bb->Release();
    return SUCCEEDED(hr) && g_mainRenderTargetView != nullptr;
}

static bool GetDX11ptr(IDXGISwapChain* This)
{
    CleanupRenderTarget();
    if (g_pd3dDeviceContext) { g_pd3dDeviceContext->Release(); g_pd3dDeviceContext = nullptr; }
    if (g_pd3dDevice)        { g_pd3dDevice->Release();        g_pd3dDevice = nullptr; }

    g_pSwapChain = This;
    if (FAILED(g_pSwapChain->GetDevice(__uuidof(ID3D11Device), reinterpret_cast<void**>(&g_pd3dDevice))) || !g_pd3dDevice)
        return false;
    g_pd3dDevice->GetImmediateContext(&g_pd3dDeviceContext);
    if (!g_pd3dDeviceContext) return false;
    return CreateRenderTarget();
}

// ============================================================================
// hkWndProc —— 窗口消息拦截
// ============================================================================

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND, UINT, WPARAM, LPARAM);

LRESULT WINAPI hkWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    // 调试：打印 IME 相关消息
    static bool debugIme = false;  // 设置为 true 启用调试
    if (debugIme && msg >= 0x0280 && msg <= 0x0290)
    {
        printf("[IME] msg=0x%04X wParam=0x%08X lParam=0x%08X\n", msg, (unsigned int)wParam, (unsigned int)lParam);
    }
    
    // IME 消息必须传递给原始窗口过程，否则输入法无法工作
    // IME 消息范围：WM_IME_SETCONTEXT(0x0281) 到 WM_IME_KEYUP(0x0290)
    if (msg >= 0x0280 && msg <= 0x0290)
    {
        // IME 消息直接传递给原始窗口过程，不经过 ImGui
        if (g_oWndProc)
            return CallWindowProcW(g_oWndProc, hWnd, msg, wParam, lParam);
        return DefWindowProcW(hWnd, msg, wParam, lParam);
    }
    
    // 菜单打开时，让 ImGui 处理其他消息
    if (Menu::IsMenuOpen && ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return TRUE;
        
    // 传递给原始窗口过程
    if (!g_oWndProc) 
        return DefWindowProcW(hWnd, msg, wParam, lParam);
    return CallWindowProcW(g_oWndProc, hWnd, msg, wParam, lParam);
}

// ============================================================================
// hkPresent —— 每帧渲染入口
// ============================================================================

HRESULT WINAPI hkPresent(IDXGISwapChain* This, UINT SyncInterval, UINT Flags)
{
    // 重入保护
    if (g_isInPresent)
        return g_oPresent ? g_oPresent(This, SyncInterval, Flags) : E_FAIL;
    g_isInPresent = true;

    if (!This) { g_isInPresent = false; return E_POINTER; }

    // INSERT 键切换菜单
    if (GetAsyncKeyState(VK_INSERT) & 1)
        Menu::ToggleMenu();

    // ---- 首次调用：初始化 ImGui ----
    if (!g_imguiState.dx11BackendInitialized)
    {
        if (!GetDX11ptr(This))
        {
            g_isInPresent = false;
            return g_oPresent ? g_oPresent(This, SyncInterval, Flags) : E_FAIL;
        }

        // 获取窗口句柄
        if (!g_hWnd || !IsWindow(g_hWnd))
        {
            DXGI_SWAP_CHAIN_DESC desc = {};
            if (SUCCEEDED(This->GetDesc(&desc)) && desc.OutputWindow)
                g_hWnd = desc.OutputWindow;
        }

        // Hook WndProc
        if (g_hWnd && IsWindow(g_hWnd) && !g_oWndProc)
        {
            g_oWndProc = reinterpret_cast<WndProcFn>(
                SetWindowLongPtr(g_hWnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(hkWndProc)));
        }

        if (!g_hWnd || !IsWindow(g_hWnd))
        {
            g_isInPresent = false;
            return g_oPresent ? g_oPresent(This, SyncInterval, Flags) : E_FAIL;
        }

        // ---- ImGui 上下文初始化（只做一次）----
        if (!g_imguiState.imguiInitialized)
        {
            IMGUI_CHECKVERSION();
            ImGui::CreateContext();
            ImGuiIO& io = ImGui::GetIO();
            io.Fonts->Clear();
            io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
            io.Fonts->TexDesiredWidth = 4096;
            
            // 显式启用 IME（输入法编辑器）
            // 这对于中文输入至关重要
            if (g_hWnd && IsWindow(g_hWnd))
            {
                // 关联 IME 上下文到窗口
                HIMC hImc = ImmGetContext(g_hWnd);
                if (hImc)
                {
                    ImmAssociateContext(g_hWnd, hImc);
                    ImmReleaseContext(g_hWnd, hImc);
                }
                printf("[+] IME 上下文已关联到窗口\n");
            }

            // 中文字体
            const ImWchar* chRange = io.Fonts->GetGlyphRangesChineseFull();
            const char* fontCandidates[] = {
                "C:\\Windows\\Fonts\\msyh.ttc",
                "C:\\Windows\\Fonts\\msyh.ttf",
                "C:\\Windows\\Fonts\\simhei.ttf",
                "C:\\Windows\\Fonts\\simsun.ttc",
                "C:\\Windows\\Fonts\\Deng.ttf"
            };
            bool fontLoaded = false;
            for (const char* fp : fontCandidates)
            {
                if (!FileExistsA(fp)) continue;
                ImFont* f = io.Fonts->AddFontFromFileTTF(fp, 18.0f, nullptr, chRange);
                if (f) { io.FontDefault = f; fontLoaded = true; printf("[+] 字体: %s\n", fp); break; }
            }
            if (!fontLoaded) io.FontDefault = io.Fonts->AddFontDefault();

            // 自定义字体（内存数据）
            ImFontConfig cfg = {};
            cfg.FontDataOwnedByAtlas = false;
            font::pPoppinsMedium    = io.Fonts->AddFontFromMemoryTTF(::poppins_medium, sizeof(::poppins_medium), 17.f, &cfg, io.Fonts->GetGlyphRangesDefault());
            font::pPoppinsMediumLow = io.Fonts->AddFontFromMemoryTTF(::poppins_medium, sizeof(::poppins_medium), 15.f, &cfg, io.Fonts->GetGlyphRangesDefault());
            font::pTabIcon          = io.Fonts->AddFontFromMemoryTTF(::tabs_icons,     sizeof(::tabs_icons),     24.f, &cfg, io.Fonts->GetGlyphRangesCyrillic());
            font::pTahomaBold       = io.Fonts->AddFontFromMemoryTTF(::tahoma_bold,    sizeof(::tahoma_bold),    17.f, &cfg, chRange);  // 使用中文字符范围
            font::pTahomaBold2      = io.Fonts->AddFontFromMemoryTTF(::tahoma_bold,    sizeof(::tahoma_bold),    28.f, &cfg, chRange);  // 使用中文字符范围
            font::pChicons          = io.Fonts->AddFontFromMemoryTTF(::chicon,         sizeof(::chicon),         20.f, &cfg, io.Fonts->GetGlyphRangesCyrillic());

            // nullptr 保护：加载失败时回退到默认字体
            if (!font::pPoppinsMedium)    font::pPoppinsMedium    = io.FontDefault;
            if (!font::pPoppinsMediumLow) font::pPoppinsMediumLow = io.FontDefault;
            if (!font::pTabIcon)          font::pTabIcon          = io.FontDefault;
            if (!font::pTahomaBold)       font::pTahomaBold       = io.FontDefault;
            if (!font::pTahomaBold2)      font::pTahomaBold2      = io.FontDefault;
            if (!font::pChicons)          font::pChicons          = io.FontDefault;

            // ============================================================================
            // 纹理加载 - 强制使用内嵌字节数组（image.h）
            // ============================================================================
            // 所有图片资源必须预先转换为字节数组并定义在 image.h 中
            // 使用 Scripts/image_to_cpp.py 将 PNG/JPG 转换为 C++ 字节数组
            
            // 加载 logo（强制从内嵌数据加载）
            #ifdef USE_EMBEDDED_IMAGES
            if (LoadTextureFromMemory(g_pd3dDevice, logo, sizeof(logo), &image::logo))
            {
                printf("[+] Logo 从内嵌数据加载成功\n");
            }
            else
            {
                printf("[!] 错误: Logo 内嵌数据加载失败！请检查 image.h 中的 logo 数组\n");
            }
            #else
            printf("[!] 警告: USE_EMBEDDED_IMAGES 未定义，Logo 将不会加载\n");
            #endif
            
            // 加载 roll 图标（强制从内嵌数据加载）
            #ifdef USE_EMBEDDED_IMAGES
            if (LoadTextureFromMemory(g_pd3dDevice, roll, sizeof(roll), &image::roll))
            {
                printf("[+] Roll 图标从内嵌数据加载成功\n");
            }
            else
            {
                printf("[!] 错误: Roll 图标内嵌数据加载失败！请检查 image.h 中的 roll 数组\n");
            }
            #else
            printf("[!] 警告: USE_EMBEDDED_IMAGES 未定义，Roll 图标将不会加载\n");
            #endif

            ImGui::StyleColorsDark();
            ImGui_ImplWin32_Init(g_hWnd);
            g_imguiState.imguiInitialized = true;
            printf("[+] ImGui 上下文初始化完成\n");
        }

        // ---- ImGui DX11 后端初始化 ----
        if (!g_imguiState.dx11BackendInitialized && g_pd3dDevice && g_pd3dDeviceContext)
        {
            if (!ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext))
            {
                g_isInPresent = false;
                return g_oPresent ? g_oPresent(This, SyncInterval, Flags) : E_FAIL;
            }
            g_imguiState.dx11BackendInitialized = true;
            g_imguiReadyForRender = true;
            printf("[+] ImGui DX11 后端初始化完成\n");

 

            
            // //=============================================================================
            // // 测试：通过 IL2CPP C API 获取 GameObject 列表，并读取每个对象的 Il2CppClass 信息
            // //
            // // 正确的链式调用（不依赖 Type.GetType 反射，IL2CPP 中反射元数据可能被裁剪）：
            // //   il2cpp_domain_get → il2cpp_domain_assembly_open → il2cpp_assembly_get_image
            // //   → il2cpp_class_from_name → il2cpp_class_get_type → il2cpp_type_get_object
            // //   → FindObjectsOfType
            // //=============================================================================

            // Il2CppDomain* pDomain = il2cpp_domain_get();
            // if (!pDomain) { printf("[-] il2cpp_domain_get 返回空\n"); }

            // if (pDomain)
            // {
            //     const Il2CppAssembly* pAssembly = il2cpp_domain_assembly_open(pDomain, "UnityEngine.CoreModule");
            //     if (!pAssembly) { printf("[-] il2cpp_domain_assembly_open 返回空\n"); }

            //     if (pAssembly)
            //     {
            //         const Il2CppImage* pImage = il2cpp_assembly_get_image(pAssembly);
            //         if (!pImage) { printf("[-] il2cpp_assembly_get_image 返回空\n"); }

            //         if (pImage)
            //         {
            //             Il2CppClass* pClass = il2cpp_class_from_name(pImage, "UnityEngine", "GameObject");
            //             if (!pClass) { printf("[-] il2cpp_class_from_name 返回空\n"); }

            //             if (pClass)
            //             {
            //                 const Il2CppType* pType = il2cpp_class_get_type(pClass);
            //                 if (!pType) { printf("[-] il2cpp_class_get_type 返回空\n"); }

            //                 if (pType)
            //                 {
            //                     Il2CppObject* pTypeObj = il2cpp_type_get_object(pType);
            //                     if (!pTypeObj) { printf("[-] il2cpp_type_get_object 返回空\n"); }

            //                     if (pTypeObj)
            //                     {
            //                         Unity_Array* ObjArray = FindObjectsOfType(pTypeObj);
            //                         if (ObjArray && ObjArray->Count > 0)
            //                         {
            //                             printf("[+] FindObjectsOfType 返回 %zu 个 GameObject\n", ObjArray->Count);
            //                             for (size_t i = 0; i < ObjArray->Count; i++)
            //                             {
            //                                 if (!ObjArray->Objects[i]) continue;
            //                                 Il2CppObject* pObject = reinterpret_cast<Il2CppObject*>(ObjArray->Objects[i]);
            //                                 Il2CppClass* pObjClass = pObject ? pObject->klass : nullptr;
            //                                 printf("[%zu] ClassName: %s, Namespace: %s\n", i,
            //                                        pObjClass ? pObjClass->ClassName : "<null>",
            //                                        pObjClass ? pObjClass->Namespace : "<null>");
            //                             }
            //                         }
            //                         else
            //                         {
            //                             printf("[-] FindObjectsOfType 返回空数组或空指针\n");
            //                         }
            //                     }
            //                 }
            //             }
            //         }
            //     }
            // }




            // 测试：获取 GameObject 列表并通过方法指针读取信息
            // {
            //     Il2CppDomain* pDomain = il2cpp_domain_get();
            //     const Il2CppAssembly* pAssembly = il2cpp_domain_assembly_open(pDomain, "UnityEngine.CoreModule");
            //     if (pAssembly)
            //     {
            //         const Il2CppImage* pImage = il2cpp_assembly_get_image(pAssembly);
            //         Il2CppClass* pClass = il2cpp_class_from_name(pImage, "UnityEngine", "GameObject");
            //         if (pClass)
            //         {
            //             const Il2CppType* pType = il2cpp_class_get_type(pClass);
            //             Il2CppObject* pTypeObj = il2cpp_type_get_object(pType);
            //             if (pTypeObj)
            //             {
            //                 System_Object_array* ObjArray = reinterpret_cast<System_Object_array*>(FindObjectsOfType(pTypeObj));
            //                 if (ObjArray && ObjArray->max_length > 0)
            //                 {
            //                     size_t total = (size_t)ObjArray->max_length;
            //                     printf("\n========== 找到 %zu 个 GameObject，读取所有信息 ==========\n", total);
            //                     for (size_t i = 0; i < total; i++)
            //                     {
            //                         printf("[%zu] ", i);
            //                         Engine::PrintGameObjectInfo(ObjArray->m_Items[i]);
            //                     }
            //                 }
            //                 else
            //                 {
            //                     printf("[-] 未找到 GameObject，改用 DumpAssembly 查看可用类\n");
            //                     Engine::DumpAssembly("UnityEngine.CoreModule");
            //                 }
            //             }
            //         }
            //     }
            // }




            

            try
            {
                Config::Initialize(".\\config");
                bool autoLoad = false, autoSave = false;
                if (Config::LoadAutoSettings("settings_auto.json", autoLoad, autoSave))
                {
                    MenuSwitches::g_Toggles.AutoLoadPreview = autoLoad;
                    MenuSwitches::g_Toggles.AutoSavePreview = autoSave;
                }
                if (MenuSwitches::g_Toggles.AutoLoadPreview)
                    Config::LoadConfig("settings.json");

                g_autoConfigInitialized = true;
                g_lastAutoSaveTime = ImGui::GetTime();
            }
            catch (const std::exception& ex)
            {
                printf("[-] Config::Initialize 异常: %s\n", ex.what());
            }
        }
    }

    // ---- 自动保存（每 10 秒）----
    if (g_autoConfigInitialized && MenuSwitches::g_Toggles.AutoSavePreview)
    {
        const double now = ImGui::GetTime();
        if (now - g_lastAutoSaveTime >= 10.0)
        {
            Config::SaveConfig("settings.json");
            g_lastAutoSaveTime = now;
        }
    }

    // ---- 更新游戏FPS计数器（每次Present调用） ----
    g_gameFpsCounter.Update();

    // ---- 渲染菜单和覆盖层 ----
    if (g_imguiReadyForRender && g_mainRenderTargetView)
    {
        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        // 更新覆盖层FPS计数器
        g_overlayFpsCounter.Update();

        // 渲染FPS显示（如果启用）
        if (MenuSwitches::g_Toggles.ShowFps)
        {
            RenderFpsDisplay();
        }

        // 绘制 ESP 覆盖层（对象指针、方框、骨骼等）
        ESP::Render();

        // 渲染菜单
        if (Menu::IsMenuOpen)
        {
            ImGui::GetIO().MouseDrawCursor = true;
            Menu::Render();
        }
        else
        {
            ImGui::GetIO().MouseDrawCursor = false;
        }

        ImGui::Render();
        g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, nullptr);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
    }

    if (!g_oPresent) { g_isInPresent = false; return E_POINTER; }
    const HRESULT result = g_oPresent(This, SyncInterval, Flags);
    g_isInPresent = false;
    return result;
}

// ============================================================================
// RenderFpsDisplay —— 渲染FPS显示
// ============================================================================

static void RenderFpsDisplay()
{
    // 检查是否至少有一个FPS显示选项被启用
    const bool showGameFps = MenuSwitches::g_Toggles.ShowGameFps;
    const bool showOverlayFps = MenuSwitches::g_Toggles.ShowOverlayFps;
    
    if (!showGameFps && !showOverlayFps)
        return;  // 两个都不显示，直接返回

    // 获取FPS数据
    const float gameFps = g_gameFpsCounter.GetFPS();
    const float overlayFps = g_overlayFpsCounter.GetFPS();
    const float gameFrameTime = g_gameFpsCounter.GetAvgFrameTime();
    const float overlayFrameTime = g_overlayFpsCounter.GetAvgFrameTime();

    // 设置窗口位置（右上角）
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImVec2 windowPos = ImVec2(viewport->Pos.x + viewport->Size.x - 10.0f, viewport->Pos.y + 10.0f);
    ImGui::SetNextWindowPos(windowPos, ImGuiCond_Always, ImVec2(1.0f, 0.0f));
    ImGui::SetNextWindowBgAlpha(0.75f);  // 半透明背景

    // 创建FPS显示窗口
    ImGuiWindowFlags flags = 
        ImGuiWindowFlags_NoDecoration | 
        ImGuiWindowFlags_AlwaysAutoResize | 
        ImGuiWindowFlags_NoSavedSettings | 
        ImGuiWindowFlags_NoFocusOnAppearing | 
        ImGuiWindowFlags_NoNav |
        ImGuiWindowFlags_NoMove;

    if (ImGui::Begin("##FpsDisplay", nullptr, flags))
    {
        // 标题
        ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "FPS 计数器");
        ImGui::Separator();

        // 游戏FPS（仅在启用时显示）
        if (showGameFps)
        {
            ImVec4 gameFpsColor = gameFps >= 60.0f ? ImVec4(0.0f, 1.0f, 0.0f, 1.0f) :  // 绿色 >= 60
                                  gameFps >= 30.0f ? ImVec4(1.0f, 1.0f, 0.0f, 1.0f) :  // 黄色 >= 30
                                                     ImVec4(1.0f, 0.0f, 0.0f, 1.0f);   // 红色 < 30
            
            ImGui::Text("游戏:");
            ImGui::SameLine(80.0f);
            ImGui::TextColored(gameFpsColor, "%.1f FPS", gameFps);
            ImGui::SameLine(160.0f);
            ImGui::TextDisabled("(%.2f ms)", gameFrameTime);
        }

        // 覆盖层FPS（仅在启用时显示）
        if (showOverlayFps)
        {
            ImVec4 overlayFpsColor = overlayFps >= 60.0f ? ImVec4(0.0f, 1.0f, 0.0f, 1.0f) :
                                     overlayFps >= 30.0f ? ImVec4(1.0f, 1.0f, 0.0f, 1.0f) :
                                                           ImVec4(1.0f, 0.0f, 0.0f, 1.0f);
            
            ImGui::Text("覆盖层:");
            ImGui::SameLine(80.0f);
            ImGui::TextColored(overlayFpsColor, "%.1f FPS", overlayFps);
            ImGui::SameLine(160.0f);
            ImGui::TextDisabled("(%.2f ms)", overlayFrameTime);
        }

        // 总帧数（仅在显示游戏FPS时显示）
        if (showGameFps)
        {
            ImGui::Separator();
            ImGui::TextDisabled("总帧数: %llu", g_gameFpsCounter.GetFrameCount());
        }
    }
    ImGui::End();
}

// ============================================================================
// hkResize —— 窗口大小改变时重建渲染资源
// ============================================================================

HRESULT WINAPI hkResize(IDXGISwapChain* This, UINT BufferCount, UINT Width, UINT Height,
                        DXGI_FORMAT NewFormat, UINT SwapChainFlags)
{
    if (g_imguiState.dx11BackendInitialized)
    {
        ImGui_ImplDX11_Shutdown();
        g_imguiState.dx11BackendInitialized = false;
    }
    g_imguiReadyForRender = false;
    CleanupRenderTarget();

    const HRESULT hr = g_oResize(This, BufferCount, Width, Height, NewFormat, SwapChainFlags);
    if (SUCCEEDED(hr)) g_pSwapChain = This;
    return hr;
}

// ============================================================================
// DX11HOOK —— 安装虚表 Hook
// ============================================================================

bool DX11HOOK()
{
    if (g_hookInstalled) return true;

    // 等待 Unity 窗口
    for (int i = 0; i < 100 && !g_hWnd; ++i)
    {
        g_hWnd = FindWindowA("UnityWndClass", nullptr);
        if (!g_hWnd) Sleep(100);
    }
    if (!g_hWnd) { printf("[-] DX11HOOK: 未找到 Unity 窗口\n"); return false; }
    printf("[+] 找到 Unity 窗口: 0x%p\n", g_hWnd);

    // 创建临时 SwapChain 获取虚表
    DXGI_SWAP_CHAIN_DESC sd = {};
    sd.BufferCount = 2;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate = { 60, 1 };
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = g_hWnd;
    sd.SampleDesc.Count = 1;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    const D3D_FEATURE_LEVEL featureLevels[] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0 };
    IDXGISwapChain*      tmpSC  = nullptr;
    ID3D11Device*        tmpDev = nullptr;
    ID3D11DeviceContext* tmpCtx = nullptr;
    D3D_FEATURE_LEVEL    fl;

    const HRESULT hr = D3D11CreateDeviceAndSwapChain(
        nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0,
        featureLevels, 2, D3D11_SDK_VERSION, &sd,
        &tmpSC, &tmpDev, &fl, &tmpCtx);

    if (FAILED(hr) || !tmpSC) { printf("[-] 创建临时 DX11 设备失败: 0x%x\n", hr); return false; }

    g_vtableSwapChain = *reinterpret_cast<DWORD64**>(tmpSC);
    if (!g_vtableSwapChain)
    {
        tmpCtx->Release(); tmpDev->Release(); tmpSC->Release();
        printf("[-] 获取虚表失败\n"); return false;
    }

    g_oPresent = reinterpret_cast<PresentFn>(g_vtableSwapChain[8]);
    g_oResize  = reinterpret_cast<ResizeFn> (g_vtableSwapChain[13]);

    if (!g_oPresent || !g_oResize)
    {
        tmpCtx->Release(); tmpDev->Release(); tmpSC->Release();
        printf("[-] 获取原始函数指针失败\n"); return false;
    }

    // 修改虚表
    DWORD protect = 0;
    if (!VirtualProtect(g_vtableSwapChain, sizeof(void*) * 14, PAGE_EXECUTE_READWRITE, &protect))
    {
        tmpCtx->Release(); tmpDev->Release(); tmpSC->Release();
        printf("[-] VirtualProtect 失败\n"); return false;
    }
    g_vtableSwapChain[8]  = reinterpret_cast<DWORD64>(hkPresent);
    g_vtableSwapChain[13] = reinterpret_cast<DWORD64>(hkResize);
    VirtualProtect(g_vtableSwapChain, sizeof(void*) * 14, protect, &protect);

    tmpCtx->Release(); tmpDev->Release(); tmpSC->Release();

    InitializeGlobals();
    g_hookInstalled = true;

    printf("[+] DX11 Hook 安装成功！按 INSERT 键开关菜单\n");
    return true;
}
