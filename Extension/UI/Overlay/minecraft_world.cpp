#include "minecraft_world.h"
#include "Extension/Plugins/plugin_camera.h"
#include "Extension/SkateCraft/world_mesh.h"
#include "Engine/Core/Log/logging.h"
#include <Windows.h>
#include <d3dcompiler.h>
#include <wrl/client.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <map>
#include <memory>
#include <numbers>
#include <string>
#include <vector>

namespace dingosdk::minecraft_world {
namespace {
using Microsoft::WRL::ComPtr;
namespace mesh = skatecraft::world_mesh;
constexpr float near_plane = 0.05f, far_plane = 1000.0f;
constexpr UINT root_floats = 32;

// The blocks' shader. Positions are Minecraft's, turned into skate.'s by flipping X when the axes are
// mirrored (Extension/SkateCraft/coords.h: no other offset), then seen from the game's camera. The
// atlas holds sRGB codes, as Minecraft draws them; the overlay's output decides what is written.
constexpr char shader[] = R"hlsl(
cbuffer root : register(b0) {
    float4 camera;  // xyz the eye in skate.'s world, w the X axis' sign (-1 mirrored)
    float4 right;
    float4 up;
    float4 back;
    float4 lens;    // x, y: the projection's scale; z, w: depth = z * lens.z + lens.w over z
    float4 output;  // x the transfer (0 SDR, 1 scRGB, 2 HDR10), y white in nits
    float4 origin;  // xyz the section's corner (Minecraft), w the pass: 0 solid, 1 translucent, 2 outline
    float4 box_max; // the outline's far corner (its near one is origin)
};
Texture2D atlas : register(t0);
SamplerState nearest : register(s0);

struct vs_in {
    float3 position : POSITION;
    float2 uv : TEXCOORD0;
    float4 color : COLOR0;
    uint4 light : TEXCOORD1;
    uint flags : TEXCOORD2;
};
struct ps_in {
    float4 clip : SV_POSITION;
    float2 uv : TEXCOORD0;
    float4 color : COLOR0;
    float2 light : TEXCOORD1;
    float shade : TEXCOORD2;
};

float4 project(float3 mc) {
    float3 d = float3(mc.x * camera.w, mc.y, mc.z) - camera.xyz;
    float x = dot(d, right.xyz), y = dot(d, up.xyz), z = -dot(d, back.xyz);
    return float4(x * lens.x, y * lens.y, z * lens.z + lens.w, z);
}

// Minecraft's fixed face shading, which its meshes leave out: down, up, north/south, west/east.
float face_shade(uint flags) {
    uint face = (flags >> 4) & 7;
    if (face == 1) return 0.5;
    if (face == 3 || face == 4) return 0.8;
    if (face == 5 || face == 6) return 0.6;
    return 1.0;
}

ps_in vs_main(vs_in v) {
    ps_in o;
    o.clip = project(origin.xyz + v.position);
    o.uv = v.uv;
    o.color = v.color;
    o.light = float2(v.light.x, v.light.y) / 15.0;
    o.shade = face_shade(v.flags);
    return o;
}

// The twelve edges of the outlined block, two corners each: bit 0 X, bit 1 Z, bit 2 Y.
static const uint edge_corners[24] = {0, 1, 1, 3, 3, 2, 2, 0, 4, 5, 5, 7, 7, 6, 6, 4, 0, 4, 1, 5, 2, 6, 3, 7};
ps_in vs_outline(uint id : SV_VertexID) {
    uint c = edge_corners[id];
    float3 grow = float3(0.002, 0.002, 0.002);
    float3 lo = origin.xyz - grow, hi = box_max.xyz + grow;
    float3 p = float3((c & 1) ? hi.x : lo.x, (c & 4) ? hi.y : lo.y, (c & 2) ? hi.z : lo.z);
    ps_in o;
    o.clip = project(p);
    o.uv = float2(0, 0);
    o.color = float4(0, 0, 0, 0.45);
    o.light = float2(1, 1);
    o.shade = 1;
    return o;
}

float brightness(float level) { return level / (4.0 - 3.0 * level); } // Minecraft's light curve
float3 srgb_to_linear(float3 c) { return lerp(c / 12.92, pow(abs((c + 0.055) / 1.055), 2.4), step(0.04045, c)); }

// The overlay's output, as ImGui's own (imgui_impl_dx12.cpp): sRGB codes in SDR, else linear light.
float4 written(float3 code, float alpha) {
    if (output.x < 0.5) return float4(saturate(code), alpha);
    float3 colour = srgb_to_linear(saturate(code));
    if (output.x < 1.5) return float4(colour * (output.y / 80.0), alpha);
    float3 wide = float3(dot(colour, float3(0.6274, 0.3293, 0.0433)), dot(colour, float3(0.0691, 0.9195, 0.0114)), dot(colour, float3(0.0164, 0.0880, 0.8956)));
    float3 y = pow(saturate(wide * (output.y / 10000.0)), 0.1593017578125);
    return float4(pow((0.8359375 + 18.8515625 * y) / (1.0 + 18.6875 * y), 78.84375), alpha);
}

float4 ps_main(ps_in p) : SV_Target {
    float4 t = atlas.Sample(nearest, p.uv);
    bool translucent = origin.w > 0.5;
    if (!translucent && t.a < 0.5) discard; // leaves, glass panes, flowers: cut out
    float light = lerp(0.12, 1.0, max(brightness(p.light.y), brightness(p.light.x)));
    float3 code = t.rgb * p.color.rgb * p.shade * light;
    float alpha = translucent ? t.a * p.color.a : 1.0;
    if (alpha < 0.01) discard;
    return written(code, alpha);
}

float4 ps_outline(ps_in p) : SV_Target { return written(p.color.rgb, p.color.a); }
)hlsl";

struct Retired {
    ComPtr<ID3D12Resource> resource;
    std::uint64_t fence{};
};
struct GpuSection {
    std::shared_ptr<const mesh::Section> source;
    ComPtr<ID3D12Resource> solid, translucent;
    D3D12_VERTEX_BUFFER_VIEW solid_view{}, translucent_view{};
    UINT solid_count{}, translucent_count{};
};
struct Gpu {
    ComPtr<ID3D12RootSignature> root;
    ComPtr<ID3D12PipelineState> solid, translucent, outline;
    DXGI_FORMAT format{DXGI_FORMAT_UNKNOWN};
    ComPtr<ID3D12Resource> depth;
    ComPtr<ID3D12DescriptorHeap> depth_view;
    UINT64 depth_width{};
    UINT depth_height{};
    std::shared_ptr<const mesh::Atlas> atlas_source;
    ComPtr<ID3D12Resource> atlas;
    ID3D12DescriptorHeap* atlas_heap{};
    unsigned atlas_slot{~0u};
    std::shared_ptr<const std::vector<std::shared_ptr<const mesh::Section>>> list;
    std::map<const mesh::Section*, GpuSection> sections;
    std::vector<Retired> retired;
    bool failed{};
};
Gpu& gpu() {
    static auto* value = new Gpu;
    return *value;
}

void say_failed(const std::string& why) {
    auto& g = gpu();
    if (g.failed) return;
    g.failed = true;
    logging::write(logging::Level::warning, logging::Channel::graphics, "SkateCraft's blocks are not drawn: " + why);
}

void transition(ID3D12GraphicsCommandList* list, ID3D12Resource* resource, D3D12_RESOURCE_STATES from, D3D12_RESOURCE_STATES to) {
    D3D12_RESOURCE_BARRIER barrier{};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition.pResource = resource;
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    barrier.Transition.StateBefore = from;
    barrier.Transition.StateAfter = to;
    list->ResourceBarrier(1, &barrier);
}

void retire(ComPtr<ID3D12Resource>& resource, std::uint64_t fence) {
    if (resource) gpu().retired.push_back({std::move(resource), fence});
    resource.Reset();
}

ComPtr<ID3D12Resource> upload_buffer(ID3D12Device* device, std::size_t size, const void* data) {
    D3D12_HEAP_PROPERTIES heap{};
    heap.Type = D3D12_HEAP_TYPE_UPLOAD;
    D3D12_RESOURCE_DESC description{};
    description.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    description.Width = size;
    description.Height = 1;
    description.DepthOrArraySize = 1;
    description.MipLevels = 1;
    description.SampleDesc.Count = 1;
    description.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    ComPtr<ID3D12Resource> made;
    if (FAILED(device->CreateCommittedResource(&heap, D3D12_HEAP_FLAG_NONE, &description, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&made))))
        return {};
    if (data) {
        void* mapped{};
        const D3D12_RANGE none{};
        if (FAILED(made->Map(0, &none, &mapped)) || !mapped) return {};
        std::memcpy(mapped, data, size);
        made->Unmap(0, nullptr);
    }
    return made;
}

bool compile(const char* entry, const char* target, ComPtr<ID3DBlob>& out) {
    ComPtr<ID3DBlob> error;
    if (SUCCEEDED(D3DCompile(shader, std::strlen(shader), "minecraft_world", nullptr, nullptr, entry, target, D3DCOMPILE_OPTIMIZATION_LEVEL3, 0, &out,
                             &error)) &&
        out)
        return true;
    say_failed(std::string("its shader ") + entry + " did not compile" +
               (error ? std::string(": ") + static_cast<const char*>(error->GetBufferPointer()) : std::string(".")));
    return false;
}

bool build_root(ID3D12Device* device) {
    auto& g = gpu();
    if (g.root) return true;
    D3D12_DESCRIPTOR_RANGE range{};
    range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    range.NumDescriptors = 1;
    std::array<D3D12_ROOT_PARAMETER, 2> parameters{};
    parameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
    parameters[0].Constants.ShaderRegister = 0;
    parameters[0].Constants.Num32BitValues = root_floats;
    parameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
    parameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    parameters[1].DescriptorTable.NumDescriptorRanges = 1;
    parameters[1].DescriptorTable.pDescriptorRanges = &range;
    parameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
    D3D12_STATIC_SAMPLER_DESC sampler{};
    sampler.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT; // Minecraft's pixels stay pixels
    sampler.AddressU = sampler.AddressV = sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    sampler.ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
    sampler.MaxLOD = D3D12_FLOAT32_MAX;
    sampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
    D3D12_ROOT_SIGNATURE_DESC description{};
    description.NumParameters = static_cast<UINT>(parameters.size());
    description.pParameters = parameters.data();
    description.NumStaticSamplers = 1;
    description.pStaticSamplers = &sampler;
    description.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
    ComPtr<ID3DBlob> blob, error;
    if (FAILED(D3D12SerializeRootSignature(&description, D3D_ROOT_SIGNATURE_VERSION_1, &blob, &error)) || !blob ||
        FAILED(device->CreateRootSignature(0, blob->GetBufferPointer(), blob->GetBufferSize(), IID_PPV_ARGS(&g.root)))) {
        say_failed("its root signature could not be made.");
        return false;
    }
    return true;
}

bool build_pipelines(ID3D12Device* device, DXGI_FORMAT format) {
    auto& g = gpu();
    if (g.solid && g.format == format) return true;
    g.solid.Reset();
    g.translucent.Reset();
    g.outline.Reset();
    if (!build_root(device)) return false;
    ComPtr<ID3DBlob> vs, ps, vs_line, ps_line;
    if (!compile("vs_main", "vs_5_0", vs) || !compile("ps_main", "ps_5_0", ps) || !compile("vs_outline", "vs_5_0", vs_line) ||
        !compile("ps_outline", "ps_5_0", ps_line))
        return false;
    const std::array<D3D12_INPUT_ELEMENT_DESC, 5> layout{{
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(mesh::Vertex, x), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, offsetof(mesh::Vertex, u), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        {"COLOR", 0, DXGI_FORMAT_R8G8B8A8_UNORM, 0, offsetof(mesh::Vertex, color), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        {"TEXCOORD", 1, DXGI_FORMAT_R8G8B8A8_UINT, 0, offsetof(mesh::Vertex, light), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        {"TEXCOORD", 2, DXGI_FORMAT_R32_UINT, 0, offsetof(mesh::Vertex, flags), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
    }};
    D3D12_GRAPHICS_PIPELINE_STATE_DESC d{};
    d.pRootSignature = g.root.Get();
    d.VS = {vs->GetBufferPointer(), vs->GetBufferSize()};
    d.PS = {ps->GetBufferPointer(), ps->GetBufferSize()};
    d.InputLayout = {layout.data(), static_cast<UINT>(layout.size())};
    d.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_RED | D3D12_COLOR_WRITE_ENABLE_GREEN | D3D12_COLOR_WRITE_ENABLE_BLUE;
    d.SampleMask = UINT_MAX;
    d.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
    // Both sides: mirroring the X axis turns every face round.
    d.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
    d.RasterizerState.DepthClipEnable = TRUE;
    d.DepthStencilState.DepthEnable = TRUE;
    d.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
    d.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
    d.DSVFormat = DXGI_FORMAT_D32_FLOAT;
    d.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    d.NumRenderTargets = 1;
    d.RTVFormats[0] = format;
    d.SampleDesc.Count = 1;
    if (FAILED(device->CreateGraphicsPipelineState(&d, IID_PPV_ARGS(&g.solid)))) {
        say_failed("its pipeline was refused for this swapchain format.");
        return false;
    }
    // Glass, water, ice: blended over what is behind them, and not hiding what comes after.
    auto& blend = d.BlendState.RenderTarget[0];
    blend.BlendEnable = TRUE;
    blend.SrcBlend = D3D12_BLEND_SRC_ALPHA;
    blend.DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
    blend.BlendOp = D3D12_BLEND_OP_ADD;
    blend.SrcBlendAlpha = D3D12_BLEND_ZERO;
    blend.DestBlendAlpha = D3D12_BLEND_ONE;
    blend.BlendOpAlpha = D3D12_BLEND_OP_ADD;
    d.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
    d.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
    if (FAILED(device->CreateGraphicsPipelineState(&d, IID_PPV_ARGS(&g.translucent)))) {
        say_failed("its translucent pipeline was refused.");
        return false;
    }
    d.VS = {vs_line->GetBufferPointer(), vs_line->GetBufferSize()};
    d.PS = {ps_line->GetBufferPointer(), ps_line->GetBufferSize()};
    d.InputLayout = {nullptr, 0};
    d.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
    if (FAILED(device->CreateGraphicsPipelineState(&d, IID_PPV_ARGS(&g.outline)))) {
        say_failed("its outline pipeline was refused.");
        return false;
    }
    g.format = format;
    return true;
}

bool ensure_depth(const Target& t, UINT64 width, UINT height) {
    auto& g = gpu();
    if (g.depth && g.depth_width == width && g.depth_height == height) return true;
    retire(g.depth, t.fence);
    if (!g.depth_view) {
        D3D12_DESCRIPTOR_HEAP_DESC heap{};
        heap.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
        heap.NumDescriptors = 1;
        if (FAILED(t.device->CreateDescriptorHeap(&heap, IID_PPV_ARGS(&g.depth_view)))) {
            say_failed("its depth view could not be made.");
            return false;
        }
    }
    D3D12_RESOURCE_DESC description{};
    description.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    description.Width = width;
    description.Height = height;
    description.DepthOrArraySize = 1;
    description.MipLevels = 1;
    description.Format = DXGI_FORMAT_D32_FLOAT;
    description.SampleDesc.Count = 1;
    description.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL | D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;
    D3D12_HEAP_PROPERTIES heap{};
    heap.Type = D3D12_HEAP_TYPE_DEFAULT;
    D3D12_CLEAR_VALUE clear{};
    clear.Format = DXGI_FORMAT_D32_FLOAT;
    clear.DepthStencil.Depth = 1.0f;
    if (FAILED(t.device->CreateCommittedResource(&heap, D3D12_HEAP_FLAG_NONE, &description, D3D12_RESOURCE_STATE_DEPTH_WRITE, &clear, IID_PPV_ARGS(&g.depth)))) {
        say_failed("its depth buffer could not be made.");
        return false;
    }
    t.device->CreateDepthStencilView(g.depth.Get(), nullptr, g.depth_view->GetCPUDescriptorHandleForHeapStart());
    g.depth_width = width;
    g.depth_height = height;
    return true;
}

// Minecraft's block atlas as a texture, again whenever Minecraft sends a new one (a resource pack).
bool sync_atlas(const Target& t, const std::shared_ptr<const mesh::Atlas>& atlas) {
    auto& g = gpu();
    if (!atlas) return false;
    if (g.atlas_source != atlas) {
        D3D12_RESOURCE_DESC texture{};
        texture.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        texture.Width = atlas->width;
        texture.Height = atlas->height;
        texture.DepthOrArraySize = 1;
        texture.MipLevels = 1;
        texture.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        texture.SampleDesc.Count = 1;
        D3D12_PLACED_SUBRESOURCE_FOOTPRINT layout{};
        UINT rows{};
        UINT64 row_bytes{}, total{};
        t.device->GetCopyableFootprints(&texture, 0, 1, 0, &layout, &rows, &row_bytes, &total);
        if (!total || rows != atlas->height || row_bytes < std::uint64_t{atlas->width} * 4) return false;
        auto upload = upload_buffer(t.device, static_cast<std::size_t>(total), nullptr);
        D3D12_HEAP_PROPERTIES heap{};
        heap.Type = D3D12_HEAP_TYPE_DEFAULT;
        ComPtr<ID3D12Resource> made;
        void* mapped{};
        const D3D12_RANGE none{};
        if (!upload ||
            FAILED(t.device->CreateCommittedResource(&heap, D3D12_HEAP_FLAG_NONE, &texture, D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&made))) ||
            FAILED(upload->Map(0, &none, &mapped)) || !mapped) {
            say_failed("the block atlas could not be put on the GPU.");
            return false;
        }
        for (UINT row = 0; row < atlas->height; ++row)
            std::memcpy(static_cast<std::uint8_t*>(mapped) + layout.Offset + std::size_t{row} * layout.Footprint.RowPitch,
                        atlas->rgba.data() + std::size_t{row} * atlas->width * 4, std::size_t{atlas->width} * 4);
        upload->Unmap(0, nullptr);
        D3D12_TEXTURE_COPY_LOCATION to{}, from{};
        to.pResource = made.Get();
        to.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
        from.pResource = upload.Get();
        from.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
        from.PlacedFootprint = layout;
        t.commands->CopyTextureRegion(&to, 0, 0, 0, &from, nullptr);
        transition(t.commands, made.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        retire(upload, t.fence);
        retire(g.atlas, t.fence);
        g.atlas = std::move(made);
        g.atlas_source = atlas;
        g.atlas_heap = nullptr; // its view is made below
    }
    if (g.atlas_heap != t.srvs || g.atlas_slot != t.slot) {
        const auto step = t.device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        auto cpu = t.srvs->GetCPUDescriptorHandleForHeapStart();
        cpu.ptr += static_cast<SIZE_T>(step) * t.slot;
        D3D12_SHADER_RESOURCE_VIEW_DESC view{};
        view.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        view.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        view.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        view.Texture2D.MipLevels = 1;
        t.device->CreateShaderResourceView(g.atlas.Get(), &view, cpu);
        g.atlas_heap = t.srvs;
        g.atlas_slot = t.slot;
    }
    return true;
}

// Each section's mesh in its own buffer, read by the GPU straight from upload memory (the mirror
// world holds only what the player built: small). A changed section is a new one.
void sync_sections(const Target& t, const mesh::Scene& scene) {
    auto& g = gpu();
    if (g.list == scene.sections) return;
    g.list = scene.sections;
    std::map<const mesh::Section*, GpuSection> next;
    if (scene.sections) {
        for (const auto& section : *scene.sections) {
            if (!section) continue;
            if (auto found = g.sections.find(section.get()); found != g.sections.end()) {
                next.emplace(section.get(), std::move(found->second));
                g.sections.erase(found);
                continue;
            }
            GpuSection made;
            made.source = section;
            const auto put = [&](const std::vector<mesh::Vertex>& vertices, ComPtr<ID3D12Resource>& buffer, D3D12_VERTEX_BUFFER_VIEW& view, UINT& count) {
                if (vertices.empty()) return;
                const std::size_t bytes = vertices.size() * sizeof(mesh::Vertex);
                buffer = upload_buffer(t.device, bytes, vertices.data());
                if (!buffer) return;
                view = {buffer->GetGPUVirtualAddress(), static_cast<UINT>(bytes), sizeof(mesh::Vertex)};
                count = static_cast<UINT>(vertices.size());
            };
            put(section->solid, made.solid, made.solid_view, made.solid_count);
            put(section->translucent, made.translucent, made.translucent_view, made.translucent_count);
            next.emplace(section.get(), std::move(made));
        }
    }
    for (auto& [key, old] : g.sections) {
        retire(old.solid, t.fence);
        retire(old.translucent, t.fence);
    }
    g.sections = std::move(next);
}

void free_retired(const Target& t) {
    auto& g = gpu();
    if (g.retired.empty() || !t.gpu_done) return;
    const auto done = t.gpu_done();
    std::erase_if(g.retired, [&](const Retired& r) { return r.fence <= done; });
}
} // namespace

bool pending() noexcept {
    try {
        return !gpu().failed && mesh::shown();
    } catch (...) {
        return false;
    }
}

bool record(const Target& t) noexcept {
    try {
        auto& g = gpu();
        if (g.failed || !t.device || !t.commands || !t.buffer || !t.srvs) return false;
        free_retired(t);
        const auto scene = mesh::snapshot();
        if (!scene.shown) return false;
        sync_sections(t, scene);
        const bool any = !g.sections.empty() || scene.selection;
        if (!any || !sync_atlas(t, scene.atlas)) return false;
        const auto view = plugin_camera::camera_now();
        if (!view || !(view->vertical_fov > 1 && view->vertical_fov < 175)) return false;
        const auto desc = t.buffer->GetDesc();
        if (desc.Width < 16 || desc.Height < 16) return false;
        if (!build_pipelines(t.device, t.format) || !ensure_depth(t, desc.Width, desc.Height)) return false;

        const auto& m = view->world; // rows right, up, back with their scale, then the eye
        const float aspect = static_cast<float>(desc.Width) / static_cast<float>(desc.Height);
        const float focal = 1.0f / std::tan(view->vertical_fov * std::numbers::pi_v<float> / 360.0f);
        const float depth_scale = far_plane / (far_plane - near_plane);
        const auto unit = [](float x, float y, float z) {
            const float l = std::sqrt(x * x + y * y + z * z);
            return l > 1e-6f ? std::array<float, 3>{x / l, y / l, z / l} : std::array<float, 3>{x, y, z};
        };
        const auto r = unit(m[0], m[1], m[2]), u = unit(m[4], m[5], m[6]), b = unit(m[8], m[9], m[10]);
        std::array<float, root_floats> constants{
            m[12], m[13], m[14], scene.mirror_x ? -1.0f : 1.0f,
            r[0], r[1], r[2], 0,
            u[0], u[1], u[2], 0,
            b[0], b[1], b[2], 0,
            focal / aspect, focal, depth_scale, -near_plane * depth_scale,
            static_cast<float>(t.transfer), t.white_nits, 0, 0,
            0, 0, 0, 0,
            0, 0, 0, 0};

        transition(t.commands, t.buffer, D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
        const auto depth = g.depth_view->GetCPUDescriptorHandleForHeapStart();
        t.commands->OMSetRenderTargets(1, &t.rtv, FALSE, &depth);
        t.commands->ClearDepthStencilView(depth, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
        const D3D12_VIEWPORT viewport{0.0f, 0.0f, static_cast<float>(desc.Width), static_cast<float>(desc.Height), 0.0f, 1.0f};
        const D3D12_RECT scissor{0, 0, static_cast<LONG>(desc.Width), static_cast<LONG>(desc.Height)};
        t.commands->RSSetViewports(1, &viewport);
        t.commands->RSSetScissorRects(1, &scissor);
        t.commands->SetGraphicsRootSignature(g.root.Get());
        t.commands->SetGraphicsRoot32BitConstants(0, root_floats, constants.data(), 0);
        const auto step = t.device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        auto table = t.srvs->GetGPUDescriptorHandleForHeapStart();
        table.ptr += static_cast<UINT64>(step) * t.slot;
        t.commands->SetGraphicsRootDescriptorTable(1, table);
        t.commands->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        const auto section_origin = [&](const GpuSection& s, float pass) {
            const std::array<float, 4> origin{static_cast<float>(s.source->sx * 16), static_cast<float>(s.source->sy * 16),
                                              static_cast<float>(s.source->sz * 16), pass};
            t.commands->SetGraphicsRoot32BitConstants(0, 4, origin.data(), 24);
        };
        t.commands->SetPipelineState(g.solid.Get());
        for (const auto& [key, s] : g.sections) {
            if (!s.solid_count) continue;
            section_origin(s, 0);
            t.commands->IASetVertexBuffers(0, 1, &s.solid_view);
            t.commands->DrawInstanced(s.solid_count, 1, 0, 0);
        }
        t.commands->SetPipelineState(g.translucent.Get());
        for (const auto& [key, s] : g.sections) {
            if (!s.translucent_count) continue;
            section_origin(s, 1);
            t.commands->IASetVertexBuffers(0, 1, &s.translucent_view);
            t.commands->DrawInstanced(s.translucent_count, 1, 0, 0);
        }
        if (scene.selection) {
            const auto& box = *scene.selection;
            const std::array<float, 8> corners{box.min[0], box.min[1], box.min[2], 2, box.max[0], box.max[1], box.max[2], 0};
            t.commands->SetGraphicsRoot32BitConstants(0, 8, corners.data(), 24);
            t.commands->SetPipelineState(g.outline.Get());
            t.commands->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_LINELIST);
            t.commands->DrawInstanced(24, 1, 0, 0);
        }
        transition(t.commands, t.buffer, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
        return true;
    } catch (...) {
        say_failed("an exception was raised while drawing them.");
        return false;
    }
}

void release() noexcept {
    auto& g = gpu();
    // The overlay waits for the GPU before it tears down, so nothing here is still in use.
    g.retired.clear();
    g.solid.Reset();
    g.translucent.Reset();
    g.outline.Reset();
    g.format = DXGI_FORMAT_UNKNOWN;
    g.root.Reset();
    g.depth.Reset();
    g.depth_view.Reset();
    g.depth_width = 0;
    g.depth_height = 0;
    g.atlas.Reset();
    g.atlas_source.reset();
    g.atlas_heap = nullptr;
    g.atlas_slot = ~0u;
    g.list.reset();
    g.sections.clear();
}
}
