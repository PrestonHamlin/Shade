#include "Shade.h"
#include "Dx12RenderEngine.h"
#include "ShaderToyScene.h"


extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);



_Use_decl_annotations_
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow)
{
    //D3D12HelloConstBuffers sample(1280, 720, L"D3D12 Hello Constant Buffers");
    Dx12RenderEngine engine(800, 800, L"D3D12 Tinkering");
    ShaderToyScene scene(800, 800, L"Scene Title");
    //Dx12RenderEngine engine(1000, 1000, L"D3D12 Tinkering");
    //return Win32Application::Run(&sample, hInstance, nCmdShow);
    //RenderEngine::pCurrentEngine = &engine;

    // Parse the command line parameters
    int argc;
    LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    engine.ParseCommandLineArgs(argv, argc);
    LocalFree(argv);

    // Initialize the window class.
    WNDCLASSEX windowClass = { 0 };
    windowClass.cbSize = sizeof(WNDCLASSEX);
    windowClass.style = CS_HREDRAW | CS_VREDRAW;
    windowClass.lpfnWndProc = WindowProc;
    //windowClass.lpfnWndProc = ImGui_ImplWin32_WndProcHandler;
    windowClass.hInstance = hInstance;
    windowClass.hCursor = LoadCursor(NULL, IDC_ARROW);
    windowClass.lpszClassName = L"DXSampleClass";
    RegisterClassEx(&windowClass);

    RECT windowRect = { 0, 0, static_cast<LONG>(engine.GetWidth()), static_cast<LONG>(engine.GetHeight()) };
    AdjustWindowRect(&windowRect, WS_OVERLAPPEDWINDOW, FALSE);

    // Create the window and store a handle to it.
    HWND window = CreateWindow(
        windowClass.lpszClassName,
        L"window text",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        windowRect.right - windowRect.left,
        windowRect.bottom - windowRect.top,
        nullptr,		// We have no parent window.
        nullptr,		// We aren't using menus.
        hInstance,
        //&engine);
        &scene);

    // remove borders and title bar
    SetWindowLong(window, GWL_STYLE, 0); // remove all window styles
    //ShowWindow(window, SW_SHOW);         // refresh window to apply (lack of) style
    SetWindowPos(window, nullptr, 50, 50, 800, 800, 0);

    // initialize engine and scene
    engine.Init(window);
    scene.Init(window, &engine);


    ShowWindow(window, nCmdShow);

    // Main sample loop.
    MSG msg = {};
    while (msg.message != WM_QUIT)
    {
        // Process any messages in the queue.
        if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    engine.OnDestroy();

    // Return this part of the WM_QUIT message to Windows.
    return static_cast<char>(msg.wParam);


}




LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    //RenderEngine* pEngine = reinterpret_cast<RenderEngine*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
    ShaderToyScene* pScene = reinterpret_cast<ShaderToyScene*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));

    // let ImGui have first pick of events
    if (ImGui_ImplWin32_WndProcHandler(hWnd, message, wParam, lParam))
    {
        return true;
    }

    switch (message)
    {
    case WM_CREATE:
        {
            // Save the DXSample* passed in to CreateWindow.
            LPCREATESTRUCT pCreateStruct = reinterpret_cast<LPCREATESTRUCT>(lParam);
            SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pCreateStruct->lpCreateParams));
        }
        return 0;

    //case WM_KEYDOWN:
    //    if (pScene)
    //    {
    //        pScene->OnKeyDown(static_cast<UINT8>(wParam));
    //    }
    //    return 0;

    //case WM_KEYUP:
    //    if (pScene)
    //    {
    //        pScene->OnKeyUp(static_cast<UINT8>(wParam));
    //    }
    //    return 0;

    case WM_PAINT:
        if (pScene)
        {
            pScene->OnUpdate();
            pScene->OnRender();
        }
        return 0;

    //case WM_SIZING:
    //    if (pScene)
    //    {
    //        RECT* newDimensions = reinterpret_cast<RECT*>(lParam);
    //        pScene->OnResize(newDimensions);
    //    }
    //    return 0;







    //case WM_KEYDOWN:
    //    if (pEngine)
    //    {
    //        pEngine->OnKeyDown(static_cast<UINT8>(wParam));
    //    }
    //    return 0;

    //case WM_KEYUP:
    //    if (pEngine)
    //    {
    //        pEngine->OnKeyUp(static_cast<UINT8>(wParam));
    //    }
    //    return 0;

    //case WM_PAINT:
    //    if (pEngine)
    //    {
    //        pEngine->OnUpdate();
    //        pEngine->OnRender();
    //    }
    //    return 0;

    //case WM_SIZING:
    //    if (pEngine)
    //    {
    //        RECT* newDimensions = reinterpret_cast<RECT*>(lParam);
    //        pEngine->OnResize(newDimensions);
    //    }
    //    return 0;

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }

    // use default handler for any unhandled messages
    return DefWindowProc(hWnd, message, wParam, lParam);
}
