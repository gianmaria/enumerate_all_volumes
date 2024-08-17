// This is an open source non-commercial project. Dear PVS-Studio, please check it.

// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com

#define NOMINMAX
#include <Windows.h>

#include <locale.h>

#include <iostream>
#include <fstream>
#include <print>
#include <string>
#include <vector>
#include <cstdio>
#include <array>
#include <locale>

using u8 = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;
using u64 = uint64_t;

using i8 = int8_t;
using i16 = int16_t;
using i32 = int32_t;
using i64 = int64_t;

using std::cout;
using std::wcout;
using std::endl;

using std::string;
using std::string_view;
using std::wstring;

using std::vector;
using std::array;

using str = std::string;
using str_cref = const std::string&;
using str_view = std::string_view;

using wstr = std::wstring;
using wstr_cref = const std::wstring&;
using wstr_view = std::wstring_view;

template<typename T>
using vec = vector<T>;

using namespace std::string_literals;
using namespace std::string_view_literals;


str to_UTF8(wstr_view wide_str)
{
    if (wide_str == L"")
        return "";

    int size = WideCharToMultiByte(
        CP_UTF8, // code page
        0, // flags, zero for codepage CP_UTF8
        wide_str.data(), // unicode string to convert
        wide_str.size(), // size, in characters, of the string to convert
        nullptr, // pointer to a buffer that receives the converted string
        0, // size, in bytes, of the buffer indicated
        nullptr, // zero for code page CP_UTF8
        nullptr // zero for code page CP_UTF8
    );

    if (size == 0)
        return "";

    auto utf8_str = str(size, '\0');

    WideCharToMultiByte(
        CP_UTF8, 
        0, 
        wide_str.data(), 
        wide_str.size(),
        utf8_str.data(),
        size, 
        nullptr, 
        nullptr
    );

    return utf8_str;
}

wstr to_UTF16(str_view utf8_str)
{
    if (utf8_str == "")
        return L"";

    int size = MultiByteToWideChar(
        CP_UTF8, 
        0, 
        utf8_str.data(),
        utf8_str.size(),
        nullptr, 
        0);

    if (size == 0)
        return L"";

    wstr wide_str(size, L'\0');

    MultiByteToWideChar(
        CP_UTF8, 
        0, 
        utf8_str.data(),
        utf8_str.size(), 
        wide_str.data(),
        size);

    return wide_str;
}

str last_error_as_string(DWORD last_error)
{
    auto buffer = array<wchar_t, 2048>();

    DWORD size = FormatMessageW(
        FORMAT_MESSAGE_FROM_SYSTEM |
        FORMAT_MESSAGE_IGNORE_INSERTS |
        FORMAT_MESSAGE_MAX_WIDTH_MASK,
        NULL,
        last_error,
        0,
        (wchar_t*)buffer.data(),
        buffer.size(),
        NULL);

    return to_UTF8(wstr(buffer.data()));
}


vec<str> getVolumes()
{
    vec<str> res;

    auto buffer = array<wchar_t, 1024>();

    HANDLE handle = FindFirstVolumeW(buffer.data(), buffer.size());

    if (handle == INVALID_HANDLE_VALUE)
    {
        std::println("FindFirstVolumeW failed with code ({}): '{}'",
                     GetLastError(), last_error_as_string(GetLastError()));
        return res;
    }

    res.push_back(to_UTF8({buffer.data(), wcslen(buffer.data())}));

    while (true)
    {
        BOOL success = FindNextVolumeW(handle,
                                       buffer.data(),
                                       buffer.size());

        if (success != 0)
        {
            res.push_back(to_UTF8({buffer.data(), wcslen(buffer.data())}));
        }
        else
        {
            if (GetLastError() == ERROR_NO_MORE_FILES)
            {
                // good we done here
                break;
            }
            else
            {
                std::println("FindNextVolumeW failed with code ({}): '{}'",
                             GetLastError(), last_error_as_string(GetLastError()));
                break;
            }
        }
    }

    FindVolumeClose(handle);

    return res;
}

vec<str> getDeviceNameForVolume(str volume)
{
    vec<str> res;

    volume = volume.erase(0, 4); // remove '\\?\'
    volume.pop_back(); // remove '\'

    auto buffer = array<wchar_t, 2048>();

    DWORD actual_size = QueryDosDeviceW(to_UTF16(volume).data(),
                                        buffer.data(),
                                        buffer.size());

    if (actual_size == 0)
    {
        std::println("QueryDosDeviceW failed with code ({}): '{}'",
                     GetLastError(), last_error_as_string(GetLastError()));
        return res;
    }

    const wchar_t* dd = buffer.data();

    while (*dd)
    {
        res.push_back(to_UTF8({dd, wcslen(dd)}));
        
        dd += wcslen(dd) + 1;
    }

    return res;
}

str getDriveLetter(str_view guid)
{
    auto buffer = array<wchar_t, 2048>();
    DWORD return_length = 0;

    BOOL res = GetVolumePathNamesForVolumeNameW(
        to_UTF16(guid).data(),
        buffer.data(), buffer.size(),
        &return_length
    );

    if (res == 0)
    {
        std::println("GetVolumePathNamesForVolumeNameW failed with code ({}): '{}'",
                     GetLastError(), last_error_as_string(GetLastError()));
        return "";
    }

    return to_UTF8({buffer.data(), wcslen(buffer.data())});
}

int main()
{
    auto volumes = getVolumes();

    cout << "found following volumes 😁:" << endl;
    for (const auto& volume : volumes)
    {
        cout << "vol: " << volume << endl;

        auto devices = getDeviceNameForVolume(volume);

        for (const auto& device : devices)
        {
            cout << "  device: " << device << endl;
            cout << "  mount path: " << getDriveLetter(volume) << endl;
        }

        cout << endl;
    }

    return 0;
}
