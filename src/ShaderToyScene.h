#pragma once

#include "Scene.h"
#include "Viewport.h"
#include "PipelineState.h"
#include "Camera.h"

class ShaderToyScene : public Scene
{

public:
    ShaderToyScene(UINT width, UINT height, std::wstring name);
    ~ShaderToyScene();

    void Init(HWND window, Dx12RenderEngine* pEngine);
    void BuildUI();
    void SetWindow(HWND window) {m_window = window;}

    void OnUpdate();
    void OnRender();

private:
    struct SceneConstantBuffer
    {
        XMFLOAT4 angles;
        XMMATRIX MVP;
    };

    Dx12RenderEngine*                   m_pEngine;
    PipelineState                       m_pipelineState;
    Mesh                                m_mesh;
    Camera                              m_camera;

    // scene data
    float                               m_clearColor[4] = {0.0f, 0.2f, 0.4f, 1.0f};
    SceneConstantBuffer                 m_constantBufferData;
    DirectX::XMMATRIX                   m_modelMatrix;
    DirectX::XMMATRIX                   m_viewMatrix;
    DirectX::XMMATRIX                   m_projectionMatrix;
    float                               m_aspectRatio;
    float                               m_fieldOfViewAngle;

    // UI
    HWND                                m_window;
    bool                                m_fullscreen;
    bool                                m_showDebugConsole;
    bool                                m_showImGuiDemoWindow;
    bool                                m_showImGuiMetrics;
    bool                                m_showImGuiStyleEditor;
    uint                                m_width, m_height;
    RECT                                m_windowPosition;
    Viewport                            m_viewport;
};
