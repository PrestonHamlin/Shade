#include "Shade.h"
#include "Dx12RenderEngine.h"
#include "ShaderToyScene.h"


extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);


_Use_decl_annotations_
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow)
{
    Dx12RenderEngine engine(800, 800);
    ShaderToyScene scene(L"Simple Scene");

    // initialize window class
    WNDCLASSEX windowClass = { 0 };
    windowClass.cbSize = sizeof(WNDCLASSEX);
    windowClass.style = CS_HREDRAW | CS_VREDRAW;
    windowClass.lpfnWndProc = WindowProc;
    windowClass.hInstance = hInstance;
    windowClass.hCursor = LoadCursor(NULL, IDC_ARROW);
    windowClass.lpszClassName = L"DXSampleClass";
    RegisterClassEx(&windowClass);

    // set initial window position
    RECT windowRect = {0, 0, 800, 800};
    AdjustWindowRect(&windowRect, WS_OVERLAPPEDWINDOW, FALSE);

    // create primary window
    HWND window = CreateWindow(
        windowClass.lpszClassName,
        L"default window text",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        windowRect.right - windowRect.left,
        windowRect.bottom - windowRect.top,
        nullptr,    // no parent window
        nullptr,    // no menus
        hInstance,
        &engine);

    // remove borders and title bar
    SetWindowLong(window, GWL_STYLE, 0);
    SetWindowPos(window, nullptr, 50, 50, 800, 800, 0);

    // initialize engine and scene
    engine.SetScene(&scene);
    engine.Init(window);
    scene.Init(&engine);
    ShowWindow(window, nCmdShow);

    // process message until time to quit
    MSG msg = {};
    while (msg.message != WM_QUIT)
    {
        if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    // clean up and exit
    engine.OnDestroy();
    return static_cast<char>(msg.wParam);
}


LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    RenderEngine* pEngine = reinterpret_cast<RenderEngine*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));

    // let ImGui have first pick of events
    if (ImGui_ImplWin32_WndProcHandler(hWnd, message, wParam, lParam))
    {
        return true;
    }

    switch (message)
    {
    case WM_CREATE:
        {
            LPCREATESTRUCT pCreateStruct = reinterpret_cast<LPCREATESTRUCT>(lParam);
            SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pCreateStruct->lpCreateParams));
        }
        return 0;

    case WM_PAINT:
        if (pEngine)
        {
            pEngine->OnUpdate();
            pEngine->OnRender();
        }
        return 0;

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }

    // use default handler for any unhandled messages
    return DefWindowProc(hWnd, message, wParam, lParam);
}
