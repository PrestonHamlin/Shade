#include "ShaderToyScene.h"

#include "Widgets.h"

ShaderToyScene::ShaderToyScene(std::wstring name) :
    m_name(name),
    m_modelMatrix(),
    m_viewMatrix(),
    m_projectionMatrix(),
    m_constantBufferData({})
{
}
ShaderToyScene::~ShaderToyScene()
{
}

void ShaderToyScene::Init(Dx12RenderEngine* pEngine)
{
    m_pEngine = pEngine;

    m_mesh.LoadFromFile("./media/rotated_teapot.ply");
    m_camera.SetPosition({0, 10, -45});
    m_camera.SetDirection({0, -10, 45});
    PipelineCreateInfo pipelineCreateInfo = {};
    m_pipelineState.Init(pipelineCreateInfo);
    m_pipelineState.SetClearColor(m_clearColor);
    m_pipelineState.AddMesh(&m_mesh);

    m_viewport.Init("primary pipeline RTV");
}

void ShaderToyScene::BuildUI()
{
    // camera controls
    {
        ImGui::Begin("Camera Controls");
        XMFLOAT3 position  = m_camera.GetPosition();
        XMFLOAT3 direction = m_camera.GetDirection();
        XMFLOAT3 upVector  = m_camera.GetUpVector();
        ImGui::Text("Position:  %f, %f, %f", position.x, position.y, position.z);
        ImGui::Text("Direction: %f, %f, %f", direction.x, direction.y, direction.z);
        ImGui::Text("Up Vector: %f, %f, %f", upVector.x, upVector.y, upVector.z);
        ImGui::Text("Depth:     [%f, %f]", m_camera.GetNearZ(), m_camera.GetFarZ());
        if (ImGui::Button("+X")) m_camera.Move({1,0,0});    ImGui::SameLine();
        if (ImGui::Button("-X")) m_camera.Move({-1,0,0});   ImGui::SameLine();
        if (ImGui::Button("+Y")) m_camera.Move({0,1,0});    ImGui::SameLine();
        if (ImGui::Button("-Y")) m_camera.Move({0,-1,0});   ImGui::SameLine();
        if (ImGui::Button("+Z")) m_camera.Move({0,0,1});    ImGui::SameLine();
        if (ImGui::Button("-Z")) m_camera.Move({0,0,-1});

        if (ImGui::Button("yaw left"))   m_camera.YawLeft();    ImGui::SameLine();
        if (ImGui::Button("yaw right"))  m_camera.YawRight();   ImGui::SameLine();
        if (ImGui::Button("pitch up"))   m_camera.PitchUp();    ImGui::SameLine();
        if (ImGui::Button("pitch down")) m_camera.PitchDown();

        if (ImGui::Button("MinZ++")) m_camera.IncreaseNearZ();  ImGui::SameLine();
        if (ImGui::Button("MinZ--")) m_camera.DecreaseNearZ();  ImGui::SameLine();
        if (ImGui::Button("MaxZ++")) m_camera.IncreaseFarZ();   ImGui::SameLine();
        if (ImGui::Button("MaxZ--")) m_camera.DecreaseFarZ();

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

    // viewports
    m_viewport.DrawUI();
}

void ShaderToyScene::OnUpdate()
{
    m_camera.Update();

    // model
    // TODO: model coordinates

    // view
    m_viewMatrix = m_camera.GetViewMatrix();

    // projection
    //m_projectionMatrix = XMMatrixPerspectiveFovLH(XMConvertToRadians(m_fieldOfViewAngle), m_aspectRatio, 0.1f, 1.0f);
    m_projectionMatrix = m_camera.GetProjectionMatrix();

    // update MVP matrix
    //XMMATRIX mvpMatrix = XMMatrixMultiply(m_ModelMatrix, m_ViewMatrix);
    XMMATRIX mvpMatrix = XMMatrixMultiply(XMMatrixIdentity(), m_viewMatrix);
    mvpMatrix = XMMatrixMultiply(mvpMatrix, m_projectionMatrix);
    m_constantBufferData.MVP = mvpMatrix;
}

void ShaderToyScene::OnRender()
{
    CbvData frameConstants = {&m_constantBufferData, sizeof(m_constantBufferData)};
    CD3DX12_VIEWPORT newViewport(0.f, 0.f, 800, 800, m_camera.GetNearZ(), m_camera.GetFarZ());
    m_pipelineState.SetViewport(newViewport);
    m_pipelineState.SetConstantBufferData(frameConstants);
    m_pipelineState.Render();
    m_pipelineState.Execute();
    m_pEngine->CopyResource(m_viewport.GetResource(), m_pipelineState.GetRenderTarget());
}
