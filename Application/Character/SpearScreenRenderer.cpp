#include "SpearScreenRenderer.h"
#include "2D/TextureManager.h"
#include <cmath>
#include <algorithm>
#include <stdexcept>

namespace {
void CheckUi(HRESULT result) {
  if (FAILED(result))
    throw std::runtime_error("Could not initialize game presentation renderer");
}
} // namespace
void SpearScreenRenderer::Initialize(DirectXCommon *graphics) {
  graphics_ = graphics;
  auto *device = graphics->GetDevice();
  atlas_ = TextureManager::Load("Resources/UI/PrismUI/font-atlas.png");
  D3D12_DESCRIPTOR_RANGE range{};
  range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
  range.NumDescriptors = 1;
  range.OffsetInDescriptorsFromTableStart =
      D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
  D3D12_ROOT_PARAMETER parameter{};
  parameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
  parameter.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
  parameter.DescriptorTable.NumDescriptorRanges = 1;
  parameter.DescriptorTable.pDescriptorRanges = &range;
  D3D12_STATIC_SAMPLER_DESC sampler{};
  sampler.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
  sampler.AddressU = sampler.AddressV = sampler.AddressW =
      D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
  sampler.MaxLOD = D3D12_FLOAT32_MAX;
  sampler.ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
  sampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
  D3D12_ROOT_SIGNATURE_DESC root{};
  root.NumParameters = 1;
  root.pParameters = &parameter;
  root.NumStaticSamplers = 1;
  root.pStaticSamplers = &sampler;
  root.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
  Microsoft::WRL::ComPtr<ID3DBlob> blob, error;
  CheckUi(D3D12SerializeRootSignature(&root, D3D_ROOT_SIGNATURE_VERSION_1,
                                      &blob, &error));
  CheckUi(device->CreateRootSignature(0, blob->GetBufferPointer(),
                                      blob->GetBufferSize(),
                                      IID_PPV_ARGS(&signature_)));
  auto vs = graphics->CompilerShader(
      L"Resources/Shaders/PrismUI.VS.hlsl", L"vs_6_0", graphics->GetDxcUtils(),
      graphics->GetDxcCompiler(), graphics->GetIncludeHandler());
  auto ps = graphics->CompilerShader(
      L"Resources/Shaders/PrismUI.PS.hlsl", L"ps_6_0", graphics->GetDxcUtils(),
      graphics->GetDxcCompiler(), graphics->GetIncludeHandler());
  D3D12_INPUT_ELEMENT_DESC layout[] = {
      {"POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0,
       D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
      {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 8,
       D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
      {"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 16,
       D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}};
  D3D12_GRAPHICS_PIPELINE_STATE_DESC p{};
  p.pRootSignature = signature_.Get();
  p.VS = {vs->GetBufferPointer(), vs->GetBufferSize()};
  p.PS = {ps->GetBufferPointer(), ps->GetBufferSize()};
  p.InputLayout = {layout, 3};
  p.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
  p.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
  p.RasterizerState.DepthClipEnable = TRUE;
  auto &blend = p.BlendState.RenderTarget[0];
  blend.BlendEnable = TRUE;
  blend.SrcBlend = D3D12_BLEND_SRC_ALPHA;
  blend.DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
  blend.BlendOp = blend.BlendOpAlpha = D3D12_BLEND_OP_ADD;
  blend.SrcBlendAlpha = D3D12_BLEND_ONE;
  blend.DestBlendAlpha = D3D12_BLEND_INV_SRC_ALPHA;
  blend.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
  p.SampleMask = UINT_MAX;
  p.SampleDesc.Count = 1;
  p.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
  p.NumRenderTargets = 1;
  p.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
  p.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
  p.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_ALWAYS;
  CheckUi(device->CreateGraphicsPipelineState(&p, IID_PPV_ARGS(&pipeline_)));
  D3D12_HEAP_PROPERTIES heap{};
  heap.Type = D3D12_HEAP_TYPE_UPLOAD;
  D3D12_RESOURCE_DESC b{};
  b.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
  b.Width = capacity * sizeof(Vertex);
  b.Height = 1;
  b.DepthOrArraySize = b.MipLevels = 1;
  b.SampleDesc.Count = 1;
  b.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
  CheckUi(device->CreateCommittedResource(&heap, D3D12_HEAP_FLAG_NONE, &b,
                                          D3D12_RESOURCE_STATE_GENERIC_READ,
                                          nullptr, IID_PPV_ARGS(&buffer_)));
  D3D12_RANGE noRead{0, 0};
  CheckUi(buffer_->Map(0, &noRead, reinterpret_cast<void **>(&vertices_)));
  view_ = {buffer_->GetGPUVirtualAddress(), UINT(b.Width), sizeof(Vertex)};
}
void SpearScreenRenderer::Begin() {
  count_ = 0;
  batches_.clear();
  currentTexture_ = atlas_;
}
void SpearScreenRenderer::PrepareImage(const std::string &path,
                                       const std::string &loadPath) {
  if (!images_.contains(path))
    images_[path] = TextureManager::Load(loadPath.empty() ? path : loadPath);
}
void SpearScreenRenderer::Image(const std::string &path, float x, float y,
                                float width, float height, Vector4 c) {
  currentTexture_ = images_.at(path);
  Quad({x, y, 0, 0, c}, {x + width, y, 1, 0, c}, {x, y + height, 0, 1, c},
       {x + width, y + height, 1, 1, c});
}
void SpearScreenRenderer::Quad(Vertex a, Vertex b, Vertex c, Vertex d) {
  if (count_ + 6 > capacity)
    return;
  if (batches_.empty() || batches_.back().texture != currentTexture_)
    batches_.push_back({currentTexture_, count_, 0});
  batches_.back().count += 6;
  for (auto v : {a, b, c, c, b, d})
    vertices_[count_++] = v;
}
void SpearScreenRenderer::Rect(float x, float y, float width, float height,
                               Vector4 c) {
  currentTexture_ = atlas_;
  constexpr float u = 1008.0f / 1024, v = 616.0f / 640;
  Quad({x, y, u, v, c}, {x + width, y, u, v, c}, {x, y + height, u, v, c},
       {x + width, y + height, u, v, c});
}
void SpearScreenRenderer::Diamond(float x, float y, float radius, Vector4 c) {
  currentTexture_ = atlas_;
  constexpr float u = 1008.0f / 1024, v = 616.0f / 640;
  Quad({x, y - radius, u, v, c}, {x + radius, y, u, v, c},
       {x - radius, y, u, v, c}, {x, y + radius, u, v, c});
}
void SpearScreenRenderer::Text(std::string_view text, float x, float y,
                               float size, Vector4 c, float tracking,
                               bool centered) {
  currentTexture_ = atlas_;
  const float advance = size * 0.6f + tracking;
  if (centered)
    x -= (float(text.size()) * advance - tracking) * 0.5f;
  for (unsigned char letter : text) {
    const int index = letter >= 32 && letter < 127 ? letter - 32 : 31;
    const float u = float(index % 16) / 16, v = float(index / 16) * 96 / 640;
    const float w = size, h = size * 1.5f;
    Quad({x, y, u, v, c}, {x + w, y, u + 1.0f / 16, v, c},
         {x, y + h, u, v + 96.0f / 640, c},
         {x + w, y + h, u + 1.0f / 16, v + 96.0f / 640, c});
    x += advance;
  }
}
void SpearScreenRenderer::Radar(float x, float y, float width, float height,
                                const std::array<int, 5> &levels,
                                float progress, Vector4 color) {
  currentTexture_ = atlas_;
  constexpr float u = 1008.0f / 1024, v = 616.0f / 640;
  const float cx = x + width * .5f, cy = y + height * .5f;
  std::array<std::array<float, 2>, 5> points{};
  for (size_t i = 0; i < 5; ++i) {
    const float angle = -1.5707963f + float(i) * 6.2831853f / 5;
    const float amount = float((std::clamp)(levels[i], 1, 5)) * .2f *
                         (std::clamp)(progress, 0.0f, 1.0f);
    points[i] = {cx + std::cos(angle) * width * .44f * amount,
                 cy + std::sin(angle) * height * .44f * amount};
  }
  auto fill = color;
  fill.w *= .2f;
  for (size_t i = 0; i < 5; ++i) {
    const auto &a = points[i], &b = points[(i + 1) % 5];
    Quad({cx, cy, u, v, fill}, {a[0], a[1], u, v, fill},
         {b[0], b[1], u, v, fill}, {b[0], b[1], u, v, fill});
    const float dx = b[0] - a[0], dy = b[1] - a[1];
    const float length = (std::max)(.001f, std::sqrt(dx * dx + dy * dy));
    const float nx = -dy / length, ny = dx / length;
    Quad({a[0] + nx, a[1] + ny, u, v, color},
         {b[0] + nx, b[1] + ny, u, v, color},
         {a[0] - nx, a[1] - ny, u, v, color},
         {b[0] - nx, b[1] - ny, u, v, color});
    Diamond(a[0], a[1], 2.5f, color);
  }
}
void SpearScreenRenderer::Draw(int clipX, int clipY, int clipWidth,
                               int clipHeight) {
  if (!count_)
    return;
  auto *command = graphics_->GetCommandList();
  command->SetGraphicsRootSignature(signature_.Get());
  command->SetPipelineState(pipeline_.Get());
  ID3D12DescriptorHeap *heaps[] = {graphics_->GetSrvDescriptorHeap()};
  command->SetDescriptorHeaps(1, heaps);
  command->SetGraphicsRootDescriptorTable(
      0, TextureManager::GetInstance()->GetSrvHandleGPU(atlas_));
  D3D12_VIEWPORT viewport{0, 0, 1280, 720, 0, 1};
  D3D12_RECT scissor{clipX, clipY, clipX + clipWidth, clipY + clipHeight};
  command->RSSetViewports(1, &viewport);
  command->RSSetScissorRects(1, &scissor);
  command->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
  command->IASetVertexBuffers(0, 1, &view_);
  for (const auto &batch : batches_) {
    command->SetGraphicsRootDescriptorTable(
        0, TextureManager::GetInstance()->GetSrvHandleGPU(batch.texture));
    command->DrawInstanced(UINT(batch.count), 1, UINT(batch.first), 0);
  }
}
