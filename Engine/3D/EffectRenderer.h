#pragma once
#include "Base/DirectXCommon.h"
#include "ViewProjection.h"
#include "Vector4.h"
#include <array>
#include <span>
#include <memory>

struct ModelData;

// 光の板・移動経路の帯・円弧を一つの動的メッシュにまとめる。
// HDRの描画先で使用し、加算合成する。深度は読み取りだけで書き込まない。
class EffectRenderer {
public:
    void Initialize(DirectXCommon *dxCommon);
    void Begin();
    void AddGlow(Vector3 center, Vector3 axis, float length, float width, Vector4 color);
    void AddRibbon(std::span<const Vector3> path, float width, Vector4 color);
    // 二つの帯を実際の移動経路に沿って交差させ、端で一つに収束させる。
    void AddInterwovenRibbon(std::span<const Vector3> path, float fade, float phase, float width = 1, float spread = 1, float twist = 1, float glow = 1,
                             Vector4 blue = {0.08f, 0.65f, 2.4f, 1}, Vector4 red = {2.4f, 0.04f, 0.25f, 1});
    enum class ObjEffect { Shard, Projectile, Lance, Dust };
    void AddObj(ObjEffect mesh, Vector3 center, Vector3 rotation, Vector3 scale, Vector4 color);
    void AddCrystal(Vector3 center, float angle, float size, Vector4 color);
    // Draw the actual crystal facets into the HDR emission layer.
    void AddModelEmission(const ModelData &model, const Matrix4x4 &world, float intensity);
    void AddBeam(Vector3 start, Vector3 end, float width, Vector4 color);
    void AddElectricBeam(Vector3 start, Vector3 end, int frame, float intensity);
    void AddLaserEmitter(Vector3 center, float angle, int frame, float charge, bool firing);
    // 床の固定した亀裂・鋭い噴出光・結晶片。age=-1は予告だけを描く。
    void AddScorchedGround(Vector3 center, float radius, int age, float fade, int frame);
    void AddArc(Vector3 center, float angle, float radius, float sweep, float width, Vector4 color);
    void Draw(const ViewProjection &camera);
    bool HasGeometry() const {
        return vertexCount_ != 0;
    }

private:
    struct Vertex {
        Vector3 position;
        float u, v;
        Vector4 color;
        float shape;
    };

    static constexpr size_t capacity = 32768;
    void AddStrip(Vector3 a, Vector3 b, Vector3 normal, float widthA, float widthB, Vector4 colorA,
                  Vector4 colorB, float shape);
    void AddQuad(const std::array<Vertex, 4> &quad);
    std::array<std::shared_ptr<const ModelData>,4> effectModels_;
    DirectXCommon *dxCommon_ = nullptr;
    Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature_;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> pipeline_;
    Microsoft::WRL::ComPtr<ID3D12Resource> vertexBuffer_;
    Vertex *vertices_ = nullptr;
    D3D12_VERTEX_BUFFER_VIEW bufferView_{};
    size_t vertexCount_ = 0;
};
