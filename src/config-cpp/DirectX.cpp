#include <ddraw.h>
#include <d3d9.h>

#include "DirectX.h"

DirectX::DirectX()
{
    lpD3D = Direct3DCreate9(D3D_SDK_VERSION);

    DirectDrawEnumerateExA(&DDEnumCallback, this, 0);
}

std::vector<D3DMULTISAMPLE_TYPE> DirectX::GetAntiAliasingModes(int deviceOrdinal, bool windowed)
{
    std::vector<D3DMULTISAMPLE_TYPE> modes;

    for (int iType = D3DMULTISAMPLE_2_SAMPLES; iType <= D3DMULTISAMPLE_16_SAMPLES; iType++) {
        auto type = static_cast<D3DMULTISAMPLE_TYPE>(iType);
        if (SUCCEEDED(lpD3D->CheckDeviceMultiSampleType(deviceOrdinal, D3DDEVTYPE_HAL, D3DFMT_X8R8G8B8, windowed, type, nullptr))) {
            modes.push_back(type);
        }
    }
    return modes;
}

DWORD DirectX::GetMaxAnisotropy(int deviceOrdinal)
{
    D3DCAPS9 caps;

    if (SUCCEEDED(lpD3D->GetDeviceCaps(deviceOrdinal, D3DDEVTYPE_HAL, &caps)))
    {
        return caps.MaxAnisotropy;
    }
    return 0;
}

std::vector<DisplayMode> DirectX::GetDisplayModes(int deviceOrdinal)
{
    std::vector<DisplayMode> modes;

    LPDIRECTDRAW4 lpDD4 = devices[deviceOrdinal].lpDD4;
    lpDD4->EnumDisplayModes(0, nullptr, &modes, EnumModesCallback);
    std::sort(modes.begin(), modes.end());
    return modes;
}

std::vector<CString> DirectX::GetDeviceNames()
{
    std::vector<CString> deviceNames;

    for (auto device : devices) {
        CString name = device.description; // + " (" + device.name + ")";
        deviceNames.push_back(name);
    }
    return deviceNames;
}

HRESULT WINAPI DirectX::EnumModesCallback(LPDDSURFACEDESC2 lpDDSurfaceDesc, LPVOID lpContext)
{
    const auto modes = static_cast<std::vector<DisplayMode> *>(lpContext);
    if (lpDDSurfaceDesc->ddpfPixelFormat.dwRGBBitCount != 8)
        modes->push_back({lpDDSurfaceDesc->dwWidth, lpDDSurfaceDesc->dwHeight, lpDDSurfaceDesc->ddpfPixelFormat.dwRGBBitCount});
    return DDENUMRET_OK;
}

BOOL WINAPI DirectX::DDEnumCallback(GUID *lpGUID, LPSTR lpDriverDescription, LPSTR lpDriverName, LPVOID lpContext, HMONITOR hm)
{
    const auto _this = static_cast<DirectX *>(lpContext);

    LPDIRECTDRAW lpDD;
    LPDIRECTDRAW4 lpDD4;
    if (DirectDrawCreate(lpGUID, &lpDD, nullptr) != DD_OK) return TRUE;
    if (lpDD->QueryInterface(IID_IDirectDraw4, reinterpret_cast<LPVOID *>(&lpDD4)) != DD_OK) return TRUE;
    _this->devices.push_back({
        lpDriverName,
        lpDriverDescription,
        lpDD4
    });

    return TRUE;
}

