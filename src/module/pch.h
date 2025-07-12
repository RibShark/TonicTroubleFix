#pragma once

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#include <windows.h>

#pragma comment(linker, "/EXPORT:DirectInputCreateA=C:\\Windows\\System32\\dinput.DirectInputCreateA")
