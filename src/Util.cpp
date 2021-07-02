#include "Util.h"

#include <system_error>
#include <locale>
#include <codecvt>
#include <sstream>
#include <chrono>
#include <debugapi.h>

using namespace std;


std::wstring ToWideString(const std::string& str)
{
    using convert_typeX = std::codecvt_utf8<wchar_t>;
    static std::wstring_convert<convert_typeX, wchar_t> converter;

    return converter.from_bytes(str);
}

std::string ToNormalString(const std::wstring& wstr)
{
    using convert_typeX = std::codecvt_utf8<wchar_t>;
    static std::wstring_convert<convert_typeX, wchar_t> converter;

    return converter.to_bytes(wstr);
}

// TODO: wrapper macro and/or LINE preprocessor definition?
void CheckResult(HRESULT hr, const std::string& msg, bool except)
{
    if (FAILED(hr))
    {
        std::string outputMessage = std::system_category().message(hr) + "\n\t" + msg + "\n";
        PrintMessage(Error, outputMessage);

        if (except)
        {
            throw std::exception();
        }
    }
}

void CheckResult(HRESULT hr, const std::wstring& msg)
{
    CheckResult(hr, ToNormalString(msg));
}

const std::string HumanReadableFileSize(uint fileSize)
{
    uint scale = 0;
    double value = fileSize;
    std::stringstream ss;
    constexpr char* units[] = {"  B", "KiB", "MiB", "GiB", "TiB", "PiB"};

    //do
    //while(value >= 1024)
    for (; value >= 1024; ++scale)
    {
        value /= 1024;
        //scale++;
        //value = std::ceil(value * 100) / 100; // drop excessive mantissa bits
    }
    //while(value >= 1024);

    // std::to_string does not allow for specifying significant digits
    ss.precision(scale == 0 ? 0 : 2);
    ss << std::fixed << std::setw(8) << std::left << value << units[scale];

    return ss.str();
}
