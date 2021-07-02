#include "RenderEngine.h"

using namespace Microsoft::WRL;


RenderEngine::RenderEngine(UINT width, UINT height, std::wstring name) :
    m_width(width),
    m_height(height),
    m_name(name),
    m_useWarpDevice(false)
{
    m_aspectRatio = static_cast<float>(width) / static_cast<float>(height);
}

RenderEngine::~RenderEngine()
{
}
