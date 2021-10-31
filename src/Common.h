// Configures libraries and provides some common definitions
#pragma once

//**********************************************************************************************************************
//                                                   Windows & D3D12
//**********************************************************************************************************************
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#define UNICODE 1   // painful as it is, unicode support is useful...
#define NOMINMAX 1  // prevent min/max macros which can interfere with STL

// Windows
#include <windows.h>
#include <shellapi.h>   // command line args
#include <wrl.h>        // Com

// D3D
#include <d3d12.h>
#include <dxgi1_4.h>
//#include <D3Dcompiler.h>
#include <DirectXMath.h>    // SimpleMath from DirectXTK12 wraps this

#include "d3dx12.h"     // redistributable utility header


//**********************************************************************************************************************
//                                            C++ Standard Template Library
//**********************************************************************************************************************
#include <mutex>
#include <string>
#include <thread>
#include <utility>


//**********************************************************************************************************************
//                                                     Magic Enum
//**********************************************************************************************************************
// Magic Enum provides static reflection for enums. This allows for string and integer conversions, as well as ranging
//  and sequencing. The implementation has limitations, and increases compile time.
// https://github.com/Neargye/magic_enum
#include "magic_enum.hpp"
static_assert(MAGIC_ENUM_SUPPORTED == 1);

// Magic Enum's default range to scan for enum values is [-128,128] which is insufficient for some API enumerations.
//  https://github.com/Neargye/magic_enum/blob/master/doc/limitations.md
namespace magic_enum::customize {
    template <>
    struct enum_range<DXGI_FORMAT> {
        static constexpr int min = -1;  // DXGI_FORMAT_FORCE_UINT
        static constexpr int max = 190; // DXGI_FORMAT_SAMPLER_FEEDBACK_MIP_REGION_USED_OPAQUE
    };
}


//**********************************************************************************************************************
//                                                      Typedefs
//**********************************************************************************************************************
using uint = uint32_t;
using uint64 = uint64_t;


//**********************************************************************************************************************
//                                                    Wrapper Enums
//**********************************************************************************************************************
enum class MessageSizeType // WM_SIZE wParam macro values in WinUser.h
{
    SizeRestored  = SIZE_RESTORED,
    SizeMinimized = SIZE_MINIMIZED,
    SizeMaximized = SIZE_MAXIMIZED,
    SizeMaxShow   = SIZE_MAXSHOW,
    SizeMaxHide   = SIZE_MAXHIDE,
};


static float BLACK[] = {0.0, 0.0, 0.0, 1.0};
static float RED[]   = {1.0, 0.0, 0.0, 1.0};
static float GREEN[] = {0.0, 1.0, 0.0, 1.0};
static float BLUE[]  = {0.0, 0.0, 1.0, 1.0};
static float WHITE[] = {1.0, 1.0, 1.0, 1.0};

// Glorious X-Macros. They reduce readability and take a bit to understand, but improve maintainability. Say we want an
//  enum, an array of strings which describe each enum value, and a data element per enum value. If we modified one, we
//  would need to modify all three. Instead, we can keep a single list macro and unpack it inside other macros. You can
//  liken them to passing a generator to a function in Python, but at compile-time and far less pretty.
// We can still cast the enum value and use it as an index into the other arrays, but it's better to write some helper
//  functions which convert between them. We can use the macros inside a switch statement just as easily.
//
// Visual Studio provides a convenient macro expansion preview on mouse hover, so it's not complete black magick.
//
// MagicEnum library is a nice alternative if casting and string-ification is the main desire, but doesn't do tables.
#define WINDOW_RESOLUTIONS_LIST \
    X(800x800,      800,  800)  \
    X(1024x1024,    1024, 1024) \
    X(1280x720,     1280, 720)  \
    X(1600x900,     1600, 900)  \
    X(1920x1080,    1920, 1080) \
    X(2560x1080,    2560, 1080) \
    X(2560x1440,    2560, 1440) \
    X(3840x2160,    3840, 2160) \
    X(Invalid,         0,    0) 


//enum WindowResolution
//{
//    WindowResolution800x800,    // 1:1
//    WindowResolution1024x1024,  // 1:1
//    WindowResolution1280x720,   // 16:9 720p
//    WindowResolution1600x900,   // 16:9 900p
//    WindowResolution1920x1080,  // 16:9 1080p
//    WindowResolution2560x1080,  // 21:9 1080p Ultrawide
//    WindowResolution2560x1440,  // 16:9 1440p
//    WindowResolution3840x2160,  // 16:9 4K UHD
//    WindowResolutionInvalid,
//};
enum class WindowResolution
{
#define X(RESOLUTION, WIDTH, HEIGHT) WindowResolution ## RESOLUTION,
    WINDOW_RESOLUTIONS_LIST
#undef X
};

std::string ToString(WindowResolution);
std::pair<uint, uint> WindowResolutionSizes(WindowResolution);
