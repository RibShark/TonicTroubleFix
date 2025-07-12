#ifndef INIFILE_H
#define INIFILE_H

#include <filesystem>

class IniFile {
public:
    DWORD width, height, bpp, antialiasing, anisotropy, gliBuffering, gliAlphaTest;
    CString language, gliDll, gliDriver, gliTexturesMode;
    bool windowed;

    IniFile(const std::filesystem::path&, std::initializer_list<std::filesystem::path>);
    void WriteSettingsToFile();

private:
    std::filesystem::path iniFileName;

    bool ReadSettingsFromFile(const std::filesystem::path& iniFileName);
};

#endif //INIFILE_H
