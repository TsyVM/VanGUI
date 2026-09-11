// dear vangui: Renderer Backend for DirectX12
// This needs to be used along with a Platform Backend (e.g. Win32)

// Implemented features:
//  [X] Renderer: User texture binding. Use 'D3D12_GPU_DESCRIPTOR_HANDLE' as VanTextureID. Read the FAQ about VanTextureID!
//  [X] Renderer: Large meshes support (64k+ vertices) with 16-bit indices.
//  [X] Renderer: Texture updates (VanTextureStatus_WantCreate / WantUpdates / WantDestroy).

// You can use unmodified vangui_impl_* files in your project.
// Prefer including the entire vangui/ repository into your project (either as a copy or as a submodule),
// and only build the backends you need.

#pragma once
#include <vangui/vangui.h>      // VANGUI_IMPL_API

// Avoid including <d3d12.h> directly in this header — forward-declare what we need.
struct ID3D12Device;
struct ID3D12DescriptorHeap;
struct ID3D12GraphicsCommandList;
struct D3D12_CPU_DESCRIPTOR_HANDLE;
struct D3D12_GPU_DESCRIPTOR_HANDLE;

// DXGI_FORMAT is an enum, and this header used to spell it `typedef int
// DXGI_FORMAT;` to avoid an include. That is not a forward declaration --
// there is no such thing for an enum -- it is a second, different definition,
// so any translation unit that included this header and <d3d12.h> failed with
// "C2371: redefinition; different basic types". Which is every use of this
// backend, including its own .cpp; it went unnoticed because the backend is
// off by default.
//
// <dxgiformat.h> is the enum and nothing else: no COM, no d3d12.h, a few
// hundred lines of enumerators. Including it costs nothing and is what the
// upstream Dear ImGui backend does.
#include <dxgiformat.h>

// ---------------------------------------------------------------------------
//  Initialisation
// ---------------------------------------------------------------------------
//
// There are two forms. Prefer the InitInfo one; the older one is kept so that
// existing callers keep building, and is implemented in terms of it.
//
// THE PROBLEM THE INITINFO FORM SOLVES
//
// D3D12 has no texture handles, only descriptors, and a descriptor lives in a
// heap the application owns. The backend therefore cannot create a texture on
// its own -- it has no way to obtain a slot. The older Init works around this
// by being handed one pre-allocated slot, for the font, and writing every
// texture it is ever asked to create into that same slot. With one texture
// that is correct. With two it is not: the second silently overwrites the
// font's descriptor, and the symptom is that all text disappears.
//
// So a caller that wants its own textures supplies two callbacks and the
// backend allocates a descriptor per texture, the way a D3D12 renderer
// normally would. This mirrors what upstream Dear ImGui 1.92 does.
struct VanGui_ImplDX12_InitInfo
{
    ID3D12Device*           Device              = nullptr;
    int                     NumFramesInFlight   = 0;
    DXGI_FORMAT             RTVFormat           = (DXGI_FORMAT)0;

    // The heap every descriptor below is allocated from. Must be
    // SHADER_VISIBLE, must outlive the backend, and must be large enough for
    // the font plus every texture the application creates.
    ID3D12DescriptorHeap*   SrvDescriptorHeap   = nullptr;

    // Passed back to the two callbacks untouched, so an allocator can find its
    // own state without a global.
    void*                   UserData            = nullptr;

    // Allocate one shader-visible SRV slot. Both handles must refer to the same
    // slot of SrvDescriptorHeap. Leave null to keep the single-slot behaviour of
    // the older Init, in which case only the font texture is usable.
    void (*SrvDescriptorAllocFn)(VanGui_ImplDX12_InitInfo* info,
                                 D3D12_CPU_DESCRIPTOR_HANDLE* out_cpu_handle,
                                 D3D12_GPU_DESCRIPTOR_HANDLE* out_gpu_handle) = nullptr;

    // Return a slot obtained from SrvDescriptorAllocFn. Called when a texture is
    // destroyed and on shutdown. Leave null if the allocator never reclaims.
    void (*SrvDescriptorFreeFn)(VanGui_ImplDX12_InitInfo* info,
                                D3D12_CPU_DESCRIPTOR_HANDLE cpu_handle,
                                D3D12_GPU_DESCRIPTOR_HANDLE gpu_handle) = nullptr;
};

// The InitInfo is copied, so it need not outlive the call -- but UserData must
// stay valid for as long as the backend does, because the callbacks receive a
// pointer to the copy and read UserData out of it.
VANGUI_IMPL_API bool     VanGui_ImplDX12_Init(VanGui_ImplDX12_InitInfo* info);

// cmd_list is the command list that the backend will use during VanGui_ImplDX12_RenderDrawData().
// - Before calling VanGui_ImplDX12_RenderDrawData(), make sure to reset the command list.
// - num_frames_in_flight sets the number of simultaneously rendered frames (this determines how many
//   sets of vertex/index buffers are allocated, each sized for one frame's worth of geometry).
// - cbv_srv_heap is the descriptor heap from which the backend will allocate SRVs for textures.
// - font_srv_cpu_desc_handle and font_srv_gpu_desc_handle are CPU/GPU handles for the font texture SRV.
//   You need to allocate this descriptor from your cbv_srv_heap and pass the handles here.
//   VanGui_ImplDX12_Init() will write to it; do NOT write to it yourself after Init.
//
// This form supports EXACTLY ONE texture, the font. Every texture shares the
// one descriptor it was given. Use the InitInfo form above for anything else.
VANGUI_IMPL_API bool     VanGui_ImplDX12_Init(ID3D12Device* device, int num_frames_in_flight, DXGI_FORMAT rtv_format, ID3D12DescriptorHeap* cbv_srv_heap, D3D12_CPU_DESCRIPTOR_HANDLE font_srv_cpu_desc_handle, D3D12_GPU_DESCRIPTOR_HANDLE font_srv_gpu_desc_handle);
VANGUI_IMPL_API void     VanGui_ImplDX12_Shutdown();
VANGUI_IMPL_API void     VanGui_ImplDX12_NewFrame();
VANGUI_IMPL_API void     VanGui_ImplDX12_RenderDrawData(VanDrawData* draw_data, ID3D12GraphicsCommandList* graphics_command_list);

// Use if you want to reset your rendering device without losing Dear VanGUI state.
VANGUI_IMPL_API void     VanGui_ImplDX12_InvalidateDeviceObjects();
VANGUI_IMPL_API bool     VanGui_ImplDX12_CreateDeviceObjects();
