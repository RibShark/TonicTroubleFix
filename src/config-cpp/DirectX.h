#ifndef DIRECTX_H
#define DIRECTX_H

#include <ddraw.h>
#include <d3d9.h>

struct DirectDrawDevice {
    CString name;
    CString description;
    LPDIRECTDRAW4 lpDD4;
};

struct DisplayMode {
    DWORD width;
    DWORD height;
    DWORD bitsPerPixel;

    bool operator<(const DisplayMode p1) const {
        if (width == p1.width) {
            if (height == p1.height) return bitsPerPixel < p1.bitsPerPixel;
            return height < p1.height;
        }
        return width < p1.width;
    }
};

class DirectX {
public:
    DirectX();
    std::vector<D3DMULTISAMPLE_TYPE> GetAntiAliasingModes(int, bool);
    DWORD GetMaxAnisotropy(int);
    std::vector<DisplayMode> GetDisplayModes(int);
    std::vector<CString> GetDeviceNames();

private:
    LPDIRECT3D9 lpD3D;

    static HRESULT WINAPI EnumModesCallback(LPDDSURFACEDESC2, LPVOID);
    static BOOL WINAPI DDEnumCallback(GUID FAR*, LPSTR, LPSTR, LPVOID, HMONITOR);
    std::vector<DirectDrawDevice> devices;
};

#endif //DIRECTX_H
