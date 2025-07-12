#include "IniFile.h"

#include <utility>

IniFile::IniFile(const std::filesystem::path& mainIniFileName, std::initializer_list<std::filesystem::path> iniFileNames)
{
    iniFileName = mainIniFileName;

    // default settings
    width = 640;
    height = 480;
    bpp = 32;
    windowed = false;
    antialiasing = 0;
    anisotropy = 0;

    gliDll = L"GliDX6vr.dll";
    gliDriver = L"Primary Display Driver";
    gliTexturesMode = L"Full";
    gliBuffering = 2;
    gliAlphaTest = 1;

    for (const std::filesystem::path& file : iniFileNames) {
        ReadSettingsFromFile(file);
    }
    ReadSettingsFromFile(mainIniFileName);
}

bool IniFile::ReadSettingsFromFile(const std::filesystem::path& iniFileName)
{
    const auto appName = L"TONICT";
    const wchar_t *szIniFileName = iniFileName.c_str();

    if (!PathFileExists(szIniFileName)) {
        return false;
    }
    width = GetPrivateProfileInt(appName, L"Width", width, szIniFileName);
    height = GetPrivateProfileInt(appName, L"Height", height, szIniFileName);
    bpp = GetPrivateProfileInt(appName, L"BPP", bpp, szIniFileName);
    windowed = GetPrivateProfileInt(appName, L"Windowed", windowed, szIniFileName);
    antialiasing = GetPrivateProfileInt(appName, L"Antialiasing", antialiasing, szIniFileName);
    anisotropy = GetPrivateProfileInt(appName, L"Anisotropy", anisotropy, szIniFileName);

    wchar_t value[MAX_PATH];
    GetPrivateProfileString(appName, L"GliDll", gliDll, value, MAX_PATH, szIniFileName);
    gliDll = value;
    GetPrivateProfileString(appName, L"GliDriver", gliDriver, value, MAX_PATH, szIniFileName);
    gliDriver = value;
    GetPrivateProfileString(appName, L"GliTexturesMode", gliTexturesMode, value, MAX_PATH, szIniFileName);
    gliTexturesMode = value;
    gliBuffering = GetPrivateProfileInt(appName, L"GliBuffering", gliBuffering, szIniFileName);
    gliAlphaTest = GetPrivateProfileInt(appName, L"GliAlphaTest", gliAlphaTest, szIniFileName);

    return true;
}

void IniFile::WriteSettingsToFile()
{
    const auto appName = L"TONICT";
    const wchar_t *szIniFileName = iniFileName.c_str();

    WritePrivateProfileString(appName, L"Width", ToCString(width), szIniFileName);
    WritePrivateProfileString(appName, L"Height", ToCString(height), szIniFileName);
    WritePrivateProfileString(appName, L"BPP", ToCString(bpp), szIniFileName);
    WritePrivateProfileString(appName, L"Windowed", ToCString(windowed), szIniFileName);
    WritePrivateProfileString(appName, L"Antialiasing", ToCString(antialiasing), szIniFileName);
    WritePrivateProfileString(appName, L"Anisotropy", ToCString(anisotropy), szIniFileName);

    WritePrivateProfileString(appName, L"GliDll", gliDll, szIniFileName);
    WritePrivateProfileString(appName, L"GliDriver", gliDriver, szIniFileName);
    WritePrivateProfileString(appName, L"GliTexturesMode", gliTexturesMode, szIniFileName);
    WritePrivateProfileString(appName, L"GliBuffering", ToCString(gliBuffering), szIniFileName);
    WritePrivateProfileString(appName, L"GliAlphaTest", ToCString(gliAlphaTest), szIniFileName);
}