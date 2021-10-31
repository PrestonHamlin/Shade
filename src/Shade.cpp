#include "Shade.h"

#include <windowsx.h>
#include <dwmapi.h>

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
    windowClass.hbrBackground = CreateSolidBrush(BLACK_BRUSH); // fill client area black for debug
    windowClass.cbSize = sizeof(WNDCLASSEX);
    windowClass.style = CS_HREDRAW | CS_VREDRAW;
    windowClass.lpfnWndProc = WindowProc;
    windowClass.hInstance = hInstance;
    windowClass.hCursor = LoadCursor(NULL, IDC_ARROW);
    windowClass.lpszClassName = L"DXSampleClass";
    RegisterClassEx(&windowClass);

    // set initial window position
    RECT windowRect = {50, 50, 850, 850};
    //AdjustWindowRect(&windowRect, WS_THICKFRAME, FALSE);
    //AdjustWindowRect(&windowRect, WS_OVERLAPPEDWINDOW, FALSE);
    
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

    // initialize engine and scene
    engine.SetScene(&scene);
    engine.Init(window);
    scene.Init(&engine);

    // remove borders and title bar
    SetWindowLong(window, GWL_STYLE, 0); // no style
    //SetWindowLong(window, GWL_STYLE, WS_OVERLAPPEDWINDOW); // common layout
    //SetWindowLong(window, GWL_STYLE, WS_MINIMIZEBOX | WS_MAXIMIZEBOX); // can minimize and maximize with snap
    //SetWindowLong(window, GWL_STYLE, WS_BORDER);    // thin border

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
    Dx12RenderEngine* pEngine = reinterpret_cast<Dx12RenderEngine*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
    LRESULT result = 0;
    bool messageHandled = false;

    // let ImGui have first pick of events
    LRESULT imguiResult = ImGui_ImplWin32_WndProcHandler(hWnd, message, wParam, lParam);
    if (imguiResult != 0)
    {
        return imguiResult;
    }

    if (message == WM_CREATE)
    {
        PrintMessage("Creating window!\n");
        LPCREATESTRUCT pCreateStruct = reinterpret_cast<LPCREATESTRUCT>(lParam);
        SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pCreateStruct->lpCreateParams));
        messageHandled = true;
    }

    if (pEngine != nullptr)
    {
        switch (message)
        {
        case WM_ACTIVATE:
        {
            PrintMessage("Activating window!\n");

            messageHandled = true;
            break;
        }

        case WM_NCHITTEST:
        {
            // TODO: WM_NCPAINT?
            uint xPos = GET_X_LPARAM(lParam);
            uint yPos = GET_Y_LPARAM(lParam);
            uint hitResult = HTNOWHERE;
            if (pEngine->OnHitTest(xPos, yPos, &hitResult))
            {
                messageHandled = true;
                result = hitResult;
            }
            break;
        }

        case WM_PAINT:
        {
            pEngine->OnRender();
            messageHandled = true;
            break;
        }

        case WM_WINDOWPOSCHANGED: // precursor to WM_MOVE and WM_SIZE
        {
            const WINDOWPOS* pWindowPos = reinterpret_cast<WINDOWPOS*>(lParam);
            PrintMessage("\nWindow position changed: XY({},{}) - {}x{} - 0x{:X}\n",
                          pWindowPos->x, pWindowPos->y, pWindowPos->cx, pWindowPos->cy, pWindowPos->flags);
            pEngine->OnReposition(pWindowPos);
            messageHandled = true;
            break;
        }

        case WM_KEYDOWN:
        {
            //pEngine->OnKeyDown(wParam);
            break;
        }

        case WM_DESTROY:
        {
            PostQuitMessage(0);
            messageHandled = true;
            break;
        }
        }
    }

    // use default handler for any unhandled messages
    if (messageHandled)
    {
        return result;
    }
    else
    {
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
}
