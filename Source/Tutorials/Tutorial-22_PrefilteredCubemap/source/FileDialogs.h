#pragma once

#include <string>
#include <vector>
#include <windows.h>

namespace Tga
{
    class FileDialogs
    {
    public:
        // Opens a standard Windows file open dialog. Returns true if user selected a file.
        // filter format example: L"DDS Files (*.dds)\0*.dds\0Image Files\0*.dds;*.png;*.jpg;*.tga\0All Files (*.*)\0*.*\0"
        static bool OpenFile(HWND owner, const wchar_t* filter, const wchar_t* title, std::string& outPath);

        // Opens a standard Windows file save dialog. Returns true if user specified a file.
        static bool SaveFile(HWND owner, const wchar_t* filter, const wchar_t* defaultExt, const wchar_t* defaultFileName, const wchar_t* title, std::string& outPath);
    };
}
