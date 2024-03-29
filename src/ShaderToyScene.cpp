#include "ShaderToyScene.h"

#include "Widgets.h"
#include <imgui_internal.h>

ShaderToyScene::ShaderToyScene(std::wstring name) :
    m_name(name),
    m_constantBufferData({})
{
}
ShaderToyScene::~ShaderToyScene()
{
}

void ShaderToyScene::Init(Dx12RenderEngine* pEngine)
{
    m_pEngine = pEngine;

    m_geometryManager.Init();
    uint teapotID = m_geometryManager.AddMesh("./media/rotated_teapot.ply");
    uint cubeID = m_geometryManager.AddMesh("./media/colored_cube.ply");

    m_camera.SetPosition({0, 5, -45});
    m_camera.SetDirection({0, 0, 1});

    // TODO: actually use PipelineCreateInfo...
    PipelineCreateInfo pipelineCreateInfo = {};
    m_pipelineState.Init(pipelineCreateInfo);
    m_pipelineState.RegisterGeometryManager(&m_geometryManager);
    m_pipelineState.SetConstantBufferData({&m_constantBufferData, sizeof(m_constantBufferData)});
    m_pipelineState.SetClearColor(m_clearColor);

    m_viewportForRtv.Init("RTV Viewport", m_pipelineState.GetRenderTarget());
    m_viewportForDepth.Init("Depth Viewport", m_pipelineState.GetDepthStencil());
}

void ShaderToyScene::BuildUI()
{
    // camera controls
    {
        ImGui::Begin("Camera Controls");
        XMFLOAT3 position = m_camera.GetPosition();
        XMFLOAT3 direction = m_camera.GetDirection();
        XMFLOAT3 upVector = m_camera.GetUpVector();
        ImGui::Text("Position:  %f, %f, %f", position.x, position.y, position.z);
        ImGui::Text("Direction: %f, %f, %f", direction.x, direction.y, direction.z);
        ImGui::Text("Up Vector: %f, %f, %f", upVector.x, upVector.y, upVector.z);
        ImGui::Text("Depth:     [%f, %f]", m_camera.GetNearZ(), m_camera.GetFarZ());
        if (ImGui::Button("+X")) m_camera.Move({ 1,0,0 });    ImGui::SameLine();
        if (ImGui::Button("-X")) m_camera.Move({ -1,0,0 });   ImGui::SameLine();
        if (ImGui::Button("+Y")) m_camera.Move({ 0,1,0 });    ImGui::SameLine();
        if (ImGui::Button("-Y")) m_camera.Move({ 0,-1,0 });   ImGui::SameLine();
        if (ImGui::Button("+Z")) m_camera.Move({ 0,0,1 });    ImGui::SameLine();
        if (ImGui::Button("-Z")) m_camera.Move({ 0,0,-1 });

        if (ImGui::Button("yaw left"))   m_camera.YawLeft();    ImGui::SameLine();
        if (ImGui::Button("yaw right"))  m_camera.YawRight();   ImGui::SameLine();
        if (ImGui::Button("pitch up"))   m_camera.PitchUp();    ImGui::SameLine();
        if (ImGui::Button("pitch down")) m_camera.PitchDown();

        if (ImGui::Button("MinZ++")) m_camera.IncreaseNearZ();  ImGui::SameLine();
        if (ImGui::Button("MinZ--")) m_camera.DecreaseNearZ();  ImGui::SameLine();
        if (ImGui::Button("MaxZ++")) m_camera.IncreaseFarZ();   ImGui::SameLine();
        if (ImGui::Button("MaxZ--")) m_camera.DecreaseFarZ();

        if (ImGui::Checkbox("Reverse Depth", &m_reverseDepth))
        {
            m_camera.ReverseZ(m_reverseDepth);
            m_pipelineState.SetReverseDepth(m_reverseDepth);
        }

        ImGui::End();
    }

    // clear-color picker
    {
        ImVec4 pickerColor(m_clearColor[0], m_clearColor[1], m_clearColor[2], m_clearColor[3]);

        ImGui::Begin("color picker");
        if (ImGui::ColorButton("button", pickerColor))
        {
            ImGui::OpenPopup("colorPickerPopup");
        }

        // describe color-picker popup
        if (ImGui::BeginPopup("colorPickerPopup"))
        {
            const uint flags = ImGuiColorEditFlags_PickerHueWheel | // triangle inside wheel
                               ImGuiColorEditFlags_NoSmallPreview | // no (duplicate) mini previews next to text fields
                               ImGuiColorEditFlags_NoSidePreview  | // no preview beside picker
                               ImGuiColorEditFlags_NoLabel;         // no element name as text
            ImGui::ColorPicker4("fancyColorPicker", m_clearColor, flags);
            ImGui::Text("Custom text in the custom popup");
            ImGui::EndPopup();
        }

        ImGui::End();
    }

    // file browser widget
    {
        ImGui::Begin("File Browser");

        // static field to be updated by browser popup
        static char filePath[1024] = "file path";
        FilePickerWidget(filePath, IM_ARRAYSIZE(filePath));

        ImGui::End(); // file browser
    }

    // node editor
    {
        m_nodeEditor.Update();
        m_nodeEditor.Draw();
    }

    // geometry management
    m_geometryManager.BuildUI();

    // viewports
    m_viewportForRtv.DrawUI();
    m_viewportForDepth.DrawUI();
}

void ShaderToyScene::OnUpdate()
{
    m_camera.Update();

    // provide view and projection matrices to shader
    XMStoreFloat4x4(&m_constantBufferData.viewMatrix, XMMatrixTranspose(m_camera.GetViewMatrix()));
    XMStoreFloat4x4(&m_constantBufferData.projectionMatrix, XMMatrixTranspose(m_camera.GetProjectionMatrix()));
}

void ShaderToyScene::OnRender()
{
    m_pipelineState.UpdateConstantBufferData();
    m_pipelineState.Render();
    m_pipelineState.Execute();
}
