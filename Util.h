#pragma once

// codecvt is depricated in C++17 due to unicode security concerns, but the replacement is not in yet
#define _SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING 1
#define _SILENCE_ALL_CXX17_DEPRECATION_WARNINGS 1
//#define NOMINMAX 1


#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <string>
#include <chrono>
#include <time.h>
#include <limits>
#include <filesystem>
//#include <system_error>
//#include <locale>
//#include <codecvt>


// TODO: abstract away resources and their management via engine classes and API
#include <Windows.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl.h>

//#include <fmt/format.h>
#include "imported/fmt/include/fmt/format.h"

//#include <experimental/filesystem>


using Microsoft::WRL::ComPtr;


using uint		= uint32_t;
using uint64	= uint64_t;


//enum class MessageSeverity
enum MessageSeverity
{
    Info,
    Warning,
    Error,
    FatalError,
};
// TODO: constexpr?
static const char* MessageSeverityTags[] =
{
    "[INFO]",
    "[WARNING]",
    "[ERROR]",
    "[FATAL ERROR]"
};


template <typename TP>
std::time_t TimeConvert(TP tp)
{
    using namespace std::chrono;
    auto sctp = time_point_cast<system_clock::duration>(tp - TP::clock::now() + system_clock::now());
    return system_clock::to_time_t(sctp);
}




std::wstring ToWideString(const std::string& str);
std::string ToNormalString(const std::wstring& wstr);




static void PrintMessageHelper(const std::string& format, fmt::format_args args)
{
    //fmt::vprint(format, args);
    OutputDebugStringA(fmt::vformat(format, args).c_str());
}
template <typename... Args>
static void PrintMessage(const std::string& format, const Args& ... args) // bring your own newlines!
{
    PrintMessageHelper(format, fmt::make_format_args(args...));
}
template <typename... Args>
static void PrintMessage(MessageSeverity severity, const std::string& format, const Args& ... args)
{
    std::stringstream ss;
    ss << MessageSeverityTags[static_cast<UINT>(severity)] << " " << format << "\n";
    PrintMessageHelper(ss.str(), fmt::make_format_args(args...));
}



void CheckResult(HRESULT hr, const std::string& msg = "", bool except=false);
void CheckResult(HRESULT hr, const std::wstring& msg);

const std::string HumanReadableFileSize(uint fileSize);

void SetDebugName(ID3D12Object* pObject, std::string name);

inline HRESULT ReadDataFromFile(LPCWSTR filename, byte** data, UINT* size)
{
    using namespace Microsoft::WRL;

    CREATEFILE2_EXTENDED_PARAMETERS extendedParams = {};
    extendedParams.dwSize = sizeof(CREATEFILE2_EXTENDED_PARAMETERS);
    extendedParams.dwFileAttributes = FILE_ATTRIBUTE_NORMAL;
    extendedParams.dwFileFlags = FILE_FLAG_SEQUENTIAL_SCAN;
    extendedParams.dwSecurityQosFlags = SECURITY_ANONYMOUS;
    extendedParams.lpSecurityAttributes = nullptr;
    extendedParams.hTemplateFile = nullptr;

    Wrappers::FileHandle file(CreateFile2(filename, GENERIC_READ, FILE_SHARE_READ, OPEN_EXISTING, &extendedParams));
    if (file.Get() == INVALID_HANDLE_VALUE)
    {
        throw std::exception();
    }

    FILE_STANDARD_INFO fileInfo = {};
    if (!GetFileInformationByHandleEx(file.Get(), FileStandardInfo, &fileInfo, sizeof(fileInfo)))
    {
        throw std::exception();
    }

    if (fileInfo.EndOfFile.HighPart != 0)
    {
        throw std::exception();
    }

    *data = reinterpret_cast<byte*>(malloc(fileInfo.EndOfFile.LowPart));
    *size = fileInfo.EndOfFile.LowPart;

    if (!ReadFile(file.Get(), *data, fileInfo.EndOfFile.LowPart, nullptr, nullptr))
    {
        throw std::exception();
    }

    return S_OK;
}
