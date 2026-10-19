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

    const Vector3 axis = {(end.x - start.x) / length, (end.y - start.y) / length, 0};
    const Vector3 normal = {-axis.y, axis.x, 0};
    const float phase = float(frame) * 0.11f;
    const float fade = (std::min)(1.2f, intensity);
    // 帯の形は保ち、敵の攻撃は金と橙に限定してプレイヤーの赤青と区別する。
    AddBeam(start, end, 0.72f, {1.7f, 0.65f, 0.025f, fade * 0.10f});
    AddBeam(start, end, 0.45f, {1.7f, 0.18f, 0.015f, fade * 0.10f});
    const int segments = (std::clamp)(int(std::ceil(length / 0.16f)), 24, 512);
    for (int band = 0; band < 2; ++band) {
        const float side = band == 0 ? 1.0f : -1.0f;
        const Vector4 color = band == 0 ? Vector4{2.4f, 0.6f, 0.14f, fade * 0.9f}
                                        : Vector4{2.1f, 0.3f, 0.08f, fade * 0.9f};
        const Vector4 core = band == 0 ? Vector4{3.0f, 2.1f, 1.25f, fade * 0.75f}
                                       : Vector4{2.8f, 1.25f, 0.55f, fade * 0.7f};
        const auto position = [&](float distance) {
            const float envelope =
                (std::min)(1.0f, distance / 0.75f) * (std::min)(1.0f, (length - distance) / 0.9f);
            const float wave = distance + std::sin(distance * 0.36f) * 0.22f - phase;
            auto point =
                Offset(Offset(start, axis, distance), normal,
                       side * std::sin(wave) *
                           (0.3f + std::sin(distance * 0.37f - phase * 0.3f) * 0.07f) * envelope);
            // 交差で奥行きを入れ替える。プレイヤーの手前には出さない。
            point.z += side * std::cos(wave) * 0.045f;
            return point;
        };
        const auto width = [&](float distance) {
            const float endTaper = (std::clamp)((length - distance) / 0.7f, 0.0f, 1.0f);
            const float head =
                0.5f + std::cos(distance * 0.95f - phase + float(band) * 2.0f) * 0.5f;
            return (0.028f + head * head * 0.2f) * endTaper;
        };
        for (int i = 0; i < segments; ++i) {
            const float d0 = length * float(i) / float(segments);
            const float d1 = length * float(i + 1) / float(segments);
            const auto a = position(d0);
            const auto b = position(d1);
            AddStrip(a, b, normal, width(d0), width(d1), color, color, 1);
            AddStrip(a, b, normal, width(d0) * 0.16f, width(d1) * 0.16f, core, core, 1);
        }
    }

    // 芯は細く保つ。放電の枝を増やす代わりに、帯の流れと結晶片で速度を見せる。
    AddBeam(start, end, 0.018f, {2.5f, 1.7f, 1.0f, fade * 0.35f});
    const int motes = (std::min)(48, int(length / 1.4f));
    for (int i = 0; i < motes; ++i) {
        const float distance = std::fmod(float(i) * 1.43f + float(frame) * 0.15f, length);
        const float side = i % 2 == 0 ? 1.0f : -1.0f;
        auto point = Offset(Offset(start, axis, distance), normal,
                            side * (0.45f + std::sin(float(i) + phase * 0.3f) * 0.12f));
        const Vector4 color = i % 2 == 0 ? Vector4{2.4f, 0.7f, 0.18f, fade * 0.5f}
                                         : Vector4{2.4f, 0.3f, 0.02f, fade * 0.5f};
        AddCrystal(point, std::atan2(axis.y, axis.x), 0.045f, color);
    }
}

void EffectRenderer::AddLaserEmitter(Vector3 center, float angle, int frame, float charge,
                                     bool firing) {
    charge = (std::clamp)(charge, 0.0f, 1.0f);
    const float time = float(frame);
    const float rotation = time * (firing ? 0.075f : 0.035f);
    const float radius = 1.15f + charge * 0.65f;
    const float alpha = 0.25f + charge * 0.65f;
    const Vector4 red = {2.6f, 0.55f, 0.13f, alpha};
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
                {3.2f, 1.0f, 0.12f, std::sin(age * 3.141593f) * alpha});
    }

    const float pulse = firing ? 0.95f + std::sin(time * 1.7f) * 0.05f : charge;
    AddGlow(center, {1, 0, 0}, 0.8f, 0.65f, {2.6f, 0.35f, 0.025f, pulse * 0.4f});
    AddGlow(center, {1, 0, 0}, 0.26f, 0.2f, {4.0f, 2.4f, 0.85f, pulse});
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
    // 床の亀裂は固定形状。火や帯を揺らさず、切れ目の明るさだけが脈動する。
    constexpr int segments = 80;
    for (int vein = 0; vein < 3; ++vein) {
        const float depth = (float(vein) - 1) * 0.6f;
        for (int i = 0; i < segments; ++i) {
            // 主亀裂は連続、脇の亀裂には長短と途切れを作る。
            if (vein != 1 && hash(i / 3 * 29 + vein * 131) < 0.35f) {
                continue;
            }

            const float x0 = -12 + float(i) * 0.3f;
            const float x1 = x0 + 0.3f;
            if (x1 <= -radius || x0 >= radius) {
                continue;
            }

            const float z0 = depth + (hash(i * 67 + vein * 113) - 0.5f) * 0.5f;
            const float z1 = depth + (hash((i + 1) * 67 + vein * 113) - 0.5f) * 0.5f;
            const float t0 = (std::clamp)((-radius - x0) / 0.3f, 0.0f, 1.0f);
            const float t1 = (std::clamp)((radius - x0) / 0.3f, 0.0f, 1.0f);
            const Vector3 a = {center.x + x0 + t0 * 0.3f, center.y + 0.014f,
                               center.z + z0 + (z1 - z0) * t0};
            const Vector3 b = {center.x + x0 + t1 * 0.3f, center.y + 0.014f,
                               center.z + z0 + (z1 - z0) * t1};
            const float edge = (std::clamp)((radius - std::abs(x0 + 0.15f)) / 0.4f, 0.0f, 1.0f);
            const float pulse =
                warning ? 0.6f : 0.75f + std::sin(clock * 0.07f + float(i) * 0.8f) * 0.2f;
            const float opacity = fade * pulse * edge;
            const Vector4 orange = {2.5f, 0.32f, 0.015f, opacity};
            const Vector4 gold = {2.8f, 0.8f, 0.23f, opacity};
            AddStrip(a, b, {0, 0, 1}, 0.095f, 0.095f, Fade(orange, 0.35f), Fade(orange, 0.35f), 1);
            AddStrip(a, b, {0, 0, 1}, vein == 1 ? 0.028f : 0.018f, vein == 1 ? 0.028f : 0.018f,
                     gold, gold, 1);
            if (hash(i * 41 + vein * 71) > 0.72f) {
                const float side = vein == 0 ? -1.0f : 1.0f;
                const Vector3 bend = {b.x - 0.13f, b.y, b.z + side * 0.24f};
                const Vector3 tip = {b.x + 0.12f, b.y, b.z + side * (0.4f + hash(i * 7) * 0.25f)};
                AddStrip(b, bend, {0, 0, 1}, 0.022f, 0.015f, gold, gold, 1);
                AddStrip(bend, tip, {0, 0, 1}, 0.015f, 0.002f, gold, Fade(gold, 0.2f), 1);
            }
        }
    }

    if (warning) {
        return;
    }

    // 衝撃が左右へ伝わった瞬間だけ、床を横切る鋭い光を立てる。
    if (age < 12) {
        const float opacity = (1 - float(age) / 12) * fade;
        for (float side : {-1.0f, 1.0f}) {
            const Vector3 front = {center.x + side * radius, center.y + 0.3f, center.z + 0.1f};
            AddGlow(front, {0, 1, 0}, 0.7f, 0.07f, {3.0f, 1.2f, 0.15f, opacity});
            AddGlow(front, {1, 0, 0}, 0.65f, 0.07f, {2.8f, 0.4f, 0.02f, opacity * 0.8f});
        }
    }

    // 噴出は細い直線と尖った面。位置を動かさず、短い時間だけ光量を上げる。
    for (int i = 0; i < 16; ++i) {
        const float x = -11.5f + float(i) * (23.0f / 15) + (hash(i * 23) - 0.5f) * 0.25f;
        if (std::abs(x) > radius) {
            continue;
        }

        const float cycle = std::fmod(clock * 0.019f + hash(i * 97 + 13), 1.0f);
        if (cycle > 0.28f) {
            continue;
        }

        const float t = cycle / 0.28f;
        const float envelope = (1 - t) * (1 - t) * (std::min)(1.0f, t * 12);
        const float opacity = envelope * fade;
        const float height = 0.5f + hash(i * 37 + 7) * 0.7f;
        const Vector3 root = {center.x + x, center.y + 0.03f, center.z + 0.5f};
        const Vector3 tip = {root.x + (hash(i * 43) - 0.5f) * 0.1f, root.y + height, root.z};
        AddStrip(root, tip, {1, 0, 0}, 0.11f, 0, {2.7f, 0.4f, 0.02f, opacity * 0.65f},
                 {2.7f, 0.8f, 0.08f, 0}, 1);
        AddGlow({root.x, root.y + height * 0.45f, root.z}, {0, 1, 0}, height * 0.55f, 0.025f,
                {3.2f, 1.9f, 1.0f, opacity});
        AddGlow(root, {1, 0, 0}, 0.22f, 0.05f, {3, 0.7f, 0.05f, opacity * 0.8f});
    }

    // 着弾の破片は敵色の金と橙。プレイヤーの赤青をここでは使わない。
    if (age < 24) {
        const float elapsed = float(age);
        const float opacity = (1 - elapsed / 24) * fade;
        for (int i = 0; i < 26; ++i) {
            const float angle = 0.15f + hash(i * 59 + 7) * 2.84f;
            const float speed = 0.12f + hash(i * 31 + 17) * 0.22f;
            const float vx = std::cos(angle) * speed;
            const float vy = std::sin(angle) * speed;
            const Vector3 point = {center.x + vx * elapsed,
                                   center.y + 0.08f + vy * elapsed - elapsed * elapsed * 0.008f,
                                   center.z + 0.5f + hash(i * 19) * 0.6f};
            if (point.y < center.y) {
                continue;
            }

            const Vector4 color = i % 2 == 0 ? Vector4{2.8f, 0.85f, 0.24f, opacity}
                                             : Vector4{2.6f, 0.35f, 0.02f, opacity};
            AddGlow(point, {vx, vy - elapsed * 0.016f, 0}, 0.1f, 0.022f, color);
            if (i % 2 == 0) {
                AddCrystal(point, angle + elapsed * 0.14f, 0.07f + hash(i * 11) * 0.04f, color);
            }
        }
    }
}
