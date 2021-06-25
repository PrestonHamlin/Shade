#include "ShaderToyScene.h"

#include "Widgets.h"

ShaderToyScene::ShaderToyScene(UINT width, UINT height, std::wstring name) :
    m_width(width),
    m_height(height),
    m_aspectRatio(float(width)/float(height))
    //m_aspectRatio(float(width)/float(height)),
    //m_viewport(0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height)),
    //m_scissorRect(0, 0, static_cast<LONG>(width), static_cast<LONG>(height))
{

}
ShaderToyScene::~ShaderToyScene()
{

}

void ShaderToyScene::Init(HWND window, Dx12RenderEngine* pEngine)
{
    m_window = window;
    m_pEngine = pEngine;
    GetWindowRect(m_window, &m_windowPosition);

    // TODO: move CBV into pipeline
    m_pCbvDataBegin = pEngine->GetCbvDataBegin();

    PipelineCreateInfo pipelineCreateInfo = {};
    m_pipelineState.Init(pipelineCreateInfo);
    m_pipelineState.SetClearColor(m_clearColor);

    m_viewport.Init("primary pipeline RTV");
}

void ShaderToyScene::BuildUI()
{
    ImGuiIO& io = ImGui::GetIO();
    const ImGuiViewport* mainViewport = ImGui::GetMainViewport();

    // Create dockspace before anything else and fill main viewport. This should also supplant the "debug" viewport.
    ImGui::DockSpaceOverViewport(ImGui::GetMainViewport());

    // main window menu bar containing drop-down menu dialogues and window controls
    if (ImGui::BeginMainMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {
            ImGui::Text("This is some menu text");
            ImGui::MenuItem("New");
            ImGui::MenuItem("Blah");
            if (ImGui::MenuItem("Quit"))
            {
                PostQuitMessage(0);
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Tools"))
        {
            ImGui::Text("This is some menu text");
            ImGui::Separator();
            ImGui::Checkbox("Show Debug Console", &m_showDebugConsole);
            ImGui::Checkbox("Show ImGui Metrics/Debug", &m_showImGuiMetrics);
            ImGui::Checkbox("Show ImGui Style Editor", &m_showImGuiStyleEditor);
            ImGui::Separator();
            ImGui::MenuItem("Foo");
            ImGui::MenuItem("Bar");
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("View"))
        {
            ImGui::Text("This is some menu text");
            ImGui::Separator();
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Help"))
        {
            ImGui::Text("This is some menu text");
            ImGui::Separator();
            if (ImGui::MenuItem("ShowDemoWindow"))
            {
                m_showImGuiDemoWindow = true;
            }
            ImGui::EndMenu();
        }

        // title and status text
        ImGui::Separator();
        ImGui::Spacing();
        ImGui::Text("Shade");

        // custom nav controls for minimize, maximize, drag to move, etc...
        if (ImGui::SmallButton("Drag")) // when button is released, save current window position
        {
            GetWindowRect(m_window, &m_windowPosition);
        }
        if (ImGui::IsItemActive()) // while button is held, redraw with drag offset from original position
        {
            ImVec2 dragDelta = ImGui::GetMouseDragDelta(0);
            ImGui::Text("Drag delta: %f, %f", dragDelta.x, dragDelta.y);

            SetWindowPos(m_window, nullptr, m_windowPosition.left + dragDelta.x, m_windowPosition.top + dragDelta.y, m_width, m_height, 0);
        }

        // invisible button for drag controls rather than hook title bar events?
        static std::string msg = "hello";
        if (ImGui::InvisibleButton("invis", { 10,10 }))
        {
            msg = "world";
        }
        ImGui::Text(msg.c_str());

        // minimize, maximize, quit
        ImGui::SameLine(ImGui::GetWindowWidth()-50);
        ImGui::Separator();
        ImGui::PushStyleColor(ImGuiCol_Button,        (ImVec4)ImColor::HSV(0.0f, 0.6f, 0.6f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (ImVec4)ImColor::HSV(0.0f, 0.8f, 0.8f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive,  (ImVec4)ImColor::HSV(0.0f, 0.8f, 0.3f));
        if (ImGui::Button("\xEE\x83\x9B"))
        {
            PostQuitMessage(0);
        }
        ImGui::PopStyleColor(3);

        ImGui::EndMainMenuBar();
    }

    // ImGui simple box
    {
        ImGui::Begin("test");
        ImGui::Text("this is a test");
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

    // display mode info
    {
        ImGui::Begin("Display Info");
        ImGui::Separator();
        ImGui::Text("ImGui main viewport");
        ImGui::Text("Window: %.1f,%.1f, %.1fx%.1f", mainViewport->Pos.x, mainViewport->Pos.y, mainViewport->Size.x, mainViewport->Size.y);
        ImGui::Text("Client: %.1f,%.1f, %.1fx%.1f", mainViewport->WorkPos.x, mainViewport->WorkPos.y, mainViewport->WorkSize.x, mainViewport->WorkSize.y);
        ImGui::Text("Window: %u,%u, %ux%u", m_left, m_top, m_width, m_height);
        ImGui::Text("Rect:   %i,%i,%i,%i %ix%i", m_windowPosition.left, m_windowPosition.top, m_windowPosition.right, m_windowPosition.bottom,
                                                 m_windowPosition.right - m_windowPosition.left, m_windowPosition.bottom - m_windowPosition.top);
        ImGui::Separator();
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

    // menus opened via main menu bar buttons
    if (m_showDebugConsole)
    {
        ImGui::Begin("Debug Console");
        ImGui::Text("this should be a debug console");
        ImGui::End();
    }
    if (m_showImGuiDemoWindow)
    {
        ImGui::ShowDemoWindow(&m_showImGuiDemoWindow);
    }
    if (m_showImGuiMetrics)
    {
        ImGui::ShowMetricsWindow(&m_showImGuiMetrics);
    }
    if (m_showImGuiStyleEditor)
    {
        ImGui::ShowStyleEditor();
    }

    m_viewport.DrawUI();

    // draw engine-level debug elements such as heap contents
    m_pEngine->DrawDebugUi();

    //ImGui::EndFrame();
    ImGui::Render();
}

void ShaderToyScene::OnUpdate()
{
    //m_pEngine->OnUpdate();

    m_constantBufferData.angles.x += 0.005f;
    m_constantBufferData.angles.y += 0.005f;
    m_constantBufferData.angles.z += 0.005f;

    static uint64_t frameCount = 0;
    frameCount++;

    // model
    float angle = static_cast<float>(frameCount);
    const XMVECTOR rotationAxis = XMVectorSet(0, 1, 1, 0);
    m_ModelMatrix = XMMatrixRotationAxis(rotationAxis, XMConvertToRadians(angle));

    // view
    const XMVECTOR eyePosition = XMVectorSet(0, 0, -10, 1);
    const XMVECTOR focusPoint = XMVectorSet(0, 0, 0, 1);
    const XMVECTOR upDirection = XMVectorSet(0, 1, 0, 0);
    m_ViewMatrix = XMMatrixLookAtLH(eyePosition, focusPoint, upDirection);

    // projection
    m_ProjectionMatrix = XMMatrixPerspectiveFovLH(XMConvertToRadians(m_fieldOfViewAngle), m_aspectRatio, 0.1f, 100.0f);

    // update MVP matrix
    XMMATRIX mvpMatrix = XMMatrixMultiply(m_ModelMatrix, m_ViewMatrix);
    mvpMatrix = XMMatrixMultiply(mvpMatrix, m_ProjectionMatrix);
    m_constantBufferData.MVP = mvpMatrix;

    memcpy(m_pCbvDataBegin, &m_constantBufferData, sizeof(m_constantBufferData));
}

void ShaderToyScene::OnRender()
{
    // Inform ImGui backends and core that we are starting a new frame, and construct the immediate-mode UI. The UI
    //  will be drawn later via insertions into command list.
    ImGui_ImplDX12_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    m_pEngine->NewFrame();
    CbvData frameConstants = {&m_constantBufferData, sizeof(m_constantBufferData)};
    m_pipelineState.SetConstantBufferData(frameConstants);
    m_pipelineState.Render();
    m_pipelineState.Execute();
    m_pEngine->CopyResource(m_viewport.GetResource(), m_pipelineState.GetRenderTarget());

    // Build the UI after primary pipeline is executed. Keep things simple for now with only one frame in flight.
    BuildUI();

    // finalize operations and present swapchain
    m_pEngine->OnRender();
}
