// dear vangui: Null Platform+Renderer Backends
// This is designed if you need to use a blind VanGUI context with no input and no output.

// You can use unmodified vangui_impl_* files in your project. See examples/ folder for examples of using this.
// Prefer including the entire vangui/ repository into your project (either as a copy or as a submodule), and only build the backends you need.
// Learn about VanGUI:
// - FAQ                  https://dearvangui.com/faq
// - Getting Started      https://dearvangui.com/getting-started
// - Documentation        https://dearvangui.com/docs (same as your local docs/ folder).
// - Introduction, links and more at the top of vangui.cpp

// CHANGELOG
// (minor and older changes stripped away, please see git history for details)
//  2025-11-17: Initial version.

#include "vangui.h"
#ifndef VANGUI_DISABLE
#include "vangui_impl_null.h"

// Clang/GCC warnings with -Weverything
#if defined(__clang__)
#pragma clang diagnostic ignored "-Wold-style-cast"         // warning: use of old-style cast                            // yes, they are more terse.
#endif

VANGUI_IMPL_API bool VanGui_ImplNull_Init()
{
    VanGui_ImplNullPlatform_Init();
    VanGui_ImplNullRender_Init();
    return true;
}

VANGUI_IMPL_API void VanGui_ImplNull_Shutdown()
{
    VanGui_ImplNullRender_Shutdown();
    VanGui_ImplNullPlatform_Shutdown();
}

VANGUI_IMPL_API void VanGui_ImplNull_NewFrame()
{
    VanGui_ImplNullPlatform_NewFrame();
    VanGui_ImplNullRender_NewFrame();
}

VANGUI_IMPL_API bool VanGui_ImplNullPlatform_Init()
{
    VanGuiIO& io = VanGui::GetIO();
    io.BackendFlags |= VanGuiBackendFlags_HasMouseCursors;
    return true;
}

VANGUI_IMPL_API void VanGui_ImplNullPlatform_Shutdown()
{
    VanGuiIO& io = VanGui::GetIO();
    io.BackendFlags &= ~VanGuiBackendFlags_HasMouseCursors;
}

VANGUI_IMPL_API void VanGui_ImplNullPlatform_NewFrame()
{
    VanGuiIO& io = VanGui::GetIO();
    io.DisplaySize = VanVec2(1920, 1080);
    io.DeltaTime = 1.0f / 60.0f;
}

VANGUI_IMPL_API bool VanGui_ImplNullRender_Init()
{
    VanGuiIO& io = VanGui::GetIO();
    io.BackendFlags |= VanGuiBackendFlags_RendererHasVtxOffset;
    io.BackendFlags |= VanGuiBackendFlags_RendererHasTextures;
    return true;
}

VANGUI_IMPL_API void VanGui_ImplNullRender_Shutdown()
{
    VanGuiIO& io = VanGui::GetIO();
    io.BackendFlags &= ~VanGuiBackendFlags_RendererHasVtxOffset;
    io.BackendFlags &= ~VanGuiBackendFlags_RendererHasTextures;
}

VANGUI_IMPL_API void VanGui_ImplNullRender_NewFrame()
{
}

static void VanGui_ImplNullRender_UpdateTexture(VanTextureData* tex)
{
    if (tex->Status == VanTextureStatus_WantCreate || tex->Status == VanTextureStatus_WantDestroy)
        tex->SetStatus(VanTextureStatus_OK);
    if (tex->Status == VanTextureStatus_WantDestroy)
    {
        tex->SetTexID(VanTextureID_Invalid);
        tex->SetStatus(VanTextureStatus_Destroyed);
    }
}

VANGUI_IMPL_API void VanGui_ImplNullRender_RenderDrawData(VanDrawData* draw_data)
{
    if (draw_data->Textures != nullptr)
        for (VanTextureData* tex : *draw_data->Textures)
            if (tex->Status != VanTextureStatus_OK)
                VanGui_ImplNullRender_UpdateTexture(tex);
}

#endif // #ifndef VANGUI_DISABLE
