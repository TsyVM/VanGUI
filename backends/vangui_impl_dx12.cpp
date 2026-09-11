// dear vangui: Renderer Backend for DirectX12

#include "vangui.h"
#ifndef VANGUI_DISABLE
#include "vangui_impl_dx12.h"

// DirectX
#include <stdio.h>
#include <d3d12.h>
#include <dxgi1_4.h>
#include <d3dcompiler.h>
#ifdef _MSC_VER
#pragma comment(lib, "d3dcompiler") // Automatically link with d3dcompiler.lib
#endif

// VanGUI progress (this is a DX12 VanGUI backend)
struct VanGui_ImplDX12_RenderBuffers;
struct VanGui_ImplDX12_Data;

// Per-frame resources (one set per frame-in-flight)
struct VanGui_ImplDX12_RenderBuffers
{
    ID3D12Resource*     IndexBuffer         = nullptr;
    ID3D12Resource*     VertexBuffer        = nullptr;
    int                 IndexBufferSize     = 0;
    int                 VertexBufferSize    = 0;
};

// Deferred-release entry for upload buffers used during texture creation
struct VanGui_ImplDX12_Texture
{
    ID3D12Resource*     pTexture            = nullptr;    // GPU texture (DEFAULT heap)
    ID3D12Resource*     pUploadBuffer       = nullptr;    // temporary UPLOAD heap buffer
    int                 UploadFrameIndex    = -1;         // frame when upload was submitted

    // The SRV slot this texture's view lives in. Owned when it came from
    // SrvDescriptorAllocFn, and handed back on destroy; borrowed (and not
    // freed) when the single-slot legacy path supplied the font's handles.
    D3D12_CPU_DESCRIPTOR_HANDLE hCpuDescHandle = {};
    D3D12_GPU_DESCRIPTOR_HANDLE hGpuDescHandle = {};
    bool                OwnsDescriptor      = false;
};

// One thing waiting for the GPU to be done with it. `resource` is released and,
// when `ownsDescriptor` is set, the SRV slot is handed back to the allocator --
// both only after a full lap of the frame ring, by which time the command list
// that referenced them has retired.
struct VanGui_ImplDX12_Retired
{
    ID3D12Resource*             resource        = nullptr;
    D3D12_CPU_DESCRIPTOR_HANDLE hCpuDescHandle  = {};
    D3D12_GPU_DESCRIPTOR_HANDLE hGpuDescHandle  = {};
    bool                        ownsDescriptor  = false;
};

// Main backend data
struct VanGui_ImplDX12_Data
{
    ID3D12Device*                   pd3dDevice              = nullptr;
    ID3D12RootSignature*            pRootSignature          = nullptr;
    ID3D12PipelineState*            pPipelineState          = nullptr;
    DXGI_FORMAT                     RTVFormat               = (DXGI_FORMAT)0;
    ID3D12DescriptorHeap*           pd3dSrvDescHeap         = nullptr;
    D3D12_CPU_DESCRIPTOR_HANDLE     hFontSrvCpuDescHandle   = {};
    D3D12_GPU_DESCRIPTOR_HANDLE     hFontSrvGpuDescHandle   = {};

    // A copy, so the caller's struct need not outlive Init. The callbacks are
    // handed a pointer to this copy, which is why UserData must still be the
    // caller's own pointer and must outlive the backend.
    VanGui_ImplDX12_InitInfo        InitInfo                = {};
    int                             numFramesInFlight       = 0;
    int                             frameIndex              = INT_MAX; // starts before first frame
    VanGui_ImplDX12_RenderBuffers*  pFrameResources         = nullptr;

    // Things that cannot be released until the GPU has finished with them: the
    // staging buffers a texture upload copies from, and textures themselves
    // once VanGUI has asked for them to go.
    //
    // A LIST per frame slot, not one pointer. It used to be one, on the theory
    // that a frame uploads at most one texture -- true for as long as the only
    // texture in the process was a font atlas built once. It stopped being true
    // the moment anything else existed: a second upload in the same frame
    // released the first one's staging buffer while the open command list was
    // still going to read it, and a GPU reading freed upload memory is a
    // device-removed crash on some drivers and nothing at all on others, which
    // is exactly how it survived testing.
    //
    // Textures are deferred the same way, with their descriptor, because an
    // in-flight draw still names both.
    VanVector<VanGui_ImplDX12_Retired>* pRetired = nullptr; // array[numFramesInFlight]
};

static VanGui_ImplDX12_Data* VanGui_ImplDX12_GetBackendData()
{
    return VanGui::GetCurrentContext() ? (VanGui_ImplDX12_Data*)VanGui::GetIO().BackendRendererUserData : nullptr;
}

// Hands back everything parked in one frame slot. Called when that slot comes
// round again -- a full lap later, so the command list that used any of it has
// long since retired -- and on shutdown, where the caller has already idled the
// device.
static void VanGui_ImplDX12_DrainRetired(VanGui_ImplDX12_Data* bd, int slot)
{
    if (!bd || !bd->pRetired) return;

    VanVector<VanGui_ImplDX12_Retired>& list = bd->pRetired[slot];
    for (VanGui_ImplDX12_Retired& r : list)
    {
        if (r.resource)
            r.resource->Release();

        // The descriptor goes back only now. Freeing it with the texture would
        // let the slot be reissued and overwritten while an in-flight draw was
        // still naming that GPU handle -- which shows the wrong texture at
        // best, and reads a descriptor for a released resource at worst.
        if (r.ownsDescriptor && bd->InitInfo.SrvDescriptorFreeFn)
            bd->InitInfo.SrvDescriptorFreeFn(&bd->InitInfo,
                                             r.hCpuDescHandle, r.hGpuDescHandle);
    }
    list.clear();
}

// Parks a resource until the GPU is done with it.
static void VanGui_ImplDX12_Retire(VanGui_ImplDX12_Data* bd, ID3D12Resource* res)
{
    if (!bd || !res || !bd->pRetired) { if (res) res->Release(); return; }

    const int slot = (bd->frameIndex == INT_MAX)
                   ? 0
                   : (bd->frameIndex % bd->numFramesInFlight);

    VanGui_ImplDX12_Retired r;
    r.resource = res;
    bd->pRetired[slot].push_back(r);
}

static const char* vertexShaderHlsl =
    "cbuffer vertexBuffer : register(b0) \
    {\
      float4x4 ProjectionMatrix; \
    };\
    struct VS_INPUT\
    {\
      float2 pos : POSITION;\
      float2 uv  : TEXCOORD0;\
      float4 col : COLOR0;\
    };\
    \
    struct PS_INPUT\
    {\
      float4 pos : SV_POSITION;\
      float4 col : COLOR0;\
      float2 uv  : TEXCOORD0;\
    };\
    \
    PS_INPUT main(VS_INPUT input)\
    {\
      PS_INPUT output;\
      output.pos = mul( ProjectionMatrix, float4(input.pos.xy, 0.f, 1.f));\
      output.col = input.col;\
      output.uv  = input.uv;\
      return output;\
    }";

static const char* pixelShaderHlsl =
    "struct PS_INPUT\
    {\
      float4 pos : SV_POSITION;\
      float4 col : COLOR0;\
      float2 uv  : TEXCOORD0;\
    };\
    SamplerState sampler0 : register(s0);\
    Texture2D texture0 : register(t0);\
    \
    float4 main(PS_INPUT input) : SV_Target\
    {\
      float4 out_col = input.col * texture0.Sample(sampler0, input.uv); \
      return out_col; \
    }";

bool VanGui_ImplDX12_CreateDeviceObjects()
{
    VanGui_ImplDX12_Data* bd = VanGui_ImplDX12_GetBackendData();
    if (!bd || !bd->pd3dDevice)
        return false;
    if (bd->pPipelineState)
        VanGui_ImplDX12_InvalidateDeviceObjects();

    // Compile vertex shader
    ID3DBlob* vs_blob = nullptr;
    ID3DBlob* error_blob = nullptr;
    if (FAILED(D3DCompile(vertexShaderHlsl, strlen(vertexShaderHlsl), nullptr, nullptr, nullptr,
        "main", "vs_5_0", 0, 0, &vs_blob, &error_blob)))
    {
        if (error_blob)
        {
            OutputDebugStringA((const char*)error_blob->GetBufferPointer());
            error_blob->Release();
        }
        return false;
    }
    if (error_blob) { error_blob->Release(); error_blob = nullptr; }

    // Compile pixel shader
    ID3DBlob* ps_blob = nullptr;
    if (FAILED(D3DCompile(pixelShaderHlsl, strlen(pixelShaderHlsl), nullptr, nullptr, nullptr,
        "main", "ps_5_0", 0, 0, &ps_blob, &error_blob)))
    {
        if (error_blob)
        {
            OutputDebugStringA((const char*)error_blob->GetBufferPointer());
            error_blob->Release();
        }
        vs_blob->Release();
        return false;
    }
    if (error_blob) { error_blob->Release(); error_blob = nullptr; }

    // Create root signature
    {
        D3D12_DESCRIPTOR_RANGE srvRange = {};
        srvRange.RangeType                          = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
        srvRange.NumDescriptors                     = 1;
        srvRange.BaseShaderRegister                 = 0;
        srvRange.RegisterSpace                      = 0;
        srvRange.OffsetInDescriptorsFromTableStart  = 0;

        D3D12_ROOT_PARAMETER rootParams[2] = {};

        // Param[0]: 16 root 32-bit constants (ProjectionMatrix) at b0, VERTEX visibility
        rootParams[0].ParameterType             = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
        rootParams[0].Constants.ShaderRegister  = 0;
        rootParams[0].Constants.RegisterSpace   = 0;
        rootParams[0].Constants.Num32BitValues  = 16;
        rootParams[0].ShaderVisibility          = D3D12_SHADER_VISIBILITY_VERTEX;

        // Param[1]: Descriptor table with 1 SRV range (t0), PIXEL visibility
        rootParams[1].ParameterType                       = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
        rootParams[1].DescriptorTable.NumDescriptorRanges = 1;
        rootParams[1].DescriptorTable.pDescriptorRanges   = &srvRange;
        rootParams[1].ShaderVisibility                    = D3D12_SHADER_VISIBILITY_PIXEL;

        // Static sampler: LINEAR filter, CLAMP addressing
        D3D12_STATIC_SAMPLER_DESC staticSampler = {};
        staticSampler.Filter           = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
        staticSampler.AddressU         = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        staticSampler.AddressV         = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        staticSampler.AddressW         = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        staticSampler.MipLODBias       = 0.0f;
        staticSampler.MaxAnisotropy    = 0;
        staticSampler.ComparisonFunc   = D3D12_COMPARISON_FUNC_NEVER;
        staticSampler.BorderColor      = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
        staticSampler.MinLOD           = 0.0f;
        staticSampler.MaxLOD           = D3D12_FLOAT32_MAX;
        staticSampler.ShaderRegister   = 0;
        staticSampler.RegisterSpace    = 0;
        staticSampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

        D3D12_ROOT_SIGNATURE_DESC rsDesc = {};
        rsDesc.NumParameters        = 2;
        rsDesc.pParameters          = rootParams;
        rsDesc.NumStaticSamplers    = 1;
        rsDesc.pStaticSamplers      = &staticSampler;
        rsDesc.Flags                =
            D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
            D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS       |
            D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS     |
            D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;

        ID3DBlob* rs_blob = nullptr;
        if (FAILED(D3D12SerializeRootSignature(&rsDesc, D3D_ROOT_SIGNATURE_VERSION_1, &rs_blob, &error_blob)))
        {
            if (error_blob)
            {
                OutputDebugStringA((const char*)error_blob->GetBufferPointer());
                error_blob->Release();
            }
            vs_blob->Release();
            ps_blob->Release();
            return false;
        }
        if (error_blob) { error_blob->Release(); error_blob = nullptr; }

        HRESULT hr = bd->pd3dDevice->CreateRootSignature(0,
            rs_blob->GetBufferPointer(), rs_blob->GetBufferSize(),
            IID_PPV_ARGS(&bd->pRootSignature));
        rs_blob->Release();
        if (FAILED(hr))
        {
            vs_blob->Release();
            ps_blob->Release();
            return false;
        }
    }

    // Create PSO
    {
        D3D12_INPUT_ELEMENT_DESC inputLayout[] =
        {
            { "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT,    0, (UINT)offsetof(VanDrawVert, pos), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0, (UINT)offsetof(VanDrawVert, uv),  D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "COLOR",    0, DXGI_FORMAT_R8G8B8A8_UNORM,  0, (UINT)offsetof(VanDrawVert, col), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        };

        D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
        psoDesc.pRootSignature          = bd->pRootSignature;
        psoDesc.VS                      = { vs_blob->GetBufferPointer(), vs_blob->GetBufferSize() };
        psoDesc.PS                      = { ps_blob->GetBufferPointer(), ps_blob->GetBufferSize() };
        psoDesc.InputLayout             = { inputLayout, 3 };
        psoDesc.PrimitiveTopologyType   = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        psoDesc.NumRenderTargets        = 1;
        psoDesc.RTVFormats[0]           = bd->RTVFormat;
        psoDesc.SampleDesc.Count        = 1;

        // Every sample enabled. This is not a default: psoDesc is zero
        // initialised, and a SampleMask of 0 masks out every sample. The
        // pipeline state still creates, the draws are still recorded, and not
        // one pixel is ever written -- so the UI builds its geometry, submits
        // it, and is invisible, with nothing anywhere reporting an error.
        psoDesc.SampleMask              = 0xFFFFFFFFu;

        // Rasterizer
        // Note: D3D12_RASTERIZER_DESC has no ScissorEnable — scissor testing in D3D12 is
        // always active when RSSetScissorRects has been called. No field to set here.
        psoDesc.RasterizerState.FillMode                = D3D12_FILL_MODE_SOLID;
        psoDesc.RasterizerState.CullMode                = D3D12_CULL_MODE_NONE;
        psoDesc.RasterizerState.FrontCounterClockwise   = TRUE;
        psoDesc.RasterizerState.DepthBias               = D3D12_DEFAULT_DEPTH_BIAS;
        psoDesc.RasterizerState.DepthBiasClamp          = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
        psoDesc.RasterizerState.SlopeScaledDepthBias    = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
        psoDesc.RasterizerState.DepthClipEnable         = TRUE;
        psoDesc.RasterizerState.MultisampleEnable       = FALSE;
        psoDesc.RasterizerState.AntialiasedLineEnable   = FALSE;
        psoDesc.RasterizerState.ForcedSampleCount       = 0;
        psoDesc.RasterizerState.ConservativeRaster      = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

        // Blend state for RenderTarget[0]
        D3D12_RENDER_TARGET_BLEND_DESC& rtBlend = psoDesc.BlendState.RenderTarget[0];
        rtBlend.BlendEnable             = TRUE;
        rtBlend.LogicOpEnable           = FALSE;
        rtBlend.SrcBlend                = D3D12_BLEND_SRC_ALPHA;
        rtBlend.DestBlend               = D3D12_BLEND_INV_SRC_ALPHA;
        rtBlend.BlendOp                 = D3D12_BLEND_OP_ADD;
        rtBlend.SrcBlendAlpha           = D3D12_BLEND_ONE;
        rtBlend.DestBlendAlpha          = D3D12_BLEND_INV_SRC_ALPHA;
        rtBlend.BlendOpAlpha            = D3D12_BLEND_OP_ADD;
        rtBlend.RenderTargetWriteMask   = D3D12_COLOR_WRITE_ENABLE_ALL;

        // Depth stencil
        psoDesc.DepthStencilState.DepthEnable       = FALSE;
        psoDesc.DepthStencilState.DepthWriteMask    = D3D12_DEPTH_WRITE_MASK_ALL;
        psoDesc.DepthStencilState.DepthFunc         = D3D12_COMPARISON_FUNC_ALWAYS;
        psoDesc.DepthStencilState.StencilEnable     = FALSE;

        HRESULT hr = bd->pd3dDevice->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&bd->pPipelineState));
        vs_blob->Release();
        ps_blob->Release();
        if (FAILED(hr))
            return false;
    }

    return true;
}

void VanGui_ImplDX12_InvalidateDeviceObjects()
{
    VanGui_ImplDX12_Data* bd = VanGui_ImplDX12_GetBackendData();
    if (!bd)
        return;

    if (bd->pRootSignature) { bd->pRootSignature->Release(); bd->pRootSignature = nullptr; }
    if (bd->pPipelineState) { bd->pPipelineState->Release(); bd->pPipelineState = nullptr; }

    // Release per-frame vertex/index buffers
    if (bd->pFrameResources)
    {
        for (int i = 0; i < bd->numFramesInFlight; i++)
        {
            VanGui_ImplDX12_RenderBuffers* fr = &bd->pFrameResources[i];
            if (fr->VertexBuffer) { fr->VertexBuffer->Release(); fr->VertexBuffer = nullptr; }
            if (fr->IndexBuffer)  { fr->IndexBuffer->Release();  fr->IndexBuffer  = nullptr; }
            fr->VertexBufferSize = 0;
            fr->IndexBufferSize  = 0;
        }
    }

    // Everything still waiting on the GPU. The caller has already made the
    // device idle by the time this runs, so there is nothing left to wait for.
    if (bd->pRetired)
        for (int i = 0; i < bd->numFramesInFlight; i++)
            VanGui_ImplDX12_DrainRetired(bd, i);
}

bool VanGui_ImplDX12_Init(VanGui_ImplDX12_InitInfo* info)
{
    VanGuiIO& io = VanGui::GetIO();
    VANGUI_CHECKVERSION();
    VAN_ASSERT(io.BackendRendererUserData == nullptr && "Already initialized a renderer backend!");
    VAN_ASSERT(info != nullptr);
    VAN_ASSERT(info->Device != nullptr);
    VAN_ASSERT(info->NumFramesInFlight >= 1);
    VAN_ASSERT(info->SrvDescriptorHeap != nullptr);

    // An allocator without a matching free would leak a descriptor per texture
    // for the life of the process. Catch it here rather than as a heap that
    // slowly fills up months later.
    VAN_ASSERT((info->SrvDescriptorAllocFn == nullptr) == (info->SrvDescriptorFreeFn == nullptr)
               && "supply both SrvDescriptorAllocFn and SrvDescriptorFreeFn, or neither");

    VanGui_ImplDX12_Data* bd = VAN_NEW(VanGui_ImplDX12_Data)();
    io.BackendRendererUserData = bd;
    io.BackendRendererName = "vangui_impl_dx12";
    io.BackendFlags |= VanGuiBackendFlags_RendererHasVtxOffset;
    io.BackendFlags |= VanGuiBackendFlags_RendererHasTextures;

    bd->InitInfo = *info;

    bd->pd3dDevice = info->Device;
    bd->RTVFormat = info->RTVFormat;
    bd->pd3dSrvDescHeap = info->SrvDescriptorHeap;
    bd->numFramesInFlight = info->NumFramesInFlight;
    bd->frameIndex = INT_MAX;

    bd->pFrameResources = new VanGui_ImplDX12_RenderBuffers[info->NumFramesInFlight];
    bd->pRetired        = new VanVector<VanGui_ImplDX12_Retired>[info->NumFramesInFlight];

    VanGuiPlatformIO& platform_io = VanGui::GetPlatformIO();
    platform_io.Renderer_TextureMaxWidth = platform_io.Renderer_TextureMaxHeight = D3D12_REQ_TEXTURE2D_U_OR_V_DIMENSION;

    return true;
}

bool VanGui_ImplDX12_Init(ID3D12Device* device, int num_frames_in_flight, DXGI_FORMAT rtv_format,
    ID3D12DescriptorHeap* cbv_srv_heap,
    D3D12_CPU_DESCRIPTOR_HANDLE font_srv_cpu_desc_handle,
    D3D12_GPU_DESCRIPTOR_HANDLE font_srv_gpu_desc_handle)
{
    // The single-slot path: no allocator, so every texture is given the one
    // descriptor supplied here. Correct for a caller that only ever shows the
    // font, which is what this signature was written for.
    VanGui_ImplDX12_InitInfo info = {};
    info.Device            = device;
    info.NumFramesInFlight = num_frames_in_flight;
    info.RTVFormat         = rtv_format;
    info.SrvDescriptorHeap = cbv_srv_heap;

    if (!VanGui_ImplDX12_Init(&info))
        return false;

    VanGui_ImplDX12_Data* bd = VanGui_ImplDX12_GetBackendData();
    bd->hFontSrvCpuDescHandle = font_srv_cpu_desc_handle;
    bd->hFontSrvGpuDescHandle = font_srv_gpu_desc_handle;
    return true;
}

void VanGui_ImplDX12_Shutdown()
{
    VanGui_ImplDX12_Data* bd = VanGui_ImplDX12_GetBackendData();
    VAN_ASSERT(bd != nullptr && "No renderer backend to shutdown, or already shutdown?");
    VanGuiIO& io = VanGui::GetIO();

    VanGui_ImplDX12_InvalidateDeviceObjects();

    delete[] bd->pFrameResources;
    bd->pFrameResources = nullptr;

    delete[] bd->pRetired;
    bd->pRetired = nullptr;

    io.BackendRendererName = nullptr;
    io.BackendRendererUserData = nullptr;
    io.BackendFlags &= ~(VanGuiBackendFlags_RendererHasVtxOffset | VanGuiBackendFlags_RendererHasTextures);

    VAN_DELETE(bd);
}

void VanGui_ImplDX12_NewFrame()
{
    VanGui_ImplDX12_Data* bd = VanGui_ImplDX12_GetBackendData();
    VAN_ASSERT(bd != nullptr && "Context or backend not initialized! Did you call VanGui_ImplDX12_Init()?");

    if (!bd->pPipelineState)
        VanGui_ImplDX12_CreateDeviceObjects();
}

static void VanGui_ImplDX12_CreateTexture(VanTextureData* tex, ID3D12GraphicsCommandList* cmd_list)
{
    VanGui_ImplDX12_Data* bd = VanGui_ImplDX12_GetBackendData();
    VAN_ASSERT(tex->Format == VanTextureFormat_RGBA32);
    VAN_ASSERT(tex->BackendUserData == nullptr);

    VanGui_ImplDX12_Texture* backend_tex = new VanGui_ImplDX12_Texture();

    // Create GPU texture (DEFAULT heap)
    D3D12_HEAP_PROPERTIES defaultHeap = {};
    defaultHeap.Type = D3D12_HEAP_TYPE_DEFAULT;

    D3D12_RESOURCE_DESC texDesc = {};
    texDesc.Dimension           = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    texDesc.Width               = (UINT)tex->Width;
    texDesc.Height              = (UINT)tex->Height;
    texDesc.DepthOrArraySize    = 1;
    texDesc.MipLevels           = 1;
    texDesc.Format              = DXGI_FORMAT_R8G8B8A8_UNORM;
    texDesc.SampleDesc.Count    = 1;
    texDesc.Layout              = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    texDesc.Flags               = D3D12_RESOURCE_FLAG_NONE;

    HRESULT hr = bd->pd3dDevice->CreateCommittedResource(
        &defaultHeap, D3D12_HEAP_FLAG_NONE, &texDesc,
        D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&backend_tex->pTexture));
    if (!SUCCEEDED(hr)) { delete backend_tex; return; }

    // Create upload buffer
    UINT64 uploadSize = 0;
    D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint = {};
    UINT numRows = 0;
    UINT64 rowSizeInBytes = 0;
    bd->pd3dDevice->GetCopyableFootprints(&texDesc, 0, 1, 0, &footprint, &numRows, &rowSizeInBytes, &uploadSize);

    D3D12_HEAP_PROPERTIES uploadHeap = {};
    uploadHeap.Type = D3D12_HEAP_TYPE_UPLOAD;

    D3D12_RESOURCE_DESC uploadDesc = {};
    uploadDesc.Dimension            = D3D12_RESOURCE_DIMENSION_BUFFER;
    uploadDesc.Width                = uploadSize;
    uploadDesc.Height               = 1;
    uploadDesc.DepthOrArraySize     = 1;
    uploadDesc.MipLevels            = 1;
    uploadDesc.SampleDesc.Count     = 1;
    uploadDesc.Layout               = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

    hr = bd->pd3dDevice->CreateCommittedResource(
        &uploadHeap, D3D12_HEAP_FLAG_NONE, &uploadDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&backend_tex->pUploadBuffer));
    if (!SUCCEEDED(hr)) { backend_tex->pTexture->Release(); delete backend_tex; return; }

    // Copy pixel data into upload buffer
    void* mapped = nullptr;
    D3D12_RANGE readRange = { 0, 0 };
    backend_tex->pUploadBuffer->Map(0, &readRange, &mapped);
    const unsigned char* src = (const unsigned char*)tex->GetPixels();
    unsigned char* dst = (unsigned char*)mapped + footprint.Offset;
    for (UINT y = 0; y < (UINT)tex->Height; y++)
        memcpy(dst + y * footprint.Footprint.RowPitch,
               src + y * tex->Width * 4,
               tex->Width * 4);
    backend_tex->pUploadBuffer->Unmap(0, nullptr);

    // Issue copy command
    D3D12_TEXTURE_COPY_LOCATION srcLoc = {};
    srcLoc.pResource        = backend_tex->pUploadBuffer;
    srcLoc.Type             = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
    srcLoc.PlacedFootprint  = footprint;

    D3D12_TEXTURE_COPY_LOCATION dstLoc = {};
    dstLoc.pResource        = backend_tex->pTexture;
    dstLoc.Type             = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
    dstLoc.SubresourceIndex = 0;

    cmd_list->CopyTextureRegion(&dstLoc, 0, 0, 0, &srcLoc, nullptr);

    // Transition to SHADER_RESOURCE
    D3D12_RESOURCE_BARRIER barrier = {};
    barrier.Type                        = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition.pResource        = backend_tex->pTexture;
    barrier.Transition.Subresource      = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    barrier.Transition.StateBefore      = D3D12_RESOURCE_STATE_COPY_DEST;
    barrier.Transition.StateAfter       = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
    cmd_list->ResourceBarrier(1, &barrier);

    // A descriptor of this texture's own when the caller gave us an allocator,
    // and the one shared font slot when it did not. The GPU handle becomes the
    // texture's id, which is what VanDrawCmd carries and what SetGraphicsRoot-
    // DescriptorTable is given at draw time.
    if (bd->InitInfo.SrvDescriptorAllocFn)
    {
        bd->InitInfo.SrvDescriptorAllocFn(&bd->InitInfo,
                                          &backend_tex->hCpuDescHandle,
                                          &backend_tex->hGpuDescHandle);
        backend_tex->OwnsDescriptor = true;
    }
    else
    {
        backend_tex->hCpuDescHandle = bd->hFontSrvCpuDescHandle;
        backend_tex->hGpuDescHandle = bd->hFontSrvGpuDescHandle;
        backend_tex->OwnsDescriptor = false;
    }

    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format                      = DXGI_FORMAT_R8G8B8A8_UNORM;
    srvDesc.ViewDimension               = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels         = 1;
    srvDesc.Shader4ComponentMapping     = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    bd->pd3dDevice->CreateShaderResourceView(backend_tex->pTexture, &srvDesc,
                                             backend_tex->hCpuDescHandle);

    tex->SetTexID((VanTextureID)(intptr_t)backend_tex->hGpuDescHandle.ptr);
    tex->SetStatus(VanTextureStatus_OK);
    tex->BackendUserData = backend_tex;

    // The staging buffer is parked until this frame slot comes round again --
    // a full lap, by which time the copy has certainly happened.
    VanGui_ImplDX12_Retire(bd, backend_tex->pUploadBuffer);
    backend_tex->pUploadBuffer = nullptr;   // ownership transferred
}

// Re-uploads the part of a texture that changed, without recreating it.
//
// This backend advertised "[X] Renderer: Texture updates (WantCreate /
// WantUpdates / WantDestroy)" in its header and implemented two of the three.
// Nothing noticed while the only texture was a static bitmap font atlas built
// once at one size. VanGUI 1.92's font atlas is *dynamic*: it rasterises glyphs
// on demand and asks for an incremental upload through WantUpdates, and with
// that request dropped on the floor every glyph after the first batch stayed
// blank. The symptom is a UI with most of its text missing.
//
// The queued rectangles are covered by tex->UpdateRect, and one copy of that
// bounding box is both simpler and, for a glyph atlas, cheaper than a copy per
// rect -- the rects are typically adjacent cells of the same row.
static void VanGui_ImplDX12_UpdateTexture(VanTextureData* tex,
                                          ID3D12GraphicsCommandList* cmd_list)
{
    VanGui_ImplDX12_Data* bd = VanGui_ImplDX12_GetBackendData();
    VanGui_ImplDX12_Texture* backend_tex = (VanGui_ImplDX12_Texture*)tex->BackendUserData;
    if (!bd || !backend_tex || !backend_tex->pTexture) return;

    const int x = tex->UpdateRect.x;
    const int y = tex->UpdateRect.y;
    const int w = tex->UpdateRect.w;
    const int h = tex->UpdateRect.h;
    if (w <= 0 || h <= 0) { tex->SetStatus(VanTextureStatus_OK); return; }

    // A staging buffer sized to the dirty box, with rows padded to D3D12's
    // 256-byte alignment.
    const UINT bpp      = (UINT)tex->BytesPerPixel;
    const UINT rowBytes = (UINT)w * bpp;
    const UINT rowPitch = (rowBytes + D3D12_TEXTURE_DATA_PITCH_ALIGNMENT - 1)
                        & ~(UINT)(D3D12_TEXTURE_DATA_PITCH_ALIGNMENT - 1);
    const UINT64 uploadSize = (UINT64)rowPitch * (UINT64)h;

    D3D12_HEAP_PROPERTIES uploadHeap = {};
    uploadHeap.Type = D3D12_HEAP_TYPE_UPLOAD;

    D3D12_RESOURCE_DESC uploadDesc = {};
    uploadDesc.Dimension        = D3D12_RESOURCE_DIMENSION_BUFFER;
    uploadDesc.Width            = uploadSize;
    uploadDesc.Height           = 1;
    uploadDesc.DepthOrArraySize = 1;
    uploadDesc.MipLevels        = 1;
    uploadDesc.SampleDesc.Count = 1;
    uploadDesc.Layout           = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

    ID3D12Resource* upload = nullptr;
    if (FAILED(bd->pd3dDevice->CreateCommittedResource(
            &uploadHeap, D3D12_HEAP_FLAG_NONE, &uploadDesc,
            D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&upload))))
        return;

    void* mapped = nullptr;
    D3D12_RANGE noRead = { 0, 0 };
    if (FAILED(upload->Map(0, &noRead, &mapped))) { upload->Release(); return; }

    for (int row = 0; row < h; ++row)
        memcpy((unsigned char*)mapped + (size_t)row * rowPitch,
               tex->GetPixelsAt(x, y + row), rowBytes);

    upload->Unmap(0, nullptr);

    D3D12_RESOURCE_BARRIER toCopy = {};
    toCopy.Type                   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    toCopy.Transition.pResource   = backend_tex->pTexture;
    toCopy.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    toCopy.Transition.StateBefore = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
    toCopy.Transition.StateAfter  = D3D12_RESOURCE_STATE_COPY_DEST;
    cmd_list->ResourceBarrier(1, &toCopy);

    D3D12_TEXTURE_COPY_LOCATION src = {};
    src.pResource                          = upload;
    src.Type                               = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
    src.PlacedFootprint.Offset             = 0;
    src.PlacedFootprint.Footprint.Format   = DXGI_FORMAT_R8G8B8A8_UNORM;
    src.PlacedFootprint.Footprint.Width    = (UINT)w;
    src.PlacedFootprint.Footprint.Height   = (UINT)h;
    src.PlacedFootprint.Footprint.Depth    = 1;
    src.PlacedFootprint.Footprint.RowPitch = rowPitch;

    D3D12_TEXTURE_COPY_LOCATION dst = {};
    dst.pResource        = backend_tex->pTexture;
    dst.Type             = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
    dst.SubresourceIndex = 0;

    cmd_list->CopyTextureRegion(&dst, (UINT)x, (UINT)y, 0, &src, nullptr);

    D3D12_RESOURCE_BARRIER toRead = toCopy;
    toRead.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
    toRead.Transition.StateAfter  = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
    cmd_list->ResourceBarrier(1, &toRead);

    // Same deferred release the creation path uses.
    VanGui_ImplDX12_Retire(bd, upload);

    tex->SetStatus(VanTextureStatus_OK);
}

static void VanGui_ImplDX12_DestroyTexture(VanTextureData* tex)
{
    VanGui_ImplDX12_Data* bd = VanGui_ImplDX12_GetBackendData();

    if (VanGui_ImplDX12_Texture* backend_tex = (VanGui_ImplDX12_Texture*)tex->BackendUserData)
    {
        // The texture and its descriptor go into the deferred ring rather than
        // being released here.
        //
        // VanGUI asks for a texture to be destroyed on the frame it stopped
        // using it, which is a frame the GPU has not finished -- and often has
        // not started. Releasing the resource there is a use-after-free the
        // moment the atlas grows, which a dynamic font atlas does whenever a
        // new glyph appears. Some drivers survive it; others remove the device.
        //
        // Only a descriptor this texture allocated is handed back. The legacy
        // path borrows the font's slot, and returning that to an allocator that
        // never issued it is how a heap ends up double-freeing one entry.
        if (bd && bd->pRetired)
        {
            const int slot = (bd->frameIndex == INT_MAX)
                           ? 0
                           : (bd->frameIndex % bd->numFramesInFlight);

            VanGui_ImplDX12_Retired r;
            r.resource       = backend_tex->pTexture;
            r.hCpuDescHandle = backend_tex->hCpuDescHandle;
            r.hGpuDescHandle = backend_tex->hGpuDescHandle;
            r.ownsDescriptor = backend_tex->OwnsDescriptor;
            bd->pRetired[slot].push_back(r);

            backend_tex->pTexture = nullptr;
        }
        else if (backend_tex->pTexture)
        {
            backend_tex->pTexture->Release();
        }

        if (backend_tex->pUploadBuffer) VanGui_ImplDX12_Retire(bd, backend_tex->pUploadBuffer);
        delete backend_tex;
        tex->SetTexID(VanTextureID_Invalid);
        tex->BackendUserData = nullptr;
    }
    tex->SetStatus(VanTextureStatus_Destroyed);
}

static void VanGui_ImplDX12_SetupRenderState(VanDrawData* draw_data, ID3D12GraphicsCommandList* cmd_list,
    VanGui_ImplDX12_RenderBuffers* fr, VanGui_ImplDX12_Data* bd)
{
    // Setup viewport
    D3D12_VIEWPORT vp = {};
    vp.Width    = draw_data->DisplaySize.x;
    vp.Height   = draw_data->DisplaySize.y;
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;
    vp.TopLeftX = 0.0f;
    vp.TopLeftY = 0.0f;
    cmd_list->RSSetViewports(1, &vp);

    // Setup orthographic projection matrix
    float L = draw_data->DisplayPos.x;
    float R = draw_data->DisplayPos.x + draw_data->DisplaySize.x;
    float T = draw_data->DisplayPos.y;
    float B = draw_data->DisplayPos.y + draw_data->DisplaySize.y;
    float mvp[4][4] =
    {
        { 2.0f/(R-L),    0.0f,         0.0f,  0.0f },
        { 0.0f,          2.0f/(T-B),   0.0f,  0.0f },
        { 0.0f,          0.0f,         0.5f,  0.0f },
        { (R+L)/(L-R),  (T+B)/(B-T),  0.5f,  1.0f },
    };

    // Setup vertex buffers
    UINT stride = sizeof(VanDrawVert);
    UINT offset = 0;
    D3D12_VERTEX_BUFFER_VIEW vbv = {};
    vbv.BufferLocation  = fr->VertexBuffer->GetGPUVirtualAddress();
    vbv.SizeInBytes     = (UINT)(fr->VertexBufferSize * sizeof(VanDrawVert));
    vbv.StrideInBytes   = stride;
    cmd_list->IASetVertexBuffers(0, 1, &vbv);

    D3D12_INDEX_BUFFER_VIEW ibv = {};
    ibv.BufferLocation  = fr->IndexBuffer->GetGPUVirtualAddress();
    ibv.SizeInBytes     = (UINT)(fr->IndexBufferSize * sizeof(VanDrawIdx));
    ibv.Format          = sizeof(VanDrawIdx) == 2 ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT;
    cmd_list->IASetIndexBuffer(&ibv);

    cmd_list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    cmd_list->SetPipelineState(bd->pPipelineState);
    cmd_list->SetGraphicsRootSignature(bd->pRootSignature);
    cmd_list->SetDescriptorHeaps(1, &bd->pd3dSrvDescHeap);
    cmd_list->SetGraphicsRoot32BitConstants(0, 16, &mvp[0][0], 0);
}

void VanGui_ImplDX12_RenderDrawData(VanDrawData* draw_data, ID3D12GraphicsCommandList* cmd_list)
{
    // Avoid rendering when minimized
    if (draw_data->DisplaySize.x <= 0.0f || draw_data->DisplaySize.y <= 0.0f)
        return;

    VanGui_ImplDX12_Data* bd = VanGui_ImplDX12_GetBackendData();

    // Advance frame index
    bd->frameIndex = (bd->frameIndex == INT_MAX) ? 0 : (bd->frameIndex + 1) % bd->numFramesInFlight;
    VanGui_ImplDX12_RenderBuffers* fr = &bd->pFrameResources[bd->frameIndex];

    // This slot has come round again, so everything parked in it a full lap
    // ago is finished with.
    VanGui_ImplDX12_DrainRetired(bd, bd->frameIndex);

    // Process pending textures
    if (draw_data->Textures != nullptr)
    {
        for (VanTextureData* tex : *draw_data->Textures)
        {
            if (tex->Status == VanTextureStatus_WantCreate)
                VanGui_ImplDX12_CreateTexture(tex, cmd_list);
            else if (tex->Status == VanTextureStatus_WantUpdates)
                VanGui_ImplDX12_UpdateTexture(tex, cmd_list);
            else if (tex->Status == VanTextureStatus_WantDestroy)
                VanGui_ImplDX12_DestroyTexture(tex);
        }
    }

    // Create and grow vertex buffer if needed
    if (!fr->VertexBuffer || fr->VertexBufferSize < draw_data->TotalVtxCount)
    {
        if (fr->VertexBuffer) { fr->VertexBuffer->Release(); fr->VertexBuffer = nullptr; }
        fr->VertexBufferSize = draw_data->TotalVtxCount + 5000;

        D3D12_HEAP_PROPERTIES uploadHeap = {};
        uploadHeap.Type = D3D12_HEAP_TYPE_UPLOAD;

        D3D12_RESOURCE_DESC vbDesc = {};
        vbDesc.Dimension        = D3D12_RESOURCE_DIMENSION_BUFFER;
        vbDesc.Width            = (UINT64)(fr->VertexBufferSize * sizeof(VanDrawVert));
        vbDesc.Height           = 1;
        vbDesc.DepthOrArraySize = 1;
        vbDesc.MipLevels        = 1;
        vbDesc.SampleDesc.Count = 1;
        vbDesc.Layout           = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

        if (FAILED(bd->pd3dDevice->CreateCommittedResource(
            &uploadHeap, D3D12_HEAP_FLAG_NONE, &vbDesc,
            D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&fr->VertexBuffer))))
            return;
    }

    // Create and grow index buffer if needed
    if (!fr->IndexBuffer || fr->IndexBufferSize < draw_data->TotalIdxCount)
    {
        if (fr->IndexBuffer) { fr->IndexBuffer->Release(); fr->IndexBuffer = nullptr; }
        fr->IndexBufferSize = draw_data->TotalIdxCount + 10000;

        D3D12_HEAP_PROPERTIES uploadHeap = {};
        uploadHeap.Type = D3D12_HEAP_TYPE_UPLOAD;

        D3D12_RESOURCE_DESC ibDesc = {};
        ibDesc.Dimension        = D3D12_RESOURCE_DIMENSION_BUFFER;
        ibDesc.Width            = (UINT64)(fr->IndexBufferSize * sizeof(VanDrawIdx));
        ibDesc.Height           = 1;
        ibDesc.DepthOrArraySize = 1;
        ibDesc.MipLevels        = 1;
        ibDesc.SampleDesc.Count = 1;
        ibDesc.Layout           = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

        if (FAILED(bd->pd3dDevice->CreateCommittedResource(
            &uploadHeap, D3D12_HEAP_FLAG_NONE, &ibDesc,
            D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&fr->IndexBuffer))))
            return;
    }

    // Map and copy vertex/index data
    {
        void* vtx_mapped = nullptr;
        void* idx_mapped = nullptr;
        D3D12_RANGE readRange = { 0, 0 };

        if (FAILED(fr->VertexBuffer->Map(0, &readRange, &vtx_mapped)))
            return;
        if (FAILED(fr->IndexBuffer->Map(0, &readRange, &idx_mapped)))
        {
            fr->VertexBuffer->Unmap(0, nullptr);
            return;
        }

        VanDrawVert* vtx_dst = (VanDrawVert*)vtx_mapped;
        VanDrawIdx*  idx_dst = (VanDrawIdx*)idx_mapped;
        for (const VanDrawList* draw_list : draw_data->CmdLists)
        {
            memcpy(vtx_dst, draw_list->VtxBuffer.Data, draw_list->VtxBuffer.Size * sizeof(VanDrawVert));
            memcpy(idx_dst, draw_list->IdxBuffer.Data, draw_list->IdxBuffer.Size * sizeof(VanDrawIdx));
            vtx_dst += draw_list->VtxBuffer.Size;
            idx_dst += draw_list->IdxBuffer.Size;
        }

        D3D12_RANGE vtxWriteRange = { 0, (SIZE_T)(draw_data->TotalVtxCount * sizeof(VanDrawVert)) };
        D3D12_RANGE idxWriteRange = { 0, (SIZE_T)(draw_data->TotalIdxCount * sizeof(VanDrawIdx)) };
        fr->VertexBuffer->Unmap(0, &vtxWriteRange);
        fr->IndexBuffer->Unmap(0, &idxWriteRange);
    }

    // Setup render state
    VanGui_ImplDX12_SetupRenderState(draw_data, cmd_list, fr, bd);

    // Draw loop
    int global_vtx_offset = 0;
    int global_idx_offset = 0;
    VanVec2 clip_off = draw_data->DisplayPos;

    for (const VanDrawList* draw_list : draw_data->CmdLists)
    {
        for (int cmd_i = 0; cmd_i < draw_list->CmdBuffer.Size; cmd_i++)
        {
            const VanDrawCmd* pcmd = &draw_list->CmdBuffer[cmd_i];
            if (pcmd->UserCallback != nullptr)
            {
                if (pcmd->UserCallback == VanDrawCallback_ResetRenderState)
                    VanGui_ImplDX12_SetupRenderState(draw_data, cmd_list, fr, bd);
                else
                    pcmd->UserCallback(draw_list, pcmd);
            }
            else
            {
                // Apply scissor/clipping rectangle
                float clip_min_x = pcmd->ClipRect.x - clip_off.x;
                float clip_min_y = pcmd->ClipRect.y - clip_off.y;
                float clip_max_x = pcmd->ClipRect.z - clip_off.x;
                float clip_max_y = pcmd->ClipRect.w - clip_off.y;
                if (clip_max_x <= clip_min_x || clip_max_y <= clip_min_y)
                    continue;

                D3D12_RECT scissor = {
                    (LONG)clip_min_x, (LONG)clip_min_y,
                    (LONG)clip_max_x, (LONG)clip_max_y
                };
                cmd_list->RSSetScissorRects(1, &scissor);

                // Bind texture
                D3D12_GPU_DESCRIPTOR_HANDLE texture_handle = {};
                texture_handle.ptr = (UINT64)(intptr_t)pcmd->GetTexID();
                cmd_list->SetGraphicsRootDescriptorTable(1, texture_handle);

                // Draw
                cmd_list->DrawIndexedInstanced(
                    pcmd->ElemCount, 1,
                    pcmd->IdxOffset + global_idx_offset,
                    pcmd->VtxOffset + global_vtx_offset,
                    0);
            }
        }
        global_vtx_offset += draw_list->VtxBuffer.Size;
        global_idx_offset += draw_list->IdxBuffer.Size;
    }
}

#endif // #ifndef VANGUI_DISABLE
