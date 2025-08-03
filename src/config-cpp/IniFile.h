#ifndef INIFILE_H
#define INIFILE_H

#include <filesystem>

enum WindowMode : UINT
{
    Fullscreen,
    Borderless,
    Windowed
};

class IniFile {
public:
    int width, height, bpp, windowMode, antialiasing, anisotropy, gliBuffering, gliAlphaTest;
    bool vsync;
    CString language, gliDll, gliDriver, gliTexturesMode;

    IniFile(const std::filesystem::path&, std::initializer_list<std::filesystem::path>);
    void WriteSettingsToFile();

private:
    std::filesystem::path iniFileName;

    bool ReadSettingsFromFile(const std::filesystem::path& iniFileName);
};

#endif //INIFILE_H
