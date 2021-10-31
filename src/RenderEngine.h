#pragma once

#include "Common.h"
#include "Shade.h"

#include "Util.h"


class RenderEngine
{
public:
    RenderEngine(UINT width, UINT height, std::wstring name);
    virtual ~RenderEngine();

    // primary interfaces for render loop
    virtual void Init(const HWND window) = 0;
    virtual void OnUpdate() = 0;
    virtual void PreRender() = 0;
    virtual void OnRender() = 0;
    virtual void PostRender() = 0;
    virtual void Flush() = 0;
    virtual void OnDestroy() = 0;

    // other controls
    //virtual void OnResize(uint width, uint height, MessageSizeType type) = 0;
    virtual void OnKeyDown(UINT8 key)   {}
    virtual void OnKeyUp(UINT8 key)     {}

    uint GetWidth() const               {return m_width;}
    uint GetHeight() const              {return m_height;}
    const std::wstring GetName() const  {return m_name;}

protected:
    uint m_width;
    uint m_height;
    float m_aspectRatio;
    bool m_useWarpDevice;

    HWND m_window;
    std::wstring m_name;
};
