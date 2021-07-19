#pragma once

#include "Scene.h"
#include "Viewport.h"
#include "PipelineState.h"
#include "Camera.h"

class ShaderToyScene : public Scene
{

public:
    ShaderToyScene(std::wstring name);
    ~ShaderToyScene();

    void Init(Dx12RenderEngine* pEngine);
    void BuildUI();
    void OnUpdate();
    void OnRender();

private:
    struct SceneConstantBuffer
    {
        XMFLOAT4 angles;
        XMMATRIX MVP;
    };

    // components
    Dx12RenderEngine*                   m_pEngine;
    PipelineState                       m_pipelineState;
    Mesh                                m_mesh;
    Camera                              m_camera;
    ViewportRenderTarget                m_viewportForRtv;

    // scene data
    std::wstring                        m_name;
    float                               m_clearColor[4] = {0.0f, 0.2f, 0.4f, 1.0f};
    SceneConstantBuffer                 m_constantBufferData;
    DirectX::XMMATRIX                   m_modelMatrix;
    DirectX::XMMATRIX                   m_viewMatrix;
    DirectX::XMMATRIX                   m_projectionMatrix;
};
