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
#include <Windows.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl.h>

#include <fmt/format.h>

#include "d3dx12.h"
#include "Common.h"

using Microsoft::WRL::ComPtr;

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


//**********************************************************************************************************************
//                                                      Converters
//**********************************************************************************************************************
template <typename TP>
std::time_t TimeConvert(TP tp)
{
    using namespace std::chrono;
    auto sctp = time_point_cast<system_clock::duration>(tp - TP::clock::now() + system_clock::now());
    return system_clock::to_time_t(sctp);
}

std::wstring ToWideString(const std::string& str);
std::string ToNormalString(const std::wstring& wstr);

const std::string HumanReadableFileSize(uint fileSize);


//**********************************************************************************************************************
//                                              Printing, Logging & Errors
//**********************************************************************************************************************
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


//**********************************************************************************************************************
//                                                      Wrappers
//**********************************************************************************************************************
template <typename ObjectType>
void SetDebugName(ObjectType* pObject, std::string name)
{
    pObject->SetPrivateData(WKPDID_D3DDebugObjectName, name.size(), name.c_str());
}


//**********************************************************************************************************************
//                                                      Math
//**********************************************************************************************************************
void* PointerByteIncrement(void* ptr, uint bytes);
