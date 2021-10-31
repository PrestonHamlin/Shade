#include "Common.h"

using namespace std;


string ToString(WindowResolution resolution)
{
    switch (resolution)
    {
#define X(RESOLUTION, WIDTH, HEIGHT) \
    case WindowResolution::WindowResolution ## RESOLUTION : \
        return #RESOLUTION;
        WINDOW_RESOLUTIONS_LIST
#undef X
    default:
    {
        return "INVALID";
    }
    }
}

pair<uint, uint> WindowResolutionSizes(WindowResolution resolution)
{
    switch (resolution)
    {
#define X(RESOLUTION, WIDTH, HEIGHT) \
    case WindowResolution::WindowResolution ## RESOLUTION : \
        return {WIDTH, HEIGHT};
        WINDOW_RESOLUTIONS_LIST
#undef X
    default:
    {
        return {0, 0};
    }
    }
}
