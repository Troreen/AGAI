#include "FileDialogs.h"
#include <commdlg.h>
#include <tge/util/StringCast.h>

#pragma comment(lib, "comdlg32.lib")

namespace Tga
{
    bool FileDialogs::OpenFile(HWND owner, const wchar_t* filter, const wchar_t* title, std::string& outPath)
    {
        wchar_t szFile[MAX_PATH] = { 0 };

        OPENFILENAMEW ofn = { 0 };
        ofn.lStructSize = sizeof(OPENFILENAMEW);
        ofn.hwndOwner = owner;
        ofn.lpstrFilter = filter ? filter : L"All Files (*.*)\0*.*\0";
        ofn.lpstrFile = szFile;
        ofn.nMaxFile = MAX_PATH;
        ofn.lpstrTitle = title ? title : L"Select File";
        ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;

        if (GetOpenFileNameW(&ofn))
        {
            outPath = string_cast<std::string>(std::wstring(szFile));
            return true;
        }

        return false;
    }

    bool FileDialogs::SaveFile(HWND owner, const wchar_t* filter, const wchar_t* defaultExt, const wchar_t* defaultFileName, const wchar_t* title, std::string& outPath)
    {
        wchar_t szFile[MAX_PATH] = { 0 };
        if (defaultFileName)
        {
            wcsncpy_s(szFile, defaultFileName, MAX_PATH - 1);
        }

        OPENFILENAMEW ofn = { 0 };
        ofn.lStructSize = sizeof(OPENFILENAMEW);
        ofn.hwndOwner = owner;
        ofn.lpstrFilter = filter ? filter : L"DDS Files (*.dds)\0*.dds\0All Files (*.*)\0*.*\0";
        ofn.lpstrFile = szFile;
        ofn.nMaxFile = MAX_PATH;
        ofn.lpstrDefExt = defaultExt ? defaultExt : L"dds";
        ofn.lpstrTitle = title ? title : L"Save File As";
        ofn.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT | OFN_NOCHANGEDIR;

        if (GetSaveFileNameW(&ofn))
        {
            outPath = string_cast<std::string>(std::wstring(szFile));
            return true;
        }

        return false;
    }
}
