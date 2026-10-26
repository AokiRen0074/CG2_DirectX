#pragma once
#include "Base/DirectXCommon.h"
#include "Vector4.h"
#include <array>
#include <string_view>
#include <unordered_map>
#include <vector>

// One transparent atlas and one draw call for the title, HUD and clear
// presentation.
class SpearScreenRenderer {
public:
  void Initialize(DirectXCommon *graphics);
  void Begin();
  void PrepareImage(const std::string &path, const std::string &loadPath = "");
  void Image(const std::string &path, float x, float y, float width,
             float height, Vector4 tint);
  void Rect(float x, float y, float width, float height, Vector4 color);
  void Text(std::string_view text, float x, float y, float size, Vector4 color,
            float tracking = 0, bool centered = false);
  void Diamond(float x, float y, float radius, Vector4 color);
  void Radar(float x, float y, float width, float height,
             const std::array<int, 5> &levels, float progress, Vector4 color);
  void Draw(int clipX = 0, int clipY = 0, int clipWidth = 1280,
            int clipHeight = 720);

private:
  struct Vertex {
    float x, y, u, v;
    Vector4 color;
  };
  static constexpr size_t capacity = 16384;
  void Quad(Vertex a, Vertex b, Vertex c, Vertex d);
  DirectXCommon *graphics_ = nullptr;
  Microsoft::WRL::ComPtr<ID3D12RootSignature> signature_;
  Microsoft::WRL::ComPtr<ID3D12PipelineState> pipeline_;
  Microsoft::WRL::ComPtr<ID3D12Resource> buffer_;
  Vertex *vertices_ = nullptr;
  size_t count_ = 0;
  uint32_t atlas_ = 0;
  uint32_t currentTexture_ = 0;
  struct Batch {
    uint32_t texture;
    size_t first, count;
  };
  std::vector<Batch> batches_;
  std::unordered_map<std::string, uint32_t> images_;
  D3D12_VERTEX_BUFFER_VIEW view_{};
};
