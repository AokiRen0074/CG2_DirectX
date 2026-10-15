#include "EffectRenderer.h"
#include <algorithm>
#include <cassert>
#include <cmath>
#include <d3dcompiler.h>

namespace {
Vector3 Offset(Vector3 p, Vector3 n, float distance) {
    return {p.x + n.x * distance, p.y + n.y * distance, p.z + n.z * distance};
}

Vector4 Fade(Vector4 color, float alpha) {
    color.w *= alpha;
    return color;
}
} // namespace

void EffectRenderer::Initialize(DirectXCommon *dxCommon) {
    dxCommon_ = dxCommon;
    auto *device = dxCommon->GetDevice();
    D3D12_ROOT_PARAMETER parameter{};
    parameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
    parameter.Constants.Num32BitValues = 16;
    parameter.ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
    D3D12_ROOT_SIGNATURE_DESC signature{};
    signature.NumParameters = 1;
    signature.pParameters = &parameter;
    signature.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
    Microsoft::WRL::ComPtr<ID3DBlob> blob, errors;
    HRESULT hr =
        D3D12SerializeRootSignature(&signature, D3D_ROOT_SIGNATURE_VERSION_1_0, &blob, &errors);
    assert(SUCCEEDED(hr));
    hr = device->CreateRootSignature(0, blob->GetBufferPointer(), blob->GetBufferSize(),
                                     IID_PPV_ARGS(&rootSignature_));
    assert(SUCCEEDED(hr));
    auto vs = dxCommon->CompilerShader(L"Resources/Shaders/Effect.VS.hlsl", L"vs_6_0",
                                       dxCommon->GetDxcUtils(), dxCommon->GetDxcCompiler(),
                                       dxCommon->GetIncludeHandler());
    auto ps = dxCommon->CompilerShader(L"Resources/Shaders/Effect.PS.hlsl", L"ps_6_0",
                                       dxCommon->GetDxcUtils(), dxCommon->GetDxcCompiler(),
                                       dxCommon->GetIncludeHandler());
    D3D12_INPUT_ELEMENT_DESC layout[] = {{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,
                                          D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
                                         {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12,
                                          D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
                                         {"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 20,
                                          D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
                                         {"TEXCOORD", 1, DXGI_FORMAT_R32_FLOAT, 0, 36,
                                          D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}};
    D3D12_GRAPHICS_PIPELINE_STATE_DESC desc{};
    desc.pRootSignature = rootSignature_.Get();
    desc.VS = {vs->GetBufferPointer(), vs->GetBufferSize()};
    desc.PS = {ps->GetBufferPointer(), ps->GetBufferSize()};
    desc.InputLayout = {layout, _countof(layout)};
    desc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;
    desc.SampleDesc.Count = 1;
    desc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
    desc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
    desc.RasterizerState.DepthClipEnable = TRUE;
    auto &blend = desc.BlendState.RenderTarget[0];
    blend.BlendEnable = TRUE;
    blend.SrcBlend = blend.DestBlend = D3D12_BLEND_ONE;
    blend.BlendOp = blend.BlendOpAlpha = D3D12_BLEND_OP_ADD;
    blend.SrcBlendAlpha = D3D12_BLEND_ONE;
    blend.DestBlendAlpha = D3D12_BLEND_ZERO;
    blend.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
    desc.DepthStencilState.DepthEnable = TRUE;
    desc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
    desc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
    desc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
    desc.RTVFormats[0] = DXGI_FORMAT_R16G16B16A16_FLOAT;
    desc.NumRenderTargets = 1;
    desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    hr = device->CreateGraphicsPipelineState(&desc, IID_PPV_ARGS(&pipeline_));
    assert(SUCCEEDED(hr));

    D3D12_HEAP_PROPERTIES heap{};
    heap.Type = D3D12_HEAP_TYPE_UPLOAD;
    D3D12_RESOURCE_DESC buffer{};
    buffer.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    buffer.Width = capacity * sizeof(Vertex);
    buffer.Height = buffer.DepthOrArraySize = buffer.MipLevels = 1;
    buffer.SampleDesc.Count = 1;
    buffer.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    hr = device->CreateCommittedResource(&heap, D3D12_HEAP_FLAG_NONE, &buffer,
                                         D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
                                         IID_PPV_ARGS(&vertexBuffer_));
    assert(SUCCEEDED(hr));
    D3D12_RANGE noRead{0, 0};
    hr = vertexBuffer_->Map(0, &noRead, reinterpret_cast<void **>(&vertices_));
    assert(SUCCEEDED(hr));
    bufferView_ = {vertexBuffer_->GetGPUVirtualAddress(), UINT(buffer.Width), sizeof(Vertex)};
}

void EffectRenderer::Begin() {
    // エンジンは毎フレームGPU完了を待つため、同じアップロード領域を再利用できる。
    vertexCount_ = 0;
}

void EffectRenderer::AddQuad(const std::array<Vertex, 4> &quad) {
    if (vertexCount_ + 6 > capacity) {
        return;
    }

    for (int index : {0, 1, 2, 2, 1, 3}) {
        vertices_[vertexCount_++] = quad[index];
    }
}

void EffectRenderer::AddStrip(Vector3 a, Vector3 b, Vector3 normal, float widthA, float widthB,
                              Vector4 colorA, Vector4 colorB, float shape) {
    AddQuad({Vertex{Offset(a, normal, -widthA), 0, -1, colorA, shape},
             Vertex{Offset(a, normal, widthA), 0, 1, colorA, shape},
             Vertex{Offset(b, normal, -widthB), 1, -1, colorB, shape},
             Vertex{Offset(b, normal, widthB), 1, 1, colorB, shape}});
}

void EffectRenderer::AddGlow(Vector3 center, Vector3 axis, float length, float width,
                             Vector4 color) {
    const float magnitude = std::hypot(axis.x, axis.y);
    if (magnitude < 0.001f || length <= 0 || width <= 0 || color.w <= 0) {
        return;
    }

    axis = {axis.x / magnitude, axis.y / magnitude, 0};
    AddStrip(Offset(center, axis, -length), Offset(center, axis, length), {-axis.y, axis.x, 0},
             width, width, color, color, 0);
}

void EffectRenderer::AddRibbon(std::span<const Vector3> path, float width, Vector4 color) {
    if (path.size() < 2) {
        return;
    }

    for (size_t i = 1; i < path.size(); ++i) {
        const auto a = path[i - 1];
        const auto b = path[i];
        const float length = std::hypot(b.x - a.x, b.y - a.y);
        if (length < 0.001f) {
            continue;
        }

        const float t0 = float(i - 1) / float(path.size() - 1);
        const float t1 = float(i) / float(path.size() - 1);
        AddStrip(a, b, {-(b.y - a.y) / length, (b.x - a.x) / length, 0}, width * t0, width * t1,
                 Fade(color, t0), Fade(color, t1), 1);
    }
}

void EffectRenderer::AddArc(Vector3 center, float angle, float radius, float sweep, float width,
                            Vector4 color) {
    constexpr int segments = 28;
    for (int i = 0; i < segments; ++i) {
        const float t0 = float(i) / segments;
        const float t1 = float(i + 1) / segments;
        const float a0 = angle + (t0 - 0.5f) * sweep;
        const float a1 = angle + (t1 - 0.5f) * sweep;
        const Vector3 a = {center.x + std::cos(a0) * radius,
                           center.y + std::sin(a0) * radius * 0.7f, center.z};
        const Vector3 b = {center.x + std::cos(a1) * radius,
                           center.y + std::sin(a1) * radius * 0.7f, center.z};
        const float middle = (a0 + a1) * 0.5f;
        const float edge0 = std::sin(t0 * 3.141593f);
        const float edge1 = std::sin(t1 * 3.141593f);
        AddStrip(a, b, {std::cos(middle), std::sin(middle), 0}, width * edge0, width * edge1,
                 Fade(color, edge0), Fade(color, edge1), 1);
    }
}

void EffectRenderer::AddInterwovenRibbon(std::span<const Vector3> path, float fade, float phase) {
    if (path.size() < 2 || fade <= 0) {
        return;
    }

    // 点を増やして滑らかな曲線にする。赤と青は同じ中心線の両側を行き来する。
    constexpr size_t samples = 48;
    std::array<Vector3, samples> blue{}, red{};
    for (size_t i = 0; i < samples; ++i) {
        const float t = float(i) / float(samples - 1);
        const float location = t * float(path.size() - 1);
        const size_t segment = (std::min)(size_t(location), path.size() - 2);
        const float fraction = location - float(segment);
        const auto a = path[segment];
        const auto b = path[segment + 1];
        const float length = std::hypot(b.x - a.x, b.y - a.y);
        const float nx = length > 0.001f ? -(b.y - a.y) / length : 0;
        const float ny = length > 0.001f ? (b.x - a.x) / length : 1;
        const float wave = t * 9.424778f + phase;
        const float offset = std::sin(wave) * std::sin(t * 3.141593f) * 0.5f;
        const Vector3 center = {a.x + (b.x - a.x) * fraction, a.y + (b.y - a.y) * fraction, 0.75f};
        // 奥行きも交互に入れ替え、組み合わさる印象を作る。
        blue[i] = {center.x + nx * offset, center.y + ny * offset,
                   center.z + std::cos(wave) * 0.08f};
        red[i] = {center.x - nx * offset, center.y - ny * offset,
                  center.z - std::cos(wave) * 0.08f};
    }

    const float opacity = fade * fade;
    AddRibbon(blue, 0.22f * fade, {0.08f, 0.65f, 2.4f, opacity * 0.65f});
    AddRibbon(red, 0.19f * fade, {2.4f, 0.04f, 0.25f, opacity * 0.65f});
    AddRibbon(blue, 0.027f, {0.65f, 1.7f, 2.6f, opacity * 0.7f});
    AddRibbon(red, 0.023f, {2.3f, 0.45f, 0.7f, opacity * 0.6f});
}

void EffectRenderer::Draw(const ViewProjection &camera) {
    if (!HasGeometry()) {
        return;
    }

    auto *command = dxCommon_->GetCommandList();
    const auto viewProjection = Multiply(camera.matView, camera.matProjection);
    command->SetGraphicsRootSignature(rootSignature_.Get());
    command->SetPipelineState(pipeline_.Get());
    command->SetGraphicsRoot32BitConstants(0, 16, &viewProjection, 0);
    command->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    command->IASetVertexBuffers(0, 1, &bufferView_);
    command->DrawInstanced(UINT(vertexCount_), 1, 0, 0);
}

void EffectRenderer::AddCrystal(Vector3 center, float angle, float size, Vector4 color) {
    const Vector3 axis = {std::cos(angle), std::sin(angle), 0};
    const Vector3 normal = {-axis.y, axis.x, 0};
    Vector4 shadow = color;
    shadow.x *= 0.35f;
    shadow.y *= 0.35f;
    shadow.z *= 0.35f;
    // 二つの面で明暗を分け、発光していても破片の輪郭を残す。
    AddQuad({Vertex{Offset(center, normal, size * 0.45f), 0, 0, color, 2},
             Vertex{Offset(center, axis, -size), 0, 0, shadow, 2},
             Vertex{Offset(center, axis, size), 0, 0, color, 2},
             Vertex{Offset(center, normal, -size * 0.45f), 0, 0, shadow, 2}});
}

void EffectRenderer::AddBeam(Vector3 start, Vector3 end, float width, Vector4 color) {
    const float length = std::hypot(end.x - start.x, end.y - start.y);
    if (length < 0.001f || width <= 0 || color.w <= 0) {
        return;
    }

    AddStrip(start, end, {-(end.y - start.y) / length, (end.x - start.x) / length, 0}, width, width,
             color, color, 1);
}

void EffectRenderer::AddElectricBeam(Vector3 start, Vector3 end, int frame, float intensity) {
    const float length = std::hypot(end.x - start.x, end.y - start.y);
    if (length < 0.001f || intensity <= 0) {
        return;
    }

    const Vector3 along = {(end.x - start.x) / length, (end.y - start.y) / length, 0};
    const Vector3 normal = {-along.y, along.x, 0};
    const float clock = float(frame);
    const auto noise = [](int seed) {
        const float value = std::sin(float(seed) * 12.9898f) * 43758.5453f;
        return (value - std::floor(value)) * 2 - 1;
    };
    // 格子点の乱数を補間し、隣接点と次のフレームの形をつなげる。
    const auto smoothNoise = [&](float coordinate, int seed) {
        const int cell = int(std::floor(coordinate));
        const float fraction = coordinate - float(cell);
        const float t = fraction * fraction * (3 - 2 * fraction);
        const float a = noise(cell + seed);
        return a + (noise(cell + seed + 1) - a) * t;
    };
    const float pulse = intensity * (0.94f + std::sin(clock * 1.7f) * 0.06f);
    // 幅を持つ薄い外光と、強い白い芯を分けて発光させる。
    AddBeam(start, end, 1.05f, {2.2f, 0.025f, 0.09f, pulse * 0.12f});
    AddBeam(start, end, 0.55f, {3.0f, 0.035f, 0.08f, pulse * 0.28f});

    // 芯も均一な直線にせず、幅の異なる光の塊が発射方向へ流れる帯にする。
    // UVの横軸は実距離。分割数が変わっても模様の大きさと流速を保つ。
    const int segments = (std::clamp)(int(std::ceil(length / 0.24f)), 16, 384);
    const float shape = 3.0f + float(frame % 2048) / 2048.0f;
    const auto point = [&](float distance) {
        return Offset(Offset(start, along, distance), normal,
                      std::sin(distance * 4.3f - clock * 0.37f) * 0.025f);
    };
    const auto width = [&](float distance) {
        return 0.29f + smoothNoise(distance * 1.8f - clock * 0.12f, 83) * 0.035f +
               std::sin(distance * 6.1f - clock * 0.34f) * 0.015f;
    };
    const Vector4 color = {1, 1, 1, pulse};
    for (int i = 0; i < segments; ++i) {
        const float s0 = length * float(i) / float(segments);
        const float s1 = length * float(i + 1) / float(segments);
        const auto a = point(s0);
        const auto b = point(s1);
        AddQuad({Vertex{Offset(a, normal, -width(s0)), s0, -1, color, shape},
                 Vertex{Offset(a, normal, width(s0)), s0, 1, color, shape},
                 Vertex{Offset(b, normal, -width(s1)), s1, -1, color, shape},
                 Vertex{Offset(b, normal, width(s1)), s1, 1, color, shape}});
    }

    // 芯を安定させたまま、周囲の細い放電だけを連続変形させる。
    // 長く延ばしても稲妻が間延びしないよう、距離に合わせて点を増やす。
    // 外側は長いたわみを主形状にし、短い枝を等間隔に突き出さない。
    const int points = (std::clamp)(int(std::ceil(length / 0.15f)) + 1, 17, 641);
    for (int filament = 0; filament < 2; ++filament) {
        const float side = filament == 0 ? 1.0f : -1.0f;
        const int seed = filament * 137 + 31;
        const auto strandOffset = [&](float distance) {
            const float opening = (std::min)(1.0f, distance / 0.9f);
            const float lobe = (std::clamp)(
                0.35f + smoothNoise(distance * 0.45f - clock * 0.04f, seed + 17) * 0.65f +
                    smoothNoise(distance * 0.19f + clock * 0.025f, seed + 43) * 0.45f,
                0.0f, 1.0f);
            const float bow = 0.55f + smoothNoise(clock * 0.12f, seed + 67) * 0.18f;
            const float detail = smoothNoise(distance * 5.1f - clock * 0.48f, seed + 53) * 0.08f +
                                 smoothNoise(distance * 9.1f + clock * 0.3f, seed + 91) * 0.03f;
            return (side * (0.24f + lobe * bow) + detail) * opening;
        };
        const auto fork = [&](float distance) {
            const float value = smoothNoise(distance * 0.65f - clock * 0.035f, seed + 211);
            const float gate = (std::clamp)((value - 0.1f) / 0.55f, 0.0f, 1.0f);
            return gate * gate * (3 - 2 * gate);
        };
        Vector3 previous = start;
        float previousWidth = 0.018f;
        float previousFork = 0;
        for (int i = 1; i < points; ++i) {
            const float distance = length * float(i) / float(points - 1);
            const float offset = strandOffset(distance);
            const auto next = Offset(Offset(start, along, distance), normal, offset);
            const float width =
                0.023f + smoothNoise(distance * 2.1f - clock * 0.095f, seed + 71) * 0.007f;
            const float brightness =
                0.7f + smoothNoise(distance * 1.3f - clock * 0.08f, seed + 101) * 0.2f;
            // 細い暖白色の線のすぐ外だけを赤く光らせ、芯との間には空間を残す。
            AddStrip(previous, next, normal, previousWidth * 3, width * 3,
                     {2.6f, 0.015f, 0.035f, pulse * 0.18f}, {2.6f, 0.015f, 0.035f, pulse * 0.18f},
                     1);
            const Vector4 lineColor = {3.6f, 2.2f, 1.8f, pulse * brightness};
            AddStrip(previous, next, normal, previousWidth, width, lineColor, lineColor, 1);
            // 一部だけが二股になり、主線から滑らかに離れて戻る。
            const float branch = fork(distance);
            if (branch > 0.001f || previousFork > 0.001f) {
                const auto a = Offset(previous, normal, side * previousFork * 0.16f);
                const auto b = Offset(next, normal, side * branch * 0.16f);
                AddStrip(a, b, normal, 0.014f * previousFork, 0.014f * branch,
                         {2.4f, 0.7f, 0.5f, pulse * previousFork * 0.65f},
                         {2.4f, 0.7f, 0.5f, pulse * branch * 0.65f}, 1);
            }

            previous = next;
            previousWidth = width;
            previousFork = branch;
        }
    }

    // 本体から小さな火花が剥がれ、進行方向へ流れながら外側で消える。
    const int motes = (std::min)(80, int(length / 0.9f));
    for (int i = 0; i < motes; ++i) {
        const float age = std::fmod(clock * 0.037f + float(i) * 0.618034f, 1.0f);
        const float distance = std::fmod(float(i) * 0.93f + clock * 0.21f, length);
        const float side = i % 2 == 0 ? 1.0f : -1.0f;
        const auto center =
            Offset(Offset(start, along, distance), normal, side * (0.24f + age * 1.1f));
        AddGlow(center, along, 0.04f + age * 0.05f, 0.012f,
                {4, 0.7f, 0.3f, std::sin(age * 3.141593f) * pulse * 0.45f});
    }
}

void EffectRenderer::AddLaserEmitter(Vector3 center, float angle, int frame, float charge,
                                     bool firing) {
    charge = (std::clamp)(charge, 0.0f, 1.0f);
    const float time = float(frame);
    const float rotation = time * (firing ? 0.075f : 0.035f);
    const float radius = 1.15f + charge * 0.65f;
    const float alpha = 0.25f + charge * 0.65f;
    const Vector4 red = {3.2f, 0.035f, 0.09f, alpha};
    // 発射口は塗りつぶさず、逆回転する円弧と細い歯で機構の密度を作る。
    for (int ring = 0; ring < 3; ++ring) {
        const float direction = ring == 1 ? -1.0f : 1.0f;
        for (int arc = 0; arc < 3; ++arc) {
            AddArc(center, rotation * direction + float(arc) * 2.094395f,
                   radius * (0.65f + float(ring) * 0.22f), 1.75f, ring == 1 ? 0.027f : 0.016f, red);
        }
    }

    for (int i = 0; i < 18; ++i) {
        const float theta = float(i) * 0.349066f - rotation * 0.65f;
        const Vector3 spoke = {std::cos(theta), std::sin(theta) * 0.7f, 0};
        AddBeam(Offset(center, spoke, radius * 0.79f), Offset(center, spoke, radius * 1.12f),
                0.018f, red);
        const float age = std::fmod(time * 0.026f + float(i) * 0.618034f, 1.0f);
        const float distance = firing ? 0.3f + age * 1.8f : 2.2f - age * 1.8f;
        AddGlow(Offset(center, spoke, distance), spoke, 0.045f + age * 0.08f, 0.023f,
                {3.5f, 0.4f, 0.15f, std::sin(age * 3.141593f) * alpha});
    }

    const float pulse = firing ? 0.95f + std::sin(time * 1.7f) * 0.05f : charge;
    AddGlow(center, {1, 0, 0}, 0.8f, 0.65f, {3, 0.03f, 0.08f, pulse * 0.4f});
    AddGlow(center, {1, 0, 0}, 0.26f, 0.2f, {6, 3.5f, 2.5f, pulse});
    AddGlow(center, {std::cos(angle), std::sin(angle), 0}, 1.0f, 0.022f,
            {4, 1.2f, 0.6f, pulse * 0.65f});
}

void EffectRenderer::AddScorchedGround(Vector3 center, float radius, int age, float fade,
                                       int frame) {
    if (radius <= 0 || fade <= 0) {
        return;
    }

    const bool warning = age < 0;
    const float clock = float(frame);
    const auto hash = [](int seed) {
        const float value = std::sin(float(seed) * 12.9898f) * 43758.5453f;
        return value - std::floor(value);
    };
    // 光の面を実際の床と平行に置き、キャラクターの背後へ浮いた帯にしない。
    const Vector4 base = {1.5f, 0.025f, 0.045f, fade * (warning ? 0.18f : 0.24f)};
    for (int i = 0; i < 16; ++i) {
        const float t0 = float(i) / 16;
        const float t1 = float(i + 1) / 16;
        const Vector3 a = {center.x + (t0 * 2 - 1) * radius, center.y, center.z};
        const Vector3 b = {center.x + (t1 * 2 - 1) * radius, center.y, center.z};
        AddStrip(a, b, {0, 0, 1}, 1.35f, 1.35f, Fade(base, std::sin(t0 * 3.141593f)),
                 Fade(base, std::sin(t1 * 3.141593f)), 1);
    }

    // 亀裂の形は固定し、光量だけを脈動させる。三本の不規則な筋から細い枝が伸びる。
    constexpr int segments = 72;
    for (int vein = 0; vein < 3; ++vein) {
        const float depth = (float(vein) - 1) * 0.7f;
        for (int i = 0; i < segments; ++i) {
            const float x0 = -12 + float(i) / 3;
            const float x1 = x0 + 1.0f / 3;
            if (x1 < -radius || x0 > radius) {
                continue;
            }

            const float z0 = depth + (hash(i * 17 + vein * 83) - 0.5f) * 0.38f;
            const float z1 = depth + (hash((i + 1) * 17 + vein * 83) - 0.5f) * 0.38f;
            const float t0 = (std::clamp)((-radius - x0) * 3, 0.0f, 1.0f);
            const float t1 = (std::clamp)((radius - x0) * 3, 0.0f, 1.0f);
            const Vector3 a = {center.x + x0 + t0 / 3, center.y + 0.012f,
                               center.z + z0 + (z1 - z0) * t0};
            const Vector3 b = {center.x + x0 + t1 / 3, center.y + 0.012f,
                               center.z + z0 + (z1 - z0) * t1};
            const float pulse = 0.65f + std::sin(clock * 0.07f + float(i) * 0.8f + vein) * 0.15f;
            const Vector4 red = {warning ? 2.8f : 3.6f, 0.055f, 0.08f, fade * pulse};
            AddStrip(a, b, {0, 0, 1}, 0.09f, 0.09f, Fade(red, 0.3f), Fade(red, 0.3f), 1);
            AddStrip(a, b, {0, 0, 1}, 0.022f, 0.022f, red, red, 1);
            if (i % 4 == vein) {
                const float side = vein == 1 ? -1.0f : 1.0f;
                const Vector3 tip = {b.x - 0.2f, b.y,
                                     b.z + side * (0.25f + hash(i + vein) * 0.35f)};
                AddStrip(b, tip, {0, 0, 1}, 0.018f, 0.004f, red, Fade(red, 0.4f), 1);
            }
        }
    }

    if (warning) {
        return;
    }

    // 接点から左右へ広がる二つの低い衝撃。到達した場所に亀裂と炎が残る。
    if (age < 12) {
        const float frontFade = (1 - float(age) / 12) * fade;
        for (float side : {-1.0f, 1.0f}) {
            const Vector3 front = {center.x + side * radius, center.y + 0.35f, center.z - 0.1f};
            AddGlow(front, {side, 0.25f, 0}, 1.05f, 0.4f, {4.2f, 0.32f, 0.1f, frontFade});
            AddGlow(front, {0, 1, 0}, 0.65f, 0.14f, {4.0f, 1.0f, 0.4f, frontFade * 0.9f});
        }
    }

    // 撃ち込みの瞬間だけ大きな半円の衝撃と結晶片を噴き上げる。
    // 奥側に置き、手前のプレイヤーの輪郭を隠さない。
    if (age < 28) {
        const float elapsed = float(age);
        const float burstFade = (1 - elapsed / 28) * fade;
        if (age < 16) {
            const Vector3 contact = {center.x, center.y + 0.07f, center.z};
            const float ringFade = (1 - elapsed / 16) * fade;
            AddArc(contact, 1.570796f, 0.5f + elapsed * 0.18f, 3.141593f, 0.075f,
                   {4, 0.28f, 0.1f, ringFade});
            AddGlow(contact, {0, 1, 0}, 1.35f * ringFade, 0.3f, {4, 0.35f, 0.15f, ringFade * 0.8f});
        }

        for (int i = 0; i < 42; ++i) {
            const float angle = 0.15f + hash(i * 59 + 7) * 2.84f;
            const float speed = 0.13f + hash(i * 31 + 17) * 0.22f;
            const float vx = std::cos(angle) * speed;
            const float vy = std::sin(angle) * speed;
            const Vector3 particle = {center.x + vx * elapsed,
                                      center.y + 0.08f + vy * elapsed - elapsed * elapsed * 0.008f,
                                      center.z + hash(i * 19) * 0.8f};
            if (particle.y < center.y) {
                continue;
            }

            const bool blue = i % 7 == 0;
            const Vector4 color = blue ? Vector4{0.15f, 1.1f, 2.8f, burstFade * 0.8f}
                                       : Vector4{3.6f, 0.4f, 0.12f, burstFade};
            AddGlow(particle, {vx, vy - elapsed * 0.016f, 0}, 0.13f, 0.035f, color);
            if (i % 3 == 0) {
                AddCrystal(particle, angle + elapsed * 0.14f, 0.08f + hash(i * 11) * 0.05f, color);
            }
        }
    }

    // 炎はプレイヤーの背後に置く。赤い根元から細い橙色の先へ収束させる。
    for (int i = 0; i < 36; ++i) {
        const float x = -11.8f + float(i) * (23.6f / 35) + (hash(i + 13) - 0.5f) * 0.2f;
        if (std::abs(x) > radius) {
            continue;
        }

        const float phase = std::fmod(clock * 0.027f + hash(i * 71), 1.0f);
        const float life = std::sin(phase * 3.141593f);
        const float height = (0.6f + hash(i * 37 + 5) * 0.7f) * life * fade;
        const float drift = std::sin(clock * 0.08f + float(i) * 2.1f) * 0.11f;
        const float depth = center.z + 0.1f + hash(i * 23) * 1.2f;
        const Vector3 root = {center.x + x, center.y + 0.025f, depth};
        const Vector3 bend = {root.x + drift, root.y + height * 0.5f, depth};
        const Vector3 tip = {root.x - drift * 0.4f, root.y + height, depth};
        const Vector4 red = {3.5f, 0.06f, 0.1f, fade * life * 0.85f};
        const Vector4 orange = {3.6f, 0.65f, 0.12f, fade * life * 0.7f};
        AddStrip(root, bend, {1, 0, 0}, 0.2f, 0.09f, red, orange, 1);
        AddStrip(bend, tip, {1, 0, 0}, 0.09f, 0, orange, Fade(orange, 0), 1);
        if (i % 2 == 0) {
            const Vector3 ember = {root.x + drift * 2, root.y + phase * 1.8f, depth};
            AddGlow(ember, {0.2f, 1, 0}, 0.055f, 0.025f,
                    {3.4f, 0.65f, 0.15f, (1 - phase) * fade * 0.8f});
        }

        // 常時びりびりさせず、亀裂から短い放電が時々跳ねる程度に留める。
        if (i % 6 == 0 && phase < 0.16f) {
            const Vector3 a = {root.x - 0.16f, root.y + 0.025f, depth};
            const Vector3 b = {root.x, root.y + 0.18f, depth};
            const Vector3 c = {root.x + 0.18f, root.y + 0.04f, depth};
            const Vector4 spark = {2.4f, 0.3f, 0.15f, fade * 0.45f};
            AddBeam(a, b, 0.012f, spark);
            AddBeam(b, c, 0.008f, spark);
        }
    }
}
