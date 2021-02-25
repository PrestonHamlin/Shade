#pragma once

#include "Scene.h"

// TODO: Cleanup and Additional Features
// Cleanup
//  Move functionality into parent Scene class
//  Prep for multi-dockspace
//
// Additional Features
//  Viewport class (texture/resource view, data view, render view, etc...)
//  Per-viewport controls (mesh visibility, camera, shaders/pipeline, etc...)
//  Add/remove meshes from scene
//  Mesh picker

class ShaderToyScene : public Scene
{

public:
    void DrawUI();
    void SetWindow(HWND window) {m_window = window;}

private:

    HWND m_window;

    bool m_showDebugConsole;
    bool m_showImGuiMetrics;


};

