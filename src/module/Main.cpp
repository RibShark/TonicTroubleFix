#include <KnownFolders.h>
#include <ShlObj_core.h>
#include <Shlwapi.h>
#include <dinput.h>
#include <ddraw.h>
#include <d3d.h>
#include <filesystem>
#include <pathcch.h>

#include "MemoryMgr.h"
#include "Patterns.h"
#include "ScopedUnprotect.hpp"
#include "safetyhook.hpp"

struct Config
{
    std::filesystem::path ConfigPath;
    unsigned int Width{};
    unsigned int Height{};
    unsigned int BPP{};
    bool Windowed{};
    unsigned int Antialiasing{};
    unsigned int Anisotropy{};
} Config;

// GLI MODULE HOOKS

namespace GLIHooksInit {
    void OnInitializeGLI(HMODULE);
    SafetyHookInline InitializeGLILib_HOOK{};
    int __cdecl InitializeGLILib_Custom(const char* libInfo) {
        int result = InitializeGLILib_HOOK.ccall<int>(libInfo);
        if (HMODULE gliModule = GetModuleHandleA(libInfo + 0x154))
            OnInitializeGLI(gliModule);
        return result;
    }
}

namespace TextureFormatsFix {
    HRESULT (WINAPI* EnumTextureFormatsCallback_Orig)(LPDDPIXELFORMAT, LPVOID);
    HRESULT CALLBACK EnumTextureFormatsCallback_Trim(LPDDPIXELFORMAT lpDDPixFmt, LPVOID lpContext) {
        // trim non-RGB formats
        if ( lpDDPixFmt->dwFlags & DDPF_RGB )
            return EnumTextureFormatsCallback_Orig(lpDDPixFmt, lpContext);
        return DDENUMRET_OK;
    }
}

namespace ZBufferFix {
    DWORD maxDepth = 0;

    HRESULT WINAPI EnumZBufferCallback_MaxBitDepth(LPDDPIXELFORMAT lpDDPixFmt, LPVOID lpContext) {
        if (lpDDPixFmt->dwFlags & DDPF_ZBUFFER) {
            if (lpDDPixFmt->dwZBufferBitDepth > maxDepth) {
                maxDepth = lpDDPixFmt->dwZBufferBitDepth;
                memcpy(lpContext, lpDDPixFmt, sizeof(DDPIXELFORMAT));
            }
        }
        return D3DENUMRET_OK;
    }
}

namespace BlitFix {
    LPDIRECTDRAW4 *lplpDD;
    LPDIRECTDRAWSURFACE4 *lplpBackBufferSurface;

    SafetyHookInline GLI_DRV_vWrite16bBitmapToBackBuffer_HOOK{};
    void __cdecl GLI_DRV_vWrite16bBitmapToBackBuffer(int _unk1, int _unk2, void* src, DWORD width, DWORD height) {
        LPDIRECTDRAWSURFACE4 lpSourceSurface;
        DDSURFACEDESC2 ddscDst;
        DDSURFACEDESC2 ddscSrc;
        memset(&ddscDst, 0, sizeof(ddscDst));
        memset(&ddscSrc, 0, sizeof(ddscSrc));

        LPDIRECTDRAW4 lpDD = *lplpDD;
        LPDIRECTDRAWSURFACE4 lpBackBufferSurface = *lplpBackBufferSurface;

        // return early if null pointer
        if (!lpBackBufferSurface || !lpDD) return;

        // lock backbuffer to get ddsc
        ddscDst.dwSize = sizeof(ddscDst);
        if (lpBackBufferSurface->Lock(nullptr, &ddscDst, DDLOCK_WAIT | DDLOCK_NOSYSLOCK, nullptr) != DD_OK
            || lpBackBufferSurface->Unlock(nullptr) != DD_OK) return;

        // set source ddsc
        ddscSrc.dwSize = sizeof(ddscSrc);
        ddscSrc.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT | DDSD_PIXELFORMAT;
        ddscSrc.ddsCaps.dwCaps = DDSCAPS_SYSTEMMEMORY;
        ddscSrc.dwWidth = width;
        ddscSrc.dwHeight = height;
        memcpy(&ddscSrc.ddpfPixelFormat, &ddscDst.ddpfPixelFormat, sizeof(DDPIXELFORMAT));

        if (lpDD->CreateSurface(&ddscSrc, &lpSourceSurface, nullptr) != DD_OK) return;
        if (lpSourceSurface->Lock(nullptr, &ddscSrc, DDLOCK_WAIT, nullptr) == DD_OK)
        {
            if (Config.BPP == 32)
            {
                auto srcBitmap = static_cast<uint16_t *>(src);
                auto dstBitmap = static_cast<uint32_t *>(ddscSrc.lpSurface);
                unsigned int r, g, b;
                for (auto i = 0; i < width * height; i++) {
                    r = (srcBitmap[i] & 0x7C00) >> 10;
                    g = (srcBitmap[i] & 0x3E0) >> 5;
                    b = (srcBitmap[i] & 0x1F);

                    r <<= 3;
                    g <<= 3;
                    b <<= 3;

                    dstBitmap[i] = (r << 16) | (g << 8) | b;
                }
            }
            else
            {
                auto srcBitmap = static_cast<uint16_t *>(src);
                auto dstBitmap = static_cast<uint16_t *>(ddscSrc.lpSurface);
                for (auto i = 0; i < width * height; i++) {
                    dstBitmap[i] = srcBitmap[i];
                }
            }
            lpSourceSurface->Unlock(nullptr);

            float aspectBmp = static_cast<float>(width) / static_cast<float>(height);
            float aspectGame = static_cast<float>(ddscDst.dwWidth) / static_cast<float>(ddscDst.dwHeight);
            RECT rect{};
            if (aspectBmp > aspectGame)
            {
                auto newHeight = static_cast<int>(static_cast<float>(ddscDst.dwWidth) / aspectBmp);
                rect.left = 0;
                rect.right = static_cast<LONG>(ddscDst.dwWidth);
                rect.top = static_cast<LONG>(ddscDst.dwHeight - newHeight) / 2;
                rect.bottom = static_cast<LONG>(ddscDst.dwHeight - rect.top);
            }
            else
            {
                auto newWidth = static_cast<int>(static_cast<float>(ddscDst.dwHeight) * aspectBmp);
                rect.left = static_cast<LONG>(ddscDst.dwWidth - newWidth) / 2;
                rect.right = static_cast<LONG>(ddscDst.dwWidth - rect.left);
                rect.top = 0;
                rect.bottom = static_cast<LONG>(ddscDst.dwHeight);
            }

            DDBLTFX fx{};
            fx.dwSize = sizeof(DDBLTFX);
            fx.dwFillColor = 0; // black

            lpBackBufferSurface->Blt(nullptr, nullptr, nullptr, DDBLT_WAIT | DDBLT_COLORFILL, &fx);
            lpBackBufferSurface->Blt(&rect, lpSourceSurface, nullptr, DDBLT_WAIT, nullptr);
        }

        lpSourceSurface->Release();
    }

    SafetyHookInline GLI_DRV_vWrite24bBitmapToBackBuffer_HOOK{};
    void __cdecl GLI_DRV_vWrite24bBitmapToBackBuffer(int _unk1, int _unk2, void* src, DWORD width, DWORD height) {
        LPDIRECTDRAWSURFACE4 lpSourceSurface;
        DDSURFACEDESC2 ddscDst;
        DDSURFACEDESC2 ddscSrc;
        memset(&ddscDst, 0, sizeof(ddscDst));
        memset(&ddscSrc, 0, sizeof(ddscSrc));

        LPDIRECTDRAW4 lpDD = *lplpDD;
        LPDIRECTDRAWSURFACE4 lpBackBufferSurface = *lplpBackBufferSurface;

        // return early if null pointer
        if (!lpBackBufferSurface || !lpDD) return;

        // lock backbuffer to get ddsc
        ddscDst.dwSize = sizeof(ddscDst);
        if (lpBackBufferSurface->Lock(nullptr, &ddscDst, DDLOCK_WAIT | DDLOCK_NOSYSLOCK, nullptr) != DD_OK
            || lpBackBufferSurface->Unlock(nullptr) != DD_OK) return;

        // set source ddsc
        ddscSrc.dwSize = sizeof(ddscSrc);
        ddscSrc.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT | DDSD_PIXELFORMAT;
        ddscSrc.ddsCaps.dwCaps = DDSCAPS_SYSTEMMEMORY;
        ddscSrc.dwWidth = width;
        ddscSrc.dwHeight = height;
        memcpy(&ddscSrc.ddpfPixelFormat, &ddscDst.ddpfPixelFormat, sizeof(DDPIXELFORMAT));

        if (lpDD->CreateSurface(&ddscSrc, &lpSourceSurface, nullptr) != DD_OK) return;
        if (lpSourceSurface->Lock(nullptr, &ddscSrc, DDLOCK_WAIT, nullptr) == DD_OK)
        {
            if (Config.BPP == 32)
            {
                auto srcBitmap = static_cast<uint8_t *>(src);
                auto dstBitmap = static_cast<uint32_t *>(ddscSrc.lpSurface);
                unsigned int r, g, b;
                for (auto i = 0; i < width * height; i++) {
                    b = srcBitmap[3*i];
                    g = srcBitmap[3*i+1];
                    r = srcBitmap[3*i+2];

                    dstBitmap[i] = (r << 16) | (g << 8) | b;
                }
            }
            else
            {
                auto srcBitmap = static_cast<uint8_t *>(src);
                auto dstBitmap = static_cast<uint16_t *>(ddscSrc.lpSurface);
                unsigned int r, g, b;
                for (auto i = 0; i < width * height; i++)
                {
                    b = srcBitmap[3*i] >> 3;
                    g = srcBitmap[3*i+1] >> 2;
                    r = srcBitmap[3*i+2] >> 3;

                    dstBitmap[i] = (r << 11) | (g << 5) | b;
                }
            }
            lpSourceSurface->Unlock(nullptr);

            float aspectBmp = static_cast<float>(width) / static_cast<float>(height);
            float aspectGame = static_cast<float>(ddscDst.dwWidth) / static_cast<float>(ddscDst.dwHeight);
            RECT rect{};
            if (aspectBmp > aspectGame)
            {
                auto newHeight = static_cast<int>(static_cast<float>(ddscDst.dwWidth) / aspectBmp);
                rect.left = 0;
                rect.right = static_cast<LONG>(ddscDst.dwWidth);
                rect.top = static_cast<LONG>(ddscDst.dwHeight - newHeight) / 2;
                rect.bottom = static_cast<LONG>(ddscDst.dwHeight - rect.top);
            }
            else
            {
                auto newWidth = static_cast<int>(static_cast<float>(ddscDst.dwHeight) * aspectBmp);
                rect.left = static_cast<LONG>(ddscDst.dwWidth - newWidth) / 2;
                rect.right = static_cast<LONG>(ddscDst.dwWidth - rect.left);
                rect.top = 0;
                rect.bottom = static_cast<LONG>(ddscDst.dwHeight);
            }

            DDBLTFX fx{};
            fx.dwSize = sizeof(DDBLTFX);
            fx.dwFillColor = 0; // black
            fx.dwDDFX = DDBLTFX_MIRRORUPDOWN;

            lpBackBufferSurface->Blt(nullptr, nullptr, nullptr, DDBLT_WAIT | DDBLT_COLORFILL, nullptr); // clear background
            lpBackBufferSurface->Blt(&rect, lpSourceSurface, nullptr, DDBLT_WAIT | DDBLT_DDFX, &fx);
        }

        lpSourceSurface->Release();
    }
}

void GLIHooksInit::OnInitializeGLI(HMODULE GliModule) {
    using namespace Memory;
    using namespace hook::txn;

    std::unique_ptr<ScopedUnprotect::Unprotect> Protect = ScopedUnprotect::UnprotectSectionOrFullModule(
            GliModule, "");

    // fix texture formats overflowing
    try {
        using namespace TextureFormatsFix;
        auto ptn = make_module_pattern(GliModule, "68 ? ? ? ? 51 FF 50"); // 0x1000F4FA
        EnumTextureFormatsCallback_Orig = *ptn.get_first<decltype(EnumTextureFormatsCallback_Orig)>(1);
        Patch(ptn.get_first(1), &EnumTextureFormatsCallback_Trim);
    }
    TXN_CATCH()

    // always use max Z-Buffer bit depth
    try {
        using namespace ZBufferFix;
        auto ptn = make_module_pattern(GliModule, "68 ? ? ? ? ? ? 68 ? ? ? ? 50 FF 52"); // 0x10012B3D
        Patch(ptn.get_first(1), &EnumZBufferCallback_MaxBitDepth);
    }
    TXN_CATCH()

    // fix blitting in 32-bit mode
    try {
        using namespace BlitFix;

        lplpDD = *static_cast<LPDIRECTDRAW4**>(make_module_pattern(GliModule, "A1 ? ? ? ? 51 8D 4C 24 ? ? ? 51").get_first(1));
        lplpBackBufferSurface = *static_cast<LPDIRECTDRAWSURFACE4**>(make_module_pattern(GliModule, "8B 15 ? ? ? ? 53 56 57").get_first(2));

        void *pGLI_DRV_vWrite16bBitmapToBackBuffer = make_module_pattern(GliModule, "81 EC ? ? ? ? 8B 15 ? ? ? ? 53 56").get_first();// 0x1000D4E0
        GLI_DRV_vWrite16bBitmapToBackBuffer_HOOK = safetyhook::create_inline(pGLI_DRV_vWrite16bBitmapToBackBuffer, reinterpret_cast<void *>(GLI_DRV_vWrite16bBitmapToBackBuffer));

        void *pGLI_DRV_vWrite24bBitmapToBackBuffer = make_module_pattern(GliModule, "81 EC ? ? ? ? 8B 15 ? ? ? ? 53 55").get_first();// 0x1000D6B0
        GLI_DRV_vWrite24bBitmapToBackBuffer_HOOK = safetyhook::create_inline(pGLI_DRV_vWrite24bBitmapToBackBuffer, reinterpret_cast<void *>(GLI_DRV_vWrite24bBitmapToBackBuffer));
    }
    TXN_CATCH()

    // do not maximize window
    try
    {
        auto ptn = make_module_pattern(GliModule, "6A ? 50 A3"); // 0x10010733
        Patch<uint8_t>(ptn.get_first(1), SW_NORMAL);
    }
    TXN_CATCH()
}

// MAIN MODULE HOOKS

namespace ExceptionFix {
    SafetyHookInline sub_45D0A0_HOOK{};
    BOOL __cdecl sub_45D0A0_wrapException(int a1) {
        __try {
            return sub_45D0A0_HOOK.ccall<BOOL>(a1);
        }
        __except ( EXCEPTION_EXECUTE_HANDLER ) {
            return 0;
        }
    }
}

namespace RegisterClassFix {
    SafetyHookInline RegisterClassA_HOOK{};
    ATOM __stdcall RegisterClassA_fixHINSTANCE(WNDCLASSA* lpWndClass) {
        lpWndClass->hInstance = GetModuleHandleA(nullptr);
        return RegisterClassA_HOOK.stdcall<ATOM>(lpWndClass);
    }
}

namespace SaveGameRedirect {
    static char saveGameFolder[MAX_PATH];
    static char optionsFolder[MAX_PATH];

    SafetyHookInline getSaveGameFolder_HOOK{};
    const char* __cdecl getSaveGameFolder() {
        return saveGameFolder;
    }

    SafetyHookInline getOptionsFolder_HOOK{};
    const char* __cdecl getOptionsFolder() {
        return optionsFolder;
    }

    SafetyHookInline combinePath_HOOK{};
    byte __cdecl combinePath_FixNonRelative(const char* path1, const char* path2) {
        // the game attempts to concatenate ".\" onto many paths; this is problematic for non-relative paths, so don't concatenate in that case
        if ( strchr(path2, ':') ) {
            std::filesystem::path p(path2);
            std::string path = p.parent_path().string();
            std::string fileName = p.filename().string();
            return combinePath_HOOK.ccall<byte>(path.c_str(), fileName.c_str());
        }
        return combinePath_HOOK.ccall<byte>(path1, path2);
    }
}

namespace ForceResolution {
    SafetyHookInline getWindowWidth_HOOK{};
    int __cdecl getWindowWidth_force() {
        return Config.Width;
    }

    SafetyHookInline getWindowHeight_HOOK{};
    int __cdecl getWindowHeight_force() {
        return Config.Height;
    }

    SafetyHookInline getBPP_HOOK{};
    int __cdecl getBPP_force() {
        return Config.BPP;
    }

    SafetyHookInline setResolutionFromOrdinal_HOOK{};
    void __cdecl setResolutionFromOrdinal_force(int ordinal, int* pWidth, int* pHeight) {
        *pWidth = Config.Width;
        *pHeight = Config.Height;
    }

    SafetyHookInline unkCameraFunc_HOOK{};
    int __cdecl unkCameraFunc_force(int arg1, short arg2, float* camera) {
        *(camera + 0x19) = 2.0f * atan(tan(0.75f) * 0.75f * (static_cast<float>(Config.Width) / static_cast<float>(Config.Height)));
        return unkCameraFunc_HOOK.ccall<int>(arg1, arg2, camera);
    }
}

namespace WindowedMode
{
    SafetyHookInline CreateWindowExA_HOOK{};
    HWND __stdcall CreateWindowExA_force(DWORD dwExStyle, LPCSTR lpClassName, LPCSTR lpWindowName, DWORD dwStyle, int X, int Y, int nWidth, int nHeight, HWND hWndParent, HMENU hMenu, HINSTANCE hInstance, LPVOID lpParam)
    {
        dwStyle |= WS_SYSMENU | WS_MINIMIZEBOX | DS_CENTER;
        nWidth = Config.Width;
        nHeight = Config.Height;

        // center window
        const auto hwnd = CreateWindowExA_HOOK.stdcall<HWND>(dwExStyle, lpClassName, lpWindowName, dwStyle, X, Y, nWidth, nHeight, hWndParent, hMenu, hInstance, lpParam);
        RECT rc;
        GetClientRect(hwnd, &rc);

        const int xPos = (GetSystemMetrics(SM_CXSCREEN) - rc.right) / 2;
        const int yPos = (GetSystemMetrics(SM_CYSCREEN) - rc.bottom) / 2;

        SetWindowPos(hwnd, HWND_TOP, xPos, yPos, 0, 0, SWP_NOZORDER | SWP_NOSIZE);

        return hwnd;
    }

    LPDIRECTINPUTDEVICE lpDeviceMouse;
    SafetyHookMid MouseInit_HOOK{};
    void MouseInit_getDIInterface(SafetyHookContext& ctx)
    {
        lpDeviceMouse = reinterpret_cast<LPDIRECTINPUTDEVICE>(ctx.eax);;
    }

    WNDPROC WndProc_Orig;
    RECT* pRect;
    LRESULT WndProc_Custom(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
    {
        switch (msg)
        {
        case WM_CREATE:
            SetCursor(nullptr);
            return 0;
        case WM_SETCURSOR:
            if (LOWORD(lParam) == HTCLIENT)
            {
                return true;
            }
            SetCursor(LoadCursor(nullptr, IDC_ARROW));
            return DefWindowProc(hWnd, msg, wParam, lParam);
        case WM_LBUTTONUP:
            SetCursor(nullptr);
            ClipCursor(pRect);
            if (lpDeviceMouse != nullptr)
            {
                lpDeviceMouse->Acquire();
            }
            break;
        case WM_MOVE:
            GetClientRect(hWnd, pRect);
            MapWindowPoints(hWnd, nullptr, reinterpret_cast<POINT*>(pRect), 2);
            return 0;
        case WM_ACTIVATEAPP:
            if (wParam == false)
            {
                if (lpDeviceMouse != nullptr)
                {
                    lpDeviceMouse->Unacquire();
                }
            }
            return DefWindowProc(hWnd, msg, wParam, lParam);
        case WM_SYSKEYUP:
            // the game tries and fails to switch fullscreen mode on alt-enter
            if (wParam == VK_RETURN) return 0;
            break;
        case WM_SYSCOMMAND:
            if (wParam == SC_CLOSE || wParam == SC_MINIMIZE)
                return DefWindowProc(hWnd, msg, wParam, lParam);
            break;
        default:
            if (msg >= WM_NCMOUSEMOVE && msg <= WM_NCMBUTTONDBLCLK) // the game disables handling of these for some reason
                return DefWindowProc(hWnd, msg, wParam, lParam);
            return WndProc_Orig(hWnd, msg, wParam, lParam);
        }
        return WndProc_Orig(hWnd, msg, wParam, lParam);
    }
}

namespace Indeo
{
    SafetyHookInline RegQueryValueExW_HOOK{};
    LSTATUS __stdcall RegQueryValueExW_ForceIndeo(HKEY hKey, LPCWSTR lpValueName, LPDWORD reserved, LPDWORD lpType, LPBYTE lpData, LPDWORD lpcbData)
    {
        if (!lstrcmpiW(lpValueName, L"vidc.iv41"))
        {
            LSTATUS result = ERROR_SUCCESS;

            const auto value = L"ir41_32.ax";
            *lpType = REG_SZ;
            if (sizeof(value) < *lpcbData)
                result = ERROR_MORE_DATA;
            *lpcbData = sizeof(value);
            if (lpData != nullptr)
                lstrcpyW(reinterpret_cast<LPWSTR>(lpData), value);

            return result;
        }

        return RegQueryValueExW_HOOK.stdcall<LSTATUS>(hKey, lpValueName, reserved, lpType, lpData, lpcbData);
    }
}

namespace IniFile
{
    SafetyHookInline GetPrivateProfileStringA_HOOK{};
    DWORD WINAPI GetPrivateProfileStringA_redirect(LPCSTR lpAppName, LPCSTR lpKeyName, LPCSTR lpDefault, LPCSTR lpReturnedString, DWORD nSize, LPCSTR lpFileName)
    {
        if (lpAppName != nullptr && strcmp(lpAppName, "TONICT") != 0)
            return GetPrivateProfileStringA_HOOK.stdcall<DWORD>(lpAppName, lpKeyName, lpDefault, lpReturnedString, nSize, lpFileName);

        std::filesystem::path IniPath = Config.ConfigPath;
        IniPath += "\\ubi.ini";
        return GetPrivateProfileStringA_HOOK.stdcall<DWORD>(lpAppName, lpKeyName, lpDefault, lpReturnedString, nSize, IniPath.string().c_str());
    }
}

namespace Menu
{
    void (__cdecl *MNU_fn_vChangeDisplayableMenuItem)(void** ppItem, char bIsVisible);
    void** (__cdecl *MNU_fn_xGetActiveMenuItem)();
    void* (__cdecl *MNU_fn_xGetActiveMenu)();
    void (__cdecl *MNU_fn_vSelectNextItem)(void*);

    SafetyHookInline MNU_cbInitResolution_HOOK{};
    SafetyHookInline MNU_cbInitSynchroLoop_HOOK{};

    void __cdecl MNU_cbHideItem(void** ppItem)
    {
        MNU_fn_vChangeDisplayableMenuItem(ppItem, false);
        if (MNU_fn_xGetActiveMenuItem() == ppItem)
        {
            void* menu = MNU_fn_xGetActiveMenu();
            MNU_fn_vSelectNextItem(menu);
        }
    };

}

void OnInitializeHook() {
    using namespace Memory;
    using namespace hook::txn;

    std::unique_ptr<ScopedUnprotect::Unprotect> Protect = ScopedUnprotect::UnprotectSectionOrFullModule(
            GetModuleHandle( nullptr ), "");

    // Load config
    {
        const auto appName = L"TONICT";

        PWSTR savedGamesPath{};
        if (SHGetKnownFolderPath(FOLDERID_SavedGames, KF_FLAG_CREATE, nullptr, &savedGamesPath) == S_OK)
        {
            Config.ConfigPath = savedGamesPath;
            Config.ConfigPath += R"(\Tonic Trouble)";
            CoTaskMemFree(savedGamesPath);
        }
        std::filesystem::path IniPath = Config.ConfigPath;
        IniPath += R"(\ubi.ini)";

        Config.Width = GetPrivateProfileIntW(appName, L"Width", 640, IniPath.c_str());
        Config.Height = GetPrivateProfileIntW(appName, L"Height", 480, IniPath.c_str());
        Config.BPP = GetPrivateProfileIntW(appName, L"BPP", 32, IniPath.c_str());
        Config.Windowed = GetPrivateProfileIntW(appName, L"Windowed", 0, IniPath.c_str());
        Config.Antialiasing = GetPrivateProfileIntW(appName, L"Antialiasing", 0, IniPath.c_str());
        Config.Anisotropy = GetPrivateProfileIntW(appName, L"Anisotropy", 0, IniPath.c_str());
    }

    // Hook GetPrivateProfileStringA
    {
        using namespace IniFile;
        GetPrivateProfileStringA_HOOK = safetyhook::create_inline(GetPrivateProfileStringA, GetPrivateProfileStringA_redirect);
    }

    // Copy current.cfg from GAMEDATA to save directory if it doesn't exist
    {
        std::filesystem::path CfgPathSrc = std::filesystem::current_path();
        CfgPathSrc += R"(\GAMEDATA\Options\Current.cfg)";

        std::filesystem::path CfgPathDst = Config.ConfigPath;
        CfgPathDst += R"(\Options)";
        std::filesystem::create_directories(CfgPathDst);
        CfgPathDst += R"(\Current.cfg)";

        std::filesystem::copy_file(CfgPathSrc, CfgPathDst, std::filesystem::copy_options::skip_existing);
    }

    // Show error and quit if ubi.ini is not in save directory
    {
        std::filesystem::path IniPath = Config.ConfigPath;
        IniPath += R"(\ubi.ini)";
        if (!std::filesystem::exists(IniPath))
        {
            MessageBox(nullptr, "Tonic Trouble's configuration file could not be found. Please run the "
                "configuration program before running Tonic Trouble.", "Error", MB_OK);
            ExitProcess(0);
        }

    }

    // Force resolution
    try {
        using namespace ForceResolution;
        void* pGetWindowWidth = get_pattern("E8 ? ? ? ? 85 C0 74 10 A1 ? ? ? ? 8D 04 40 8B 04 85 ? ? ? ? C3 B8 80"); // 0x442620
        getWindowWidth_HOOK = safetyhook::create_inline(pGetWindowWidth, reinterpret_cast<void *>(getWindowWidth_force));

        void* pGetWindowHeight = get_pattern("E8 ? ? ? ? 85 C0 74 10 A1 ? ? ? ? 8D 04 40 8B 04 85 ? ? ? ? C3 B8 E0"); // 0x442640
        getWindowHeight_HOOK = safetyhook::create_inline(pGetWindowHeight, reinterpret_cast<void *>(getWindowHeight_force));

        void* pGetBPP = get_pattern("A1 ? ? ? ? ? ? ? 8B 04 85 ? ? ? ? C3 8B 0D"); // 0x442660
        getBPP_HOOK = safetyhook::create_inline(pGetBPP, reinterpret_cast<void *>(getBPP_force));

        void* pSetResolutionFromOrdinal = get_pattern("8B 44 24 04 48"); // 0x411DF0
        setResolutionFromOrdinal_HOOK = safetyhook::create_inline(pSetResolutionFromOrdinal, reinterpret_cast<void *>(setResolutionFromOrdinal_force));

        void* pUnkCameraFunc = get_pattern("55 8B EC 81 EC D8 00 00 00 53 56 8B"); // 0x442210
        unkCameraFunc_HOOK = safetyhook::create_inline(pUnkCameraFunc, reinterpret_cast<void *>(unkCameraFunc_force));
    }
    TXN_CATCH()

    // Fix bad exception handling on protection routine
    try {
        using namespace ExceptionFix;
        void* protectionRoutine = get_pattern("8B 54 24 04 33 C0 66"); // 0x45D0A0
        sub_45D0A0_HOOK = safetyhook::create_inline(protectionRoutine, reinterpret_cast<void*>(&sub_45D0A0_wrapException));
    }
    TXN_CATCH()

    // Fix RegisterClass using invalid HINSTANCE
    {
        using namespace RegisterClassFix;
        RegisterClassA_HOOK = safetyhook::create_inline(reinterpret_cast<void*>(&RegisterClassA), reinterpret_cast<void*>(&RegisterClassA_fixHINSTANCE));
    }

    // Use only the first fakefile
    try {
        auto ptn = pattern("42 3D CB 34 00 00"); // 0x4594D3
        Nop(ptn.get_first(), 1); // inc eax -> nop (this usually increments the number in the filename)

        Patch<uint8_t>(ptn.get_first(70), FILE_SHARE_READ); // allow the file to be opened more than once

        void* setRandomFileIndex = get_pattern("E8 ? ? ? ? 99 33 C2"); // 0x4596B8
        Patch(setRandomFileIndex, { 0xB8, 0x00, 0x00, 0x00, 0x00 } ); // mov eax, 0 (usually, eax is a semi-random number between 0 and 3)
    }
    TXN_CATCH()

    // Remove CD check
    try {
        static const char* cdrom = "-cdrom:.";
        void* cmdLine = get_pattern("8B 44 24 04 8B 4C 24 08 A3", -15); // 0x401360
        Patch(cmdLine, cdrom);
    }
    TXN_CATCH()

    // Fix windowed mode
    try
    {
        using namespace WindowedMode;

        // Change windows creation params
        CreateWindowExA_HOOK = safetyhook::create_inline(CreateWindowExA, CreateWindowExA_force);

        // get pointer to cursor clip rect
        auto ptn = pattern("A1 ? ? ? ? 83 EC ? 85 C0 0F 85"); // 0x4016E0
        pRect = *static_cast<RECT**>(ptn.get_first(0x18));

        // get DirectInput mouse interface
        ptn = pattern("? ? 8D 54 24 ? 52 6A ? 50 FF 51 ? 3D ? ? ? ? 74 ? 3D ? ? ? ? 74 ? 85 C0 75 ? 8B 44 24 ? 8B 4C 24 ? 03 C1"); // 0x478B8D, god why is this so long
        MouseInit_HOOK = safetyhook::create_mid(ptn.get_first(), MouseInit_getDIInterface);

        // don't reacquire mouse automatically
        ptn = pattern("74 ? A8 ? 74 ? 0F BF C6"); // 0x478E62
        Nop(ptn.get_first(), 2);
        Patch<uint8_t>(ptn.get_first(4), 0xEB); // jz -> jmp

        // use custom WndProc
        ptn = pattern("C7 44 24 ? ? ? ? ? C7 44 24 ? ? ? ? ? C7 44 24 ? ? ? ? ? 89 7C 24"); // 0x401CEF
        WndProc_Orig = *ptn.get_first<decltype(WndProc_Orig)>(4);
        Patch(ptn.get_first(4), &WndProc_Custom);
    }
    TXN_CATCH()

    // fix Indeo
    {
        using namespace Indeo;

        HMODULE module = GetModuleHandle("kernelbase.dll");
        FARPROC RegQueryValueExW = GetProcAddress(module, "RegQueryValueExW");

        if (!module || !RegQueryValueExW)
        {
            module = GetModuleHandle("advapi32.dll");
            RegQueryValueExW = GetProcAddress(module, "RegQueryValueExW");
        }
        RegQueryValueExW_HOOK = safetyhook::create_inline(RegQueryValueExW, RegQueryValueExW_ForceIndeo);
    }

    // Fix dialogue background
    try
    {
        auto ptn = pattern("68 ? ? ? ? 51 FF 15 ? ? ? ? 83 C4 ? 5E"); // 0x446659
        Patch(ptn.get_first(1), 0.99999f); // D3D documentation: The largest allowable value for dvSZ is 0.99999, if you want the vertex to be within the range of z-values that are displayed.
    }
    TXN_CATCH()

    // redirect videos folder to HD
    try {
        void* setVideoFolderCall = get_pattern("E8 ? ? ? ? 83 C4 04 5B 5F 5E 81 C4 04 01 00 00"); // 0x410979
        uint32_t (*setVideoFolder)(const char*);
        ReadCall(setVideoFolderCall, setVideoFolder);
        setVideoFolder("videos");
        Nop(setVideoFolderCall, 5); // nop so it won't be called again
    }
    TXN_CATCH()

    // redirect save files
    try {
        using namespace SaveGameRedirect;
        wchar_t* savedGamesFolder;
        char saveFolder[MAX_PATH];

        if ( !FAILED(SHGetKnownFolderPath(FOLDERID_SavedGames, KF_FLAG_CREATE, nullptr, &savedGamesFolder)) &&
            !FAILED(WideCharToMultiByte(CP_ACP, 0, savedGamesFolder, -1, saveFolder, MAX_PATH, nullptr, nullptr)) &&
            PathAppendA(saveFolder, "\\Tonic Trouble")) {
                // redirect SaveGame folder
                strcpy_s(saveGameFolder, saveFolder);
                PathAppendA(saveGameFolder, "\\SaveGame");
                void* getSaveGameFolderCall = get_pattern("E8 ? ? ? ? 50 8D 94 24 7C 01 00 00"); // 0x42185F
                void* pGetSaveGameFolder;
                ReadCall(getSaveGameFolderCall, pGetSaveGameFolder);
                getSaveGameFolder_HOOK = safetyhook::create_inline(pGetSaveGameFolder, reinterpret_cast<void *>(getSaveGameFolder));

                // redirect Options folder
                strcpy_s(optionsFolder, saveFolder);
                PathAppendA(optionsFolder, "\\Options");
                void* getOptionsFolderCall = get_pattern("E8 ? ? ? ? 50 8D 8C 24 18 01 00 00"); // 0x410FEA
                void* pGetOptionsFolder;
                ReadCall(getOptionsFolderCall, pGetOptionsFolder);
                getOptionsFolder_HOOK = safetyhook::create_inline(pGetOptionsFolder, reinterpret_cast<void *>(getOptionsFolder));

                // fix path combining logic
                void* pCombinePath = get_pattern("81 EC 0C 02 00 00 53 8B"); // 0x472240
                combinePath_HOOK = safetyhook::create_inline(pCombinePath, reinterpret_cast<void *>(combinePath_FixNonRelative));
        }
        CoTaskMemFree(savedGamesFolder);
    }
    TXN_CATCH()

    // hide unnecessary/troublesome menu items
    try
    {
        using namespace Menu;
        MNU_fn_vChangeDisplayableMenuItem = static_cast<void(*)(void**, char)>(get_pattern("8A 44 24 ? B1 ? 32 D2")); // 0x44DD40
        MNU_fn_xGetActiveMenuItem = static_cast<void**(*)()>(get_pattern("8B 15 ? ? ? ? 33 C0 8A 4A")); // 0x44F250
        MNU_fn_xGetActiveMenu = static_cast<void*(*)()>(get_pattern("8B 15 ? ? ? ? 33 C0 8A 4A", -0x10)); // 0x44F240
        MNU_fn_vSelectNextItem = static_cast<void(*)(void*)>(get_pattern("56 8B 74 24 ? 8A 46 ? 3C ? 74 ? 57")); // 0x44EE80;

        void* MNU_cbInitResolution = get_pattern("56 8B 74 24 ? 6A ? ? ? 50 E8"); // 0x4115B0
        void* MNU_cbInitSynchroLoop = get_pattern("A1 ? ? ? ? 8B 4C 24 ? 25", 0x50); // 0x411F20
        MNU_cbInitResolution_HOOK = safetyhook::create_inline(MNU_cbInitResolution, MNU_cbHideItem);
        MNU_cbInitSynchroLoop_HOOK = safetyhook::create_inline(MNU_cbInitSynchroLoop, MNU_cbHideItem);

    }
    TXN_CATCH()

    // initialize any GLI library hooks/patches
    try {
        using namespace GLIHooksInit;
        void* initializeGLILib = get_pattern("8B 44 24 04 85 C0 56"); // 0x43E910
        InitializeGLILib_HOOK = safetyhook::create_inline(initializeGLILib, reinterpret_cast<void*>(InitializeGLILib_Custom));
    }
    TXN_CATCH()

    // set dxwrapper params
    {
        char value[4]; // not going to be any more than 3 characters

        sprintf_s(value, "%d", Config.Antialiasing);
        SetEnvironmentVariable("DXWRAPPER_ANTIALIASING", value);
        sprintf_s(value, "%d", Config.Anisotropy);
        SetEnvironmentVariable("DXWRAPPER_ANISOTROPICFILTERING", value);
        sprintf_s(value, "%d", Config.Windowed);
        SetEnvironmentVariable("DXWRAPPER_ENABLEWINDOWMODE", value);
        SetEnvironmentVariable("DXWRAPPER_FULLSCREENWINDOWMODE", strcmp(value, "0") ? "0" : "1");

        SetEnvironmentVariable("DXWRAPPER_WINDOWMODEBORDER", "1");
        SetEnvironmentVariable("DXWRAPPER_DD7TO9", "1");
        SetEnvironmentVariable("DXWRAPPER_D3D9TO9EX", "1");
        SetEnvironmentVariable("DXWRAPPER_DDRAWFIXBYTEALIGNMENT", "1");
        SetEnvironmentVariable("DXWRAPPER_LIMITPERFRAMEFPS", "60");
        SetEnvironmentVariable("DXWRAPPER_DINPUTTO8", "0");
        SetEnvironmentVariable("DXWRAPPER_DINPUT8HOOKSYSTEM32", "0");
    }

    // load dxwrapper
    LoadLibraryA("dxwrapper.dll");
}
