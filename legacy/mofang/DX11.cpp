#define IMGUI_DEFINE_MATH_OPERATORS
#include"engine.h"
#include"DX11.h"
#include<d3d11.h>
#include"class.h"
#include"base.h"
#include"struct.h"
#include "fuction.h"
#include "menu.h"
#include "hook.h"
#include"aimbot.h"
#include"ImGui/imgui_impl_dx11.h"
#include"ImGui/imgui.h"
#include"ImGui/imgui_impl_win32.h"
#include"ImGui/imgui_internal.h"
#include "imgui/imgui_freetype.h"
#include<tuple>
#include <iostream>
#include <string>
#include <fstream>
#include "font.h"
#include "image.h"
#pragma comment(lib, "D3DX11.lib")
#include <tchar.h>
#include "globals.h"
#include "FileHandler.hpp"

#include <filesystem>

namespace fs = std::filesystem;

std::tuple<float, float, float> rainbowColor(float time, float speed)
{
    constexpr float pi = 3.1415926f;
    return std::make_tuple(std::sin(speed * time) * 0.5f + 0.5f,
        std::sin(speed * time + 2 * pi / 3) * 0.5f + 0.5f,
        std::sin(speed * time + 4 * pi / 3) * 0.5f + 0.5f);
}



static ID3D11Device* g_pd3dDevice = NULL;
static ID3D11DeviceContext* g_pd3dDeviceContext = NULL;
static IDXGISwapChain* g_pSwapChain = NULL;
static ID3D11RenderTargetView* g_mainRenderTargetView = NULL;






namespace font
{
    ImFont* poppins_medium = nullptr;
    ImFont* poppins_medium_low = nullptr;
    ImFont* tab_icon = nullptr;
    ImFont* chicons = nullptr;
    ImFont* tahoma_bold = nullptr;
    ImFont* tahoma_bold2 = nullptr;
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




D3DX11_IMAGE_LOAD_INFO info; 
ID3DX11ThreadPump* pump{ nullptr };

//DWORD picker_flags = ImGuiColorEditFlags_NoSidePreview | ImGuiColorEditFlags_AlphaBar | ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_AlphaPreview;


void ImRotateStart()
{
    rotation_start_index = ImGui::GetWindowDrawList()->VtxBuffer.Size;
}



ImVec2 ImRotationCenter()
{
    ImVec2 l(FLT_MAX, FLT_MAX), u(-FLT_MAX, -FLT_MAX); // bounds

    const auto& buf = ImGui::GetWindowDrawList()->VtxBuffer;
    for (int i = rotation_start_index; i < buf.Size; i++)
        l = ImMin(l, buf[i].pos), u = ImMax(u, buf[i].pos);

    return ImVec2((l.x + u.x) / 2, (l.y + u.y) / 2); // or use _ClipRectStack?
}



void ImRotateEnd(float rad, ImVec2 center = ImRotationCenter())
{
    float s = sin(rad), c = cos(rad);
    center = ImRotate(center, s, c) - center;

    auto& buf = ImGui::GetWindowDrawList()->VtxBuffer;
    for (int i = rotation_start_index; i < buf.Size; i++)
        buf[i].pos = ImRotate(buf[i].pos, s, c) - center;
}



void Particles()
{
    ImVec2 screen_size = { (float)GetSystemMetrics(SM_CXSCREEN), (float)GetSystemMetrics(SM_CYSCREEN) };

    static ImVec2 partile_pos[100];
    static ImVec2 partile_target_pos[100];
    static float partile_speed[100];
    static float partile_radius[100];


    for (int i = 1; i < 50; i++)
    {
        if (partile_pos[i].x == 0 || partile_pos[i].y == 0)
        {
            partile_pos[i].x = rand() % (int)screen_size.x + 1;
            partile_pos[i].y = 15.f;
            partile_speed[i] = 1 + rand() % 25;
            partile_radius[i] = rand() % 4;

            partile_target_pos[i].x = rand() % (int)screen_size.x;
            partile_target_pos[i].y = screen_size.y * 2;
        }

        partile_pos[i] = ImLerp(partile_pos[i], partile_target_pos[i], ImGui::GetIO().DeltaTime * (partile_speed[i] / 60));

        if (partile_pos[i].y > screen_size.y)
        {
            partile_pos[i].x = 0;
            partile_pos[i].y = 0;
        }

        ImGui::GetWindowDrawList()->AddCircleFilled(partile_pos[i], partile_radius[i], ImColor(71, 226, 67, 255 / 2));
    }

}









//����ָ��
typedef  HRESULT(WINAPI* Present)(IDXGISwapChain* This, UINT SyncInterval, UINT Flags);
typedef  LRESULT(WINAPI* WndProc)(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
typedef  HRESULT(WINAPI* Resize)(IDXGISwapChain* This, UINT BufferCount, UINT Width, UINT Height, DXGI_FORMAT NewFormat, UINT SwapChainFlags);



HRESULT STDMETHODCALLTYPE Init(IDXGISwapChain* This, UINT SyncInterval, UINT Flags);




HWND g_hwnd = nullptr;
DWORD64* Vtb = nullptr;
Present oPresent = nullptr;
WndProc oWndProc = nullptr;
Resize oResize = nullptr;





Popup popup;


void GetDX11ptr(IDXGISwapChain* This);



HRESULT WINAPI hkPresent(IDXGISwapChain* This, UINT SyncInterval, UINT Flags)
{

    // ���� tab_size ��ֵ��ͨ�����Բ�ֵ���� tab_opening ��״̬�� 0 �� 160.f ֮���л�
    tab_size = ImLerp(tab_size, tab_opening ? 160.f : 0.f, ImGui::GetIO().DeltaTime * 12.f);
    // ���� arrow_roll ��ֵ������ tab_opening �� tab_size �������������Բ�ֵ
    arrow_roll = ImLerp(arrow_roll, tab_opening && (tab_size >= 159) ? 1.f : -1.f, ImGui::GetIO().DeltaTime * 12.f);

    // ��ʼ�� ImGui �� DX11 �����֡
    ImGui_ImplDX11_NewFrame();
    // ��ʼ�� ImGui �� Win32 �����֡
    ImGui_ImplWin32_NewFrame();
    // ��ʼ ImGui ����֡
    ImGui::NewFrame();
    {
        if (Options.Open_menu)
        {

             
            if (!isFirstAuto && !isFirstAutoCheck) {
                if (FileHandler::loadConfigAuto("Rconfig_Auto.JSON", isAutoConfig, isFirstAutoCheck) && fs::exists("Rconfig_Auto.JSON") && !fs::is_empty("Rconfig_Auto.JSON")) {
                    if (fs::exists("Rconfig.JSON") && !fs::is_empty("Rconfig.JSON")) {
                        // ����������һ��������ͨ�����ļ��ĺ������˴���Ϊʾ��
                        FileHandler::LoadConfig("Rconfig.JSON");
                        std::cout << "zddqSucces,Rconfig.JSON" << std::endl;
                        isFirstAuto = true;
                    }
                }
            }
           
      
            if (Options.Auto_Read_Config) {
                if (!isFirstAutoCheck) {
                    isFirstAutoCheck = true;
                    std::cout << "zddqSucces Is_First_Auto_check = true,Rconfig_Auto.JSON" << std::endl;
                    FileHandler::saveConfigAuto("Rconfig_Auto.JSON", isAutoConfig, isFirstAutoCheck);
                }
            }
            else {
                if (isFirstAutoCheck) {
                    isFirstAutoCheck = false;
                    isAutoConfig = false;
                    std::cout << "zddqSucces Is_First_Auto_check = false,Rconfig_Auto.JSON" << std::endl;
                    FileHandler::saveConfigAuto("Rconfig_Auto.JSON", isAutoConfig, isFirstAutoCheck);

                    auto status = FileHandler::removeFile("Rconfig_Auto.JSON");
                    switch (status) {
                    case FileHandler::FileStatus::NOT_EXISTS:
                        std::cout << "����ʧ�ܣ��㻹û������κ�����\n" << std::endl;
                        popup.Show("����ʧ�ܣ��㻹û������κ����ã�Rconfig_Auto.JSON\n", IM_COL32(255, 128, 0, 255), IM_COL32(0, 0, 0, 255)); // �Զ�����ɫ��������ɫ
                        break;
                    case FileHandler::FileStatus::EMPTY:
                        std::cout << u8"����ʧ�ܣ��������ļ������ļ�������\n" << std::endl;
                        popup.Show("����ʧ�ܣ��������ļ������ļ�������\n", IM_COL32(255, 128, 0, 255), IM_COL32(0, 0, 0, 255)); // �Զ�����ɫ��������ɫ
                        break;
                    case FileHandler::FileStatus::NON_EMPTY_DELETED_SUCCESSFULLY:
                        std::cout << u8"���óɹ�,����ע����Ч\n" << std::endl;
                        popup.Show("�رճɹ�,�ؽ��󲻻����Զ���ȡ���ã�Rconfig_Auto.JSON\n", IM_COL32(255, 128, 0, 255), IM_COL32(0, 0, 0, 255)); // �Զ�����ɫ��������ɫ
                        break;
                    case FileHandler::FileStatus::NON_EMPTY_DELETION_FAILED:
                        std::cout << u8"����ʧ�ܣ�����ϵ����Ա\n" << std::endl;
                        popup.Show("����ʧ�ܣ�����ϵ����Ա��Rconfig_Auto.JSON\n", IM_COL32(255, 128, 0, 255), IM_COL32(0, 0, 0, 255)); // �Զ�����ɫ��������ɫ
                        break;
                    }
                    popup.DrawPopup();
                }
               
                    

            }

        
        
         

            
      


            ImGuiStyle* s = &ImGui::GetStyle();

            // ���� ImGui ������ʽ
            s->WindowPadding = ImVec2(0, 0), s->WindowBorderSize = 0;
            s->ItemSpacing = ImVec2(20, 20);
            s->ScrollbarSize = 8.f;

            // �ڱ��������б������ͼ��
            ImGui::GetBackgroundDrawList()->AddImage(image::bg, ImVec2(0, 0), ImVec2(1920, 1080), ImVec2(0, 0), ImVec2(1, 1), ImColor(255, 255, 255, 255));

            // ������һ�� ImGui ���ڵĴ�С
            ImGui::SetNextWindowSize(ImVec2(c::bg::size) + ImVec2(tab_size, 0));

            // ��ʼһ�� ImGui ���ڣ����ô��ڱ�־Ϊ��װ�Ρ����ɵ�����С�Ҳ����ڻ�ý���ʱǰ��
            ImGui::Begin("IMGUI MENU", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoBringToFrontOnFocus);
            {

                

                



                const ImVec2& pos = ImGui::GetWindowPos();
                const auto& p = ImGui::GetWindowPos();
                const ImVec2 spacing = ImGui::GetStyle().ItemSpacing;
               

                // �ڱ��������б�����������Σ����崰�ڱ���
                ImGui::GetBackgroundDrawList()->AddRectFilled(pos, pos + ImVec2(c::bg::size) + ImVec2(tab_size, 0), ImGui::GetColorU32(c::bg::background), c::bg::rounding);
                // �ڱ��������б�����Ӿ��α߿򣬶��崰������
                ImGui::GetBackgroundDrawList()->AddRect(pos, pos + ImVec2(c::bg::size) + ImVec2(tab_size, 0), ImGui::GetColorU32(c::bg::outline_background), c::bg::rounding);

                // �����ı���ɫ��ʽΪǿ���ı���ɫ
                ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetColorU32(c::accent_text_color));

                // ��������Ϊ tahoma_bold2�����Ʋü����ı�"UC-Menu"
                ImGui::PushFont(font::tahoma_bold2); ImGui::RenderTextClipped(pos + ImVec2(60, 0) + spacing, pos + spacing + ImVec2(60, 60) + ImVec2(tab_size + (spacing.x / 2) - 30, 0), "AndThen", NULL, NULL, ImVec2(0.5f, 0.5f), NULL); ImGui::PopFont();

                // �ü�����ı�"Lifetime"��"hefan2429"���ض�λ��
                ImGui::RenderTextClipped(pos + ImVec2(60 + spacing.x, c::bg::size.y - 60 * 2), pos + spacing + ImVec2(60, c::bg::size.y) + ImVec2(tab_size, 0), "ʣ��ʱ��_2099-9-9", NULL, NULL, ImVec2(0.0f, 0.43f), NULL);
                ImGui::RenderTextClipped(pos + ImVec2(60 + spacing.x, c::bg::size.y - 60 * 2), pos + spacing + ImVec2(60, c::bg::size.y) + ImVec2(tab_size, 0), "Ȼ����\n", NULL, NULL, ImVec2(0.0f, 0.57f), NULL);

                ImGui::PushFont(font::tahoma_bold2); ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(250, 255, 255, 255));
                // �ü�����ı�"Hello, UC-Menu"���ض�λ��
                ImGui::RenderTextClipped(pos + ImVec2(0, 0) + spacing, pos + spacing + ImVec2(60, 40) + ImVec2(tab_size + (spacing.x / 2) + 199, 0), "Hello, User-241013", NULL, NULL, ImVec2(1.f, 0.5f), NULL); ImGui::PopFont(); ImGui::PopStyleColor();

                // �ڱ��������б������ͼ��
                ImGui::GetBackgroundDrawList()->AddImage(image::logo, pos + ImVec2(10, 10), pos + ImVec2(10, 10), ImVec2(100, 100), ImVec2(100, 100), ImColor(255, 255, 255, 255));

                ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(90, 93, 100, 255)); ImGui::RenderTextClipped(pos + ImVec2(30, 20) + spacing, pos + spacing + ImVec2(60, 80) + ImVec2(tab_size + (spacing.x / 2) + 108, -20), "��ӭ����!", NULL, NULL, ImVec2(1.f, 0.5f), NULL); ImGui::PopStyleColor();

                // ����һ����̬�ַ����飬���ڴ洢��������
                static char Search[64] = { "" };
                // ���ù��λ�ã����� tab_size ������
                ImGui::SetCursorPos(ImVec2(385 + tab_size, -20) + (s->ItemSpacing * 2));
                // ��ʼһ���Ӵ��ڣ�û���ض����ƣ�����Ϊ" "
                ImGui::BeginChild("", " ", ImVec2((c::bg::size.x - 60 - s->ItemSpacing.x * 4) / 2, 60));
                ImGui::PushFont(font::tab_icon);
                // �����ı�"I"��"H"��"G"
                ImGui::Text("I"); ImGui::SameLine(); ImGui::Text("H"); ImGui::SameLine(); ImGui::Text("G");
                // ImGui::GetWindowDrawList()->AddText(pos + ImVec2(600, 36), ImColor(90, 93, 100, 255), "I");
                // ImGui::GetWindowDrawList()->AddText(pos + ImVec2(635, 36), ImColor(90, 93, 100, 255), "H");
                // ImGui::GetWindowDrawList()->AddText(pos + ImVec2(670, 36), ImColor(90, 93, 100, 255), "G");
                ImGui::PopFont(); ImGui::SameLine();
                // ������һ����Ŀ��Ϊ 180
                ImGui::SetNextItemWidth(180);
                // ����һ�������ı�����ʾΪ"Search function"���洢�� Search �����У���󳤶�Ϊ 64
                ImGui::InputTextWithHint("��ѯ����", "��δʵ��...", Search, 64, NULL);
                ImGui::EndChild();
                ImGui::PopStyleColor(1);

                // ����һ���ַ�ָ�����飬���ڴ洢��ǩ����
                const char* nametab_array1[6] = { "E", "D", "A", "G", "C","B" };  //F��B�Ե���  ��F�ĳ���G��ʵ���Ѿ�û��

                // ��һ���ַ�ָ�����飬���ڴ洢��ͬ�ı�ǩ�ı�
                const char* nametab_array[6] = { "��׼", "�Ӿ�", "��ɫ","�ڴ�", "����","����" };
                // ��һ���ַ�ָ�����飬���ڴ洢��ǩ����ʾ�ı�
                const char* nametab_hint_array[6] = { "Aim, Recoil, Trigger", "Overlay, Chams, World", "Game Colors", "World Memory", "Element Setup", "Save Settings" };

                // ����һ����̬�������� tabs�����ڸ��ٵ�ǰѡ�еı�ǩ����
                static int tabs = 0;

                ImGui::SetCursorPos(ImVec2(spacing.x + (60 - 22) / 2, 60 + (spacing.y * 2) + 22));
                ImGui::BeginGroup();
                {
                    // ������ǩ���飬�������л��ı�ǩ������ѡ��״̬���� tabs ��ֵ
                    for (int i = 0; i < sizeof(nametab_array1) / sizeof(nametab_array1[0]); i++)
                        if (ImGui::Tab(i == tabs, nametab_array1[i], nametab_array[i], nametab_hint_array[i], ImVec2(35 + tab_size, 35))) tabs = i;
                }
                ImGui::EndGroup();

                ImGui::SetCursorPos(ImVec2(8, 9) + spacing);

                // ��ʼһ���Զ������ת����
                ImRotateStart();
                // ����һ���Զ��尴ť���������������л� tab_opening ��״̬
                if (ImGui::CustomButton(1, image::roll, ImVec2(35, 35), ImVec2(0, 0), ImVec2(1, 1), ImGui::GetColorU32(c::accent_color))) tab_opening = !tab_opening;
                // ������ת������������ arrow_roll ��ֵ�����ض��Ƕȵ���ת
                ImRotateEnd(1.57f * arrow_roll);

                // �ڱ��������б�����������Σ������ض�����ı���
                ImGui::GetBackgroundDrawList()->AddRectFilled(pos + ImVec2(0, -20 + spacing.y) + spacing, pos + spacing + ImVec2(60, c::bg::size.y - 60 - spacing.y * 3) + ImVec2(tab_size, 0), ImGui::GetColorU32(c::child::background), c::child::rounding);
                // �ڱ��������б�����Ӿ��α߿򣬶����ض����������
                ImGui::GetBackgroundDrawList()->AddRect(pos + ImVec2(0, -20 + spacing.y) + spacing, pos + spacing + ImVec2(60, c::bg::size.y - 60 - spacing.y * 3) + ImVec2(tab_size, 0), ImGui::GetColorU32(c::child::outline_background), c::child::rounding);

                // �ڱ��������б�����������Σ�������һ���ض�����ı���
                ImGui::GetBackgroundDrawList()->AddRectFilled(pos + ImVec2(0, c::bg::size.y - 60 - spacing.y * 2) + spacing, pos + spacing + ImVec2(60, c::bg::size.y - spacing.y * 2) + ImVec2(tab_size, 0), ImGui::GetColorU32(c::child::background), c::child::rounding);
                // �ڱ��������б�����Ӿ��α߿򣬶�����һ���ض����������
                ImGui::GetBackgroundDrawList()->AddRect(pos + ImVec2(0, c::bg::size.y - 60 - spacing.y * 2) + spacing, pos + spacing + ImVec2(60, c::bg::size.y - spacing.y * 2) + ImVec2(tab_size, 0), ImGui::GetColorU32(c::child::outline_background), c::child::rounding);

                // �ڴ��ڻ����б������ͼ��
                ImGui::GetWindowDrawList()->AddImage(image::logo, pos + ImVec2(0, c::bg::size.y - 60 - spacing.y * 2) + spacing + ImVec2(12, 12), pos + spacing + ImVec2(60, c::bg::size.y - spacing.y * 2) - ImVec2(12, 12), ImVec2(0, 0), ImVec2(1, 1), ImColor(255, 255, 255, 255));

                // �ڴ��ڻ����б���������Բ�Σ������ض�λ�ú���ɫ��Բ��
                ImGui::GetWindowDrawList()->AddCircleFilled(pos + ImVec2(63, c::bg::size.y - (spacing.y * 2) + 3), 10.f, ImGui::GetColorU32(c::child::background), 100.f);
                // �ڴ��ڻ����б���������Բ�Σ�������һ���ض�λ�ú���ɫ��Բ��
                ImGui::GetWindowDrawList()->AddCircleFilled(pos + ImVec2(63, c::bg::size.y - (spacing.y * 2) + 3), 6.f, ImColor(0, 255, 0, 255), 100.f);

                // ����һ����Ϊ Particles �ĺ��������幦��δ֪
                Particles();

                // ���徲̬������� tab_alpha��tab_add �;�̬�������� active_tab
                static float tab_alpha = 0.f; static float tab_add; static int active_tab = 0;

                // ���� tabs �� active_tab �Ĺ�ϵ���� tab_alpha ��ֵ���������� 0.f �� 1.f ֮��
                tab_alpha = ImClamp(tab_alpha + (4.f * ImGui::GetIO().DeltaTime * (tabs == active_tab ? 1.f : -1.f)), 0.f, 1.f);
                // ���� tabs �� active_tab �Ĺ�ϵ���� tab_add ��ֵ���������� 0.f �� 1.f ֮��
                tab_add = ImClamp(tab_add + (std::round(350.f) * ImGui::GetIO().DeltaTime * (tabs == active_tab ? 1.f : -1.f)), 0.f, 1.f);

                // ��� tab_alpha �� tab_add ��Ϊ 0.f������� active_tab ��ֵΪ tabs ��ֵ
                if (tab_alpha == 0.f && tab_add == 0.f) active_tab = tabs;

                // ������ʽ���� ImGuiStyleVar_Alpha������Ϊ tab_alpha ������ʽ�� Alpha ֵ
                ImGui::PushStyleVar(ImGuiStyleVar_Alpha, tab_alpha * s->Alpha);

               


                if (tabs == 0)
                {
                    ImGui::SetCursorPos(ImVec2(60 + tab_size, 60) + (s->ItemSpacing * 2));



                    ImGui::BeginGroup();
                    {
                        ImGui::BeginChild("D", "Main", ImVec2((c::bg::size.x - 60 - s->ItemSpacing.x * 4) + 18, 405));
                        {
                            Aim_bot01();
                        }
                        ImGui::EndChild();
                    }

                    
                   

                    ImGui::EndGroup();
                    ImGui::SameLine();
                    ImGui::BeginGroup();
                    {
                        ImGui::SetCursorPos(ImVec2(60 + tab_size, 480) + (s->ItemSpacing * 2));

                        ImGui::BeginChild("A", "Other", ImVec2((c::bg::size.x - 60 - s->ItemSpacing.x * 4) / 2, 100));
                        {
                            Aim_bot02();
                        }
                        ImGui::EndChild();

                        // ���ù��λ��

                        ImGui::SetCursorPos(ImVec2(380 + tab_size, 480) + (s->ItemSpacing * 2));


                        ImGui::BeginChild("E", "Setting", ImVec2((c::bg::size.x - 60 - s->ItemSpacing.x * 4) / 2, 100));
                        {
                            Aim_bot03();
                        }

                        ImGui::EndChild();
                    }
              
                    ImGui::EndGroup();
                }


                // ��� tabs ��ֵΪ 1
                if (tabs == 1)
                {
                    ImGui::SetCursorPos(ImVec2(60 + tab_size, 60) + (s->ItemSpacing * 2));
                    ImGui::BeginGroup();
                    {
                        ImGui::BeginChild("D", "Main", ImVec2((c::bg::size.x - 60 - s->ItemSpacing.x * 4) / 2, 420));
                        {
                            Esp01();
                            //���һ����ť�����������ķֺŵ��´����߼����⣬������Ҫ�������Ĵ����߼�
                           //if (ImGui::Button("�����޸�����", ImVec2(ImGui::GetContentRegionMax().x - s->WindowPadding.x, 25)))
                           //{
                           //    fix_bone = !fix_bone;

                           //    //printf("%s", covert_box_show);
                           //}
                        }
                        ImGui::EndChild();
                    }
                    ImGui::EndGroup();
                    ImGui::SameLine();

                    ImGui::BeginGroup();
                    {
                        ImGui::BeginChild("E", "Show", ImVec2((c::bg::size.x - 60 - s->ItemSpacing.x * 4) / 2, 420));
                        {
                            Esp02();
                        }
                        ImGui::EndChild();

                        ImGui::BeginChild("B", "Other", ImVec2((c::bg::size.x - 60 - s->ItemSpacing.x * 4) / 2, 80));
                        {
                            Esp03();
                        }
                        ImGui::EndChild();
                        // ���ù��λ��
                        ImGui::SetCursorPos(ImVec2(60 + tab_size, 500) + (s->ItemSpacing * 2));
                        // ��ʼһ����Ϊ"Team"���Ӵ���

                        ImGui::BeginChild("A", "Filter ", ImVec2((c::bg::size.x - 60 - s->ItemSpacing.x * 4) / 2, 80));
                        {
                            Esp04();
                        }

                        ImGui::EndChild();
                    }
                    ImGui::EndGroup();
                }

                if (tabs == 2)
                {
                    ImGui::SetCursorPos(ImVec2(60 + tab_size, 60) + (s->ItemSpacing * 2));
                    ImGui::BeginGroup();
                    {
                        ImGui::BeginChild("D", "Main", ImVec2((c::bg::size.x - 60 - s->ItemSpacing.x * 4) + 18, 445));
                        {
                            Color();
                        }

                    }
                    ImGui::EndChild();

                }
                ImGui::EndGroup();

                if (tabs == 3)
                {
                    ImGui::SetCursorPos(ImVec2(60 + tab_size, 60) + (s->ItemSpacing * 2));
                    ImGui::BeginGroup();
                    {
                        ImGui::BeginChild("D", "Main", ImVec2((c::bg::size.x - 60 - s->ItemSpacing.x * 4) + 18, 470));
                        {
                             Memory();
                        }

                    }
                    ImGui::EndChild();

                }
                ImGui::EndGroup();

                if (tabs == 4)
                {
                    ImGui::SetCursorPos(ImVec2(60 + tab_size, 60) + (s->ItemSpacing * 2));
                    ImGui::BeginGroup();
                    {
                        ImGui::BeginChild("D", "Main", ImVec2((c::bg::size.x - 60 - s->ItemSpacing.x * 4) + 18, 495));
                        {
                            Setting();
                        }

                    }
                    ImGui::EndChild();

                }
                ImGui::EndGroup();


                if (tabs == 5)
                {
                    ImGui::SetCursorPos(ImVec2(60 + tab_size, 60) + (s->ItemSpacing * 2));
                    ImGui::BeginGroup();
                    {
                        ImGui::BeginChild("D", "Main", ImVec2((c::bg::size.x - 60 - s->ItemSpacing.x * 4) + 18, 520));
                        {
                            // ��� tabs ���� 5���ڴ��Ӵ����п��ܻ��������������صĽ���Ԫ�أ���ĿǰΪ�գ������ɸ���������ӡ�


                             Configuration();
                             if (ImGui::Button("���ұ�������", ImVec2(ImGui::GetContentRegionMax().x - s->WindowPadding.x, 25)))
                             {
                                 
                                 FileHandler::SaveConfig("Rconfig.JSON");

                                 popup.Show("����ɹ���Rconfig.JSON\n", IM_COL32(255, 128, 0, 255), IM_COL32(0, 0, 0, 255)); // �Զ�����ɫ��������ɫ
                  
                             }
                             popup.DrawPopup();

                             if (ImGui::Button("���Ҷ�ȡ����", ImVec2(ImGui::GetContentRegionMax().x - s->WindowPadding.x, 25)))
                             {
                                 if (FileHandler::fileExists("Rconfig.JSON") && FileHandler::isFileEmpty("Rconfig.JSON") == false)
                                 {
                                     FileHandler::LoadConfig("Rconfig.JSON");
                                     popup.Show("��ȡ�ɹ���Rconfig.JSON\n", IM_COL32(255, 128, 0, 255), IM_COL32(0, 0, 0, 255)); // �Զ�����ɫ��������ɫ
                                 }
                                 else
                                 {
                                     popup.Show("���������ļ��Ƿ���� Ϊ�գ�Rconfig.JSON\n", IM_COL32(255, 128, 0, 255), IM_COL32(0, 0, 0, 255)); // �Զ�����ɫ��������ɫ
                                 }
                                 
                             }
                             popup.DrawPopup();
                          
                             if (ImGui::Button("��������", ImVec2(ImGui::GetContentRegionMax().x - s->WindowPadding.x, 25)))
                             {
                                 auto status = FileHandler::removeFile("Rconfig.JSON");
                                 switch (status) {
                                 case FileHandler::FileStatus::NOT_EXISTS:
                                     std::cout << "����ʧ�ܣ��㻹û������κ�����\n";
                                     popup.Show("����ʧ�ܣ��㻹û������κ�����\n", IM_COL32(255, 128, 0, 255), IM_COL32(0, 0, 0, 255)); // �Զ�����ɫ��������ɫ
                                     break;
                                 case FileHandler::FileStatus::EMPTY:
                                     std::cout << "����ʧ�ܣ��������ļ������ļ�������\n";
                                     popup.Show("����ʧ�ܣ��������ļ������ļ�������\n", IM_COL32(255, 128, 0, 255), IM_COL32(0, 0, 0, 255)); // �Զ�����ɫ��������ɫ
                                     break;
                                 case FileHandler::FileStatus::NON_EMPTY_DELETED_SUCCESSFULLY:
                                     std::cout << "���óɹ�,����ע����Ч\n";
                                     popup.Show("���óɹ�,����ע����Ч��Rconfig.JSON\n", IM_COL32(255, 128, 0, 255), IM_COL32(0, 0, 0, 255)); // �Զ�����ɫ��������ɫ
                                     break;
                                 case FileHandler::FileStatus::NON_EMPTY_DELETION_FAILED:
                                     std::cout << "����ʧ�ܣ�����ϵ����Ա\n";
                                     popup.Show("����ʧ�ܣ�����ϵ����Ա��Rconfig.JSON\n", IM_COL32(255, 128, 0, 255), IM_COL32(0, 0, 0, 255)); // �Զ�����ɫ��������ɫ
                                     break;
                                 }

                                 
                             }
                             popup.DrawPopup();




                        }
                        ImGui::EndChild();
                    }
         
                   

                    ImGui::EndGroup();
                }
                // ����֮ǰ�������ʽ����
                ImGui::PopStyleVar();
            }
            ImGui::End();
        }
        /*----------------------------------------------*/



        UWorld* World = GetWorld();



        if (World && World->GameState)
        {

            TArray<AActor*>& Actors = World->PersistentLevel->Actors;


            if (IsF1 == false && First_Initialization_Check == nullptr)
            {
                First_Initialization_Check = World->GameState;

                //printf("First_Initialization_Check: %p\n", First_Initialization_Check);
                IsF1 = true;
            }

            if (First_Initialization_Check != World->GameState)
            {
                xmmData = nullptr;
                Enemy_ = nullptr;
                choose_aim = 0;
                First_Initialization_Check = nullptr;
                IsF1 = false;
                //printf("First_Initialization_Check New: %p\n", First_Initialization_Check);
            }

            /*        if (IsF1 == false && First_Initialization_Check == nullptr) {
                        First_Initialization_Check = World->GameState;
                        IsF1 = true;
                    }

                    if (First_Initialization_Check != World->GameState  && IsF1 == true) {
                        Enemy = nullptr;
                        choose_aim = 0;
                        First_Initialization_Check = nullptr;
                        IsF1 = false;
                    }*/



            if (World->PersistentLevel && Actors.Count > 200 && World->OwningGameInstance && First_Initialization_Check == World->GameState)
            {



                for (size_t i = 0; i < Actors.Count; i++)
                {
                    AActor* Actor = Actors.Data[i];




                    if (!Actor || !(Actor->RootComponent)) //�����ָ��
                        continue;


                    if (Actor->GetName().find("MainMenuPlayerController_C") != string::npos)  //�ж��Ƿ�λ����Ϸ�⣬����״̬��
                        continue;

                   //printf(u8"ָ�롢���� = %p, %s\n", Actor, Actor->GetName().c_str());




                    AActor* Player = World->OwningGameInstance->GetPlayer();

                    if (!Player || Player == Actor) //�����ָ�� �����Լ�
                        continue;

                    //printf("cs");


                    if (Actor->GetName().find("Zombie_BP") == string::npos && Actor->GetName().find("Chinese_Vampire") == string::npos && !Options.Debug_character_name_ALL) //��������
                        continue;

                    if (Options.Filtering_out_death && !Options.Debug_character_name_ALL)
                    {
                        //printf(u8"����Ѫ����%f \n", Actor->Health);

                        if ((Actor->Health <= 0)) //����Ѫ��Ϊ0��
                        {
                            continue;
                        }

                    }

                    if (getObjectDistance(Player->RootComponent->Location, Actor->RootComponent->Location) > Options.Can_DrawDistance)
                        continue;


                    /*if (First_Initialization_Check != Enemy)
                    {
                        Enemy = nullptr;
                        printf("First_Initialization_Check = Enemy\n");
                    }*/



                    if (Options.BulletAim || Options.TGSoAS)
                    {
                        Enemy_ = Actor;
                        player_ = Player;
                    }















                    if (Options.Debug_character_name)
                    {
                        Vector3 Pos = Actor->RootComponent->Location;
                        Vector2 Screen{ 0 };
                        if (WorldToScreen(Pos, Screen)) {
                            char buffer[256];
                            sprintf(buffer, "%p", Actor);
                            // ��ȡ Actor ������������
                            int Debug_Class_Void_Color = ImColor(Debug_Class_Void_ColorN[0], Debug_Class_Void_ColorN[1], Debug_Class_Void_ColorN[2], Debug_Class_Void_ColorN[3]);
                            string className = Actor->GetName();
                            ImGui::GetForegroundDrawList()->AddText({ Screen.X, Screen.Y }, Debug_Class_Void_Color, className.c_str());
                        }
                    }



                    if (Options.Debug_Class_Void)
                    {

                        Vector3 Pos = Actor->RootComponent->Location;
                        Vector2 Screen{ 0 };
                        if (WorldToScreen(Pos, Screen))
                        {
                            char buffer[256];
                            sprintf(buffer, "%p", Actor);
                            int Debug_Class_Void_Color = ImColor(Debug_Class_Void_ColorN[0], Debug_Class_Void_ColorN[1], Debug_Class_Void_ColorN[2], Debug_Class_Void_ColorN[3]);
                            ImGui::GetForegroundDrawList()->AddText({ Screen.X, Screen.Y + 20 }, Debug_Class_Void_Color, buffer);
                            //ImGui::GetForegroundDrawList()->AddText({ Screen.X, Screen.Y + 20 }, getImColorFromImVec4(Debug_Class_Void_Color), to_string(getObjectDistance(Player->RootComponent->Location, Actor->RootComponent->Location)).c_str());
                        }
                    }


                    if (Options.Debug_Class_ID)
                    {

                        Vector3 Pos = Actor->RootComponent->Location;
                        Vector2 Screen{ 0 };
                        if (WorldToScreen(Pos, Screen))
                        {
                            char buffer[256];
                            char buffer1[256];
                            sprintf(buffer, "����RootComponent��ActorID   0x%X\n", Actor->RootComponent->ActorID);
                            sprintf(buffer1, "���� Actor��ActorID    0x%X\n", Actor->ActorID);

                            int Debug_Class_Void_Color = ImColor(Debug_Class_Void_ColorN[0], Debug_Class_Void_ColorN[1], Debug_Class_Void_ColorN[2], Debug_Class_Void_ColorN[3]);
                            ImGui::GetForegroundDrawList()->AddText({ Screen.X, Screen.Y + 40 }, Debug_Class_Void_Color, buffer);
                            ImGui::GetForegroundDrawList()->AddText({ Screen.X, Screen.Y + 80 }, Debug_Class_Void_Color, buffer1);
                            //ImGui::GetForegroundDrawList()->AddText({ Screen.X, Screen.Y + 20 }, getImColorFromImVec4(Debug_Class_Void_Color), to_string(getObjectDistance(Player->RootComponent->Location, Actor->RootComponent->Location)).c_str());
                        }




                    }



                    if (Options.MemoryAmmon_num)
                    {
                        Player->Weapon->BulletCount = Options.Ammon_num;
                        Player->Weapon->BulletClipCount = Options.Ammon_Clip_num;

                        Player->Weapon->LensStabilization = Options.NoCameraShake_;
                        Player->Weapon->SightDiffusion = Options.SightDiffusion_;
                        Player->Weapon->HorizontalRecoil = Options.SwayLeftAndRight_;


                        Player->Weapon->RapidFireSpeed = Options.RapidFire_;
                        // Player->Weapon->VerticalRecoilWithoutScope = Options.VerticalRecoilWithoutScope_;



                    }


                    if (Options.NoRecoil)
                    {
                        Player->Weapon->NoRecoilEnabled = true;
                        Player->Weapon->LensStabilization = 0.f;
                        Player->Weapon->HorizontalRecoil = 0.f;
                        Player->Weapon->LiftGunRecoilSize = 0.f;
                        Player->Weapon->SightDiffusion = 0.f;

                    }
                    else
                    {
                        Player->Weapon->NoRecoilEnabled = false;
                        Player->Weapon->LensStabilization = Options.NoCameraShake_;
                        Player->Weapon->SightDiffusion = Options.SightDiffusion_;
                        Player->Weapon->HorizontalRecoil = Options.SwayLeftAndRight_;
                        Player->Weapon->LiftGunRecoilSize = 0.f;
                    }

                    /* printf("Actor->HitPlayer:%p", Actor->HitPlayer);
                     printf("Actor->PlayerRef:%p", Actor->PlayerRef);*/


                     //printf("Actor->PlayerRef:%p\n", Actor->PlayerRef);
                     //printf("Actor->HitPlayer:%p\n", Actor->HitPlayer);
                     //printf("player_:%p\n", player_);


                     //if (player_ != 0&&Actor != player_)
                     //{
                     //    //Actor->PlayerRef = player_;
                     //    Actor->HitPlayer = player_;

                     //}

      
                    if(Options.KS>0)
                    {
                        Player->PlayerState->KS = Options.KS;
                    }
                    if(Options.TNBF>0)
                    {
                        Player->PlayerState->TNBF = Options.TNBF;
                    }
                    if(Options.TNBFZ>0)
                    {
                        Player->PlayerState->TNBFZ = Options.TNBFZ;
                    }
                    if(Options.TNBFZH>0)
                    {
                        Player->PlayerState->TNBFZH = Options.TNBFZH;
                    }


                    if (Options.HSR_)
                    {
                       
                        Player->PlayerState->TNBFZH = Player->PlayerState->TNBF;
                    }

                    if(Options.HR_)
                    {
                        Player->PlayerState->TNBFZ = Player->PlayerState->TNBF;
                        Player->PlayerState->TNBFZH = Player->PlayerState->TNBF;
                    }




                    if (!Options.Debug_character_name_ALL)
                    {

                        FUCTION RenderObject(Player, Actor);

                        // ��ȡ��������
                        BoneIdx Idx{ 0 };
                        RenderObject.GetBoneIdx(Actor, &Idx);

                        bool idxHeadProcessed = false;



                   









                        // ���©������������й����жϿɼ��ԣ������ȿ���ͷ��������

                        if (select_aim_location == 0)
                        {
                            choose_aim = Idx.head;
                        }
                        else if ((select_aim_location == 1))
                        {
                            choose_aim = Idx.neck_01;
                        }
                        else if (select_aim_location == 2)
                        {
                            choose_aim = Idx.spine_03;
                        }
                        else if (select_aim_location == 3)
                        {
                            choose_aim = Idx.pelvis;
                        }

                        if (Options.Missed_shot) {



                            int boneCount = static_cast<int>(Actor->Mesh->BoneTransForm.Count);

                            for (int i = 0; i < boneCount; ++i) {
                                bool IsVisible = LineTraceSingle(Player->Controller->PlayerCameraManager->CameraPos, GetBoneMatrix(Actor->Mesh, i), Actor);
                                if (!IsVisible && i == choose_aim) {
                                    Aimbot::GetInstance().Push(Player, Actor, choose_aim);

                                    idxHeadProcessed = true;
                                    break;
                                }
                            }

                            // ���û���ҵ�choose_aim�����㲻�ɼ����������������������
                            if (!idxHeadProcessed) {
                                for (int i = 0; i < boneCount; ++i) {
                                    bool IsVisible = LineTraceSingle(Player->Controller->PlayerCameraManager->CameraPos, GetBoneMatrix(Actor->Mesh, i), Actor);
                                    if (!IsVisible) {
                                        Aimbot::GetInstance().Push(Player, Actor, i);
                                        break;
                                    }
                                }
                            }
                        }
                        else {
                            // ©��δ�����ֻ���choose_aim������
                            int boneCount = static_cast<int>(Actor->Mesh->BoneTransForm.Count);
                            for (int i = 0; i < boneCount; ++i) {
                                if (Player && Player->Controller && Player->Controller->PlayerCameraManager && Actor && Actor->Mesh) {

                                    bool IsVisible = LineTraceSingle(Player->Controller->PlayerCameraManager->CameraPos, GetBoneMatrix(Actor->Mesh, i), Actor);
                                    if (!IsVisible && i == choose_aim) {
                                        Aimbot::GetInstance().Push(Player, Actor, choose_aim);
                                        idxHeadProcessed = true;
                                        break;
                                    }
                                }

                            }
                        }



                        if (Options.Enemies_Freeze)
                        {
                            Actor->Global_speed = 0.0f;
                            //Player->PlayerState->HSR = 100.0f;
                            Player->PlayerState->KS = 1000000.f;
                           // Player->PlayerState->PT = 1.f;
                            Player->PlayerState->TNBF = 100.f; 
                           Player->PlayerState->TNBFZH = 100.f;
                           Player->PlayerState->TNBFZ = 100.f;



                        }
                        else
                        {
                            Actor->Global_speed = 1.0f;
                        }

                        int Bone_Color_Occlusion = ImColor(Bone_Occlusion_ColorN[0], Bone_Occlusion_ColorN[1], Bone_Occlusion_ColorN[2], Bone_Occlusion_ColorN[3]);
                        int Bone_Color_Unobstructed = ImColor(Bone_Unobstructed_ColorN[0], Bone_Unobstructed_ColorN[1], Bone_Unobstructed_ColorN[2], Bone_Unobstructed_ColorN[3]);
                        int AimBot_Color_UnOcclusiond = ImColor(AimBot_unObstructed_ColorN[0], AimBot_unObstructed_ColorN[1], AimBot_unObstructed_ColorN[2], AimBot_unObstructed_ColorN[3]);


                        int Radar_Color_Occlusion  = ImColor(Radar_Occlusion_ColorN[0], Radar_Occlusion_ColorN[1], Radar_Occlusion_ColorN[2], Radar_Occlusion_ColorN[3]);
                        int Radar_Color_Unobstructed = ImColor(Radar_unObstructed_ColorN[0], Radar_unObstructed_ColorN[1], Radar_unObstructed_ColorN[2], Radar_unObstructed_ColorN[3]);

                        ImColor RadarActorColor = LineTraceSingle(Player->Controller->PlayerCameraManager->CameraPos, Actor->RootComponent->Location, Actor) ? Bone_Color_Occlusion : (Actor == Enemy_lock ? AimBot_Color_UnOcclusiond : Bone_Color_Unobstructed);
                        // bool IsVisible_ = LineTraceSingle(Player->Controller->PlayerCameraManager->CameraPos, );







                        //���ƹ���
                        if (Options.boneOn)
                        {

                            


                            //RenderObject.DrawBone(Actor, &Idx, Bone_Color_Occlusion, AimBot_Color_Unobstructed);

                            RenderObject.DrawBone(Actor, &Idx, Bone_Color_Occlusion, Bone_Color_Unobstructed, AimBot_Color_UnOcclusiond,Enemy_lock);



                        }
                        if (Options.DrawRange)
                        {
                            int Range_color_ = ImColor(Range_color[0], Range_color[1], Range_color[2], Range_color[3]);
                            ImGui::GetForegroundDrawList()->AddCircle({ Width,Height }, Options.Aim_Range, Range_color_);
                        }

                        if (Options.DrawRadar)
                        {
                            //int Radar_Enemy_color = ImColor(Radar_Enemy_colorN[0], Radar_Enemy_colorN[1], Radar_Enemy_colorN[2], Radar_Enemy_colorN[3]);
                            RenderObject.DrawRadar(RadarActorColor);
                        }
                        if (Options.DrawSnapLine)
                        {

                            int SnapLineValue = ImColor(SnapLine_colorN[0], SnapLine_colorN[1], SnapLine_colorN[2], SnapLine_colorN[3]);
                            RenderObject.DrawSnapLine(SnapLineValue);
                        }


                        if (Options.DrawCharacterName)
                        {
                            int NameValue = ImColor(Name_colorN[0], Name_colorN[1], Name_colorN[2], Name_colorN[3]);
                            RenderObject.DrawName(NameValue);
                        }


                        if (Options.DrawBox2D)
                        {
                            int Box2DValue = ImColor(box2D_colorN[0], box2D_colorN[1], box2D_colorN[2], box2D_colorN[3]);
                            RenderObject.DrawBox2D(Box2DValue);
                        }

                        if (Options.DrawBlood)
                        {
                            RenderObject.DrawBlood(ImColor(0, 255, 0));
                        }

                        if (Options.DrawDistance)
                        {

                            int DistanceValue = ImColor(Distance_colorN[0], Distance_colorN[1], Distance_colorN[2], Distance_colorN[3]);

                            RenderObject.DrawDistance(DistanceValue);
                        }

                        if (Options.DrawLosLine)
                        {
                            int LosLineValue = ImColor(LosLine_colorN[0], LosLine_colorN[1], LosLine_colorN[2], LosLine_colorN[3]);
                            RenderObject.DrawLosLine(LosLineValue);
                        }
                            

                        if (Options.DrawBox3D)
                        {
                            int Box3DValue = ImColor(box3D_colorN[0], box3D_colorN[1], box3D_colorN[2], box3D_colorN[3]);
                            RenderObject.DrawBox3D(Box3DValue);

                        }




                        if (Options.Debug_Bone_Name || Options.Debug_Bone_count || Options.Debug_Bone_Name_Choose || Options.RainbowText)
                        {

                            //����Ĺ������ֵĲ�������
                                 //����ǵ���call�ķ���
                            if (Options.Debug_Bone_Name)
                            {
                                for (int32_t i = 0; i < static_cast<int32_t>(Actor->Mesh->SkeletalMesh->Names.Count); i++)
                                {
                                    string BoneName = GetName(Actor->Mesh->SkeletalMesh->Names.Data[i].Name);
                                    Vector3 Pos = GetBoneMatrix(Actor->Mesh, i);
                                    Vector2 Point{ 0 };
                                    if (WorldToScreen(Pos, Point))
                                    {
                                        int Debug_Bone_Name_Color = ImColor(Debug_Bone_Name_ColorN[0], Debug_Bone_Name_ColorN[1], Debug_Bone_Name_ColorN[2], Debug_Bone_Name_ColorN[3]);
                                        ImGui::GetForegroundDrawList()->AddText({ Point.X, Point.Y }, Debug_Bone_Name_Color, BoneName.c_str());
                                    }

                                }
                            }


                            if (Options.Debug_Bone_count)
                            {

                                for (int32_t i = 0; i < static_cast<int32_t>(Actor->Mesh->BoneTransForm.Count); i++)
                                {
                                    Vector3 Pos = GetBoneMatrix(Actor->Mesh, i);
                                    Vector2 Point{ 0 };
                                    if (WorldToScreen(Pos, Point))
                                    {
                                        int Debug_Bone_Count_Color = ImColor(Debug_Bone_Count_ColorN[0], Debug_Bone_Count_ColorN[1], Debug_Bone_Count_ColorN[2], Debug_Bone_Count_ColorN[3]);
                                        ImGui::GetForegroundDrawList()->AddText({ Point.X, Point.Y }, Debug_Bone_Count_Color, to_string(i).c_str());
                                    }

                                }


                            }

                            if (Options.RainbowText)
                            {
                                auto rainbow_color = rainbowColor(static_cast<float>(GetTickCount64()), 0.003f);
                                ImColor color = ImColor(std::get<0>(rainbow_color), std::get<1>(rainbow_color), std::get<2>(rainbow_color));
                                ImGui::GetBackgroundDrawList()->AddText({ 30,30 }, color, "$ranhoune");
                            }


                            if (Options.Debug_Bone_Name_Choose)
                            {
                                //��ʾɸѡ��������
                                int Count = sizeof(BoneIdx) / 4;
                                for (size_t i = 0; i < Count; i++)
                                {
                                    int* BoneIdx = (int*)&Idx;
                                    string BoneName = GetName(Actor->Mesh->SkeletalMesh->Names.Data[BoneIdx[i]].Name);

                                    Vector3 Pos = GetBoneMatrix(Actor->Mesh, BoneIdx[i]);
                                    Vector2 Point{ 0 };

                                    if (WorldToScreen(Pos, Point))
                                    {
                                        ImGui::GetForegroundDrawList()->AddText({ Point.X, Point.Y }, ImColor(255, 255, 0), BoneName.c_str());
                                    }

                                }
                            }
                        }



                    }








                }
                if (Options.OpenAimbot && Options.MemoryAimbot)
                {
                    Aimbot::GetInstance().Clear(); //�����һ������������׼������ĵ���

                    if (GetAsyncKeyState(key) & 0x8000) // �жϰ����Ƿ��£�Control��
                    {
                        Aimbot::GetInstance().MemoryAimbot();
                    }
                    else
                    {
                        Enemy_lock = nullptr;
                    }
                   
                        
                    
                }

            }
            else
            {
                Enemy_ = nullptr;
                choose_aim = 0;
            }


        }


    }
    ImGui::Render();
    g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, NULL);
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
    return  oPresent(This, SyncInterval, Flags);

}



extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

LRESULT WINAPI hkWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    
    switch (msg)
    {
    case WM_KEYUP:
        if (wParam == VK_INSERT)
            Options.Open_menu = !Options.Open_menu;
        break;
    }



    if (Options.Open_menu && ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;
    return CallWindowProcA(oWndProc, hWnd, msg, wParam, lParam);
}

HRESULT WINAPI  hkResize(IDXGISwapChain* This, UINT BufferCount, UINT Width, UINT Height, DXGI_FORMAT NewFormat, UINT SwapChainFlags)
{
    if (g_pd3dDevice != nullptr)//����,����ֹδ����ReImGui����֮ǰ�ٴ�ִ�е��µı���
    {
        g_pd3dDevice->Release();
        g_pd3dDevice = nullptr;
        g_mainRenderTargetView->Release();
        ImGui_ImplDX11_Shutdown();
        Vtb[8] = (DWORD64)Init;
    }
    return oResize(This, BufferCount, Width, Height, NewFormat, SwapChainFlags);
}

HRESULT STDMETHODCALLTYPE Init(IDXGISwapChain* This, UINT SyncInterval, UINT Flags)
{
    GetDX11ptr(This);//��ʼ��

   


    static bool Is = true;
    if (Is)
    {
        Is = false;
        

        oWndProc = (WndProc)SetWindowLongPtrA(g_hwnd, GWLP_WNDPROC, (LONG_PTR)hkWndProc);
        ImGui::CreateContext();//����imgui����

        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
        io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\msyh.ttc", 18.0f, nullptr,
        io.Fonts->GetGlyphRangesChineseFull());

        ImFontConfig cfg;
        cfg.FontBuilderFlags = ImGuiFreeTypeBuilderFlags_ForceAutoHint | ImGuiFreeTypeBuilderFlags_LightHinting | ImGuiFreeTypeBuilderFlags_LoadColor;

        font::poppins_medium = io.Fonts->AddFontFromMemoryTTF(poppins_medium, sizeof(poppins_medium), 17.f, &cfg, io.Fonts->GetGlyphRangesCyrillic());
        font::poppins_medium_low = io.Fonts->AddFontFromMemoryTTF(poppins_medium, sizeof(poppins_medium), 15.f, &cfg, io.Fonts->GetGlyphRangesCyrillic());
        font::tab_icon = io.Fonts->AddFontFromMemoryTTF(tabs_icons, sizeof(tabs_icons), 24.f, &cfg, io.Fonts->GetGlyphRangesCyrillic());
        font::tahoma_bold = io.Fonts->AddFontFromMemoryTTF(tahoma_bold, sizeof(tahoma_bold), 17.f, &cfg, io.Fonts->GetGlyphRangesCyrillic());
        font::tahoma_bold2 = io.Fonts->AddFontFromMemoryTTF(tahoma_bold, sizeof(tahoma_bold), 28.f, &cfg, io.Fonts->GetGlyphRangesCyrillic());
        font::chicons = io.Fonts->AddFontFromMemoryTTF(chicon, sizeof(chicon), 20.f, &cfg, io.Fonts->GetGlyphRangesCyrillic());


        //if (image::bg == nullptr) D3DX11CreateShaderResourceViewFromMemory(g_pd3dDevice, background_image, sizeof(background_image), &info, pump, &image::bg, 0);
        if (image::logo == nullptr) D3DX11CreateShaderResourceViewFromMemory(g_pd3dDevice, logo, sizeof(logo), &info, pump, &image::logo, 0);
        if (image::logo_general == nullptr) D3DX11CreateShaderResourceViewFromMemory(g_pd3dDevice, logo_general, sizeof(logo_general), &info, pump, &image::logo_general, 0);


        if (image::arrow == nullptr) D3DX11CreateShaderResourceViewFromMemory(g_pd3dDevice, arrow, sizeof(arrow), &info, pump, &image::arrow, 0);
        if (image::bell_notify == nullptr) D3DX11CreateShaderResourceViewFromMemory(g_pd3dDevice, bell_notify, sizeof(bell_notify), &info, pump, &image::bell_notify, 0);
        if (image::roll == nullptr) D3DX11CreateShaderResourceViewFromMemory(g_pd3dDevice, roll, sizeof(roll), &info, pump, &image::roll, 0);



        ImGui_ImplWin32_Init(g_hwnd);

        EngineInit(g_hwnd);
        HookInitialize();
    }


    ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);
    Vtb[8] = (DWORD64)hkPresent;

    return  oPresent(This, SyncInterval, Flags);
}

void GetDX11ptr(IDXGISwapChain* This)
{

    g_pSwapChain = This;
    g_pSwapChain->GetDevice(__uuidof(g_pd3dDevice), (void**)&g_pd3dDevice);
    g_pd3dDevice->GetImmediateContext(&g_pd3dDeviceContext);


    //ID3D11Texture2D* backBuffer = nullptr;
    //g_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&backBuffer);
    //g_pd3dDevice->CreateRenderTargetView(backBuffer, nullptr, &g_mainRenderTargetView);
    //backBuffer->Release();//����Ϊ��ȡ��Ҫ�ĸ�ָ��

   /* ID3D11Texture2D* pBackBuffer;
    g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
    g_pd3dDevice->CreateRenderTargetView(pBackBuffer, NULL, &g_mainRenderTargetView);
    pBackBuffer->Release();*/

    ID3D11Texture2D* renderTarget = nullptr;
    g_pSwapChain->GetBuffer(0, __uuidof(renderTarget), (void**)&renderTarget);
    g_pd3dDevice->CreateRenderTargetView(renderTarget, nullptr, &g_mainRenderTargetView);
    renderTarget->Release();//����Ϊ��ȡ��Ҫ�ĸ�ָ��


}


void DX11Hook()
{
    g_hwnd = FindWindowA("UnrealWindow", NULL);
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
    sd.OutputWindow = g_hwnd;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    UINT createDeviceFlags = 0;
    //createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
    D3D_FEATURE_LEVEL featureLevel;
    const D3D_FEATURE_LEVEL featureLevelArray[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0, };
    if (D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, nullptr, &featureLevel, nullptr) != S_OK)
        return;
    Vtb = *(DWORD64**)g_pSwapChain;
    oPresent = (Present)Vtb[8]; //ȡ����ϷPresent��ַ

    DWORD Prptect;
    VirtualProtect(Vtb, 1, PAGE_EXECUTE_READWRITE, &Prptect);//�ɶ���д��ִ��
    Vtb[8] = (DWORD64)Init;
    oResize = (Resize)Vtb[13];//ȡ����ϷResizeBuffers��ַ
    Vtb[13] = (DWORD64)hkResize;
    g_pSwapChain->Release();//�ͷŽ�����

}
