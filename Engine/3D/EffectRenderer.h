#pragma once
#include "Base/DirectXCommon.h"
#include "ViewProjection.h"
#include "Vector4.h"
#include <array>
#include <span>

// 光の板・移動経路の帯・円弧を一つの動的メッシュにまとめる。
// HDRの描画先で使用し、加算合成する。深度は読み取りだけで書き込まない。
class EffectRenderer {
public:
    void Initialize(DirectXCommon *dxCommon);
    void Begin();
    void AddGlow(Vector3 center, Vector3 axis, float length, float width, Vector4 color);
    void AddRibbon(std::span<const Vector3> path, float width, Vector4 color);
    // 二つの帯を実際の移動経路に沿って交差させ、端で一つに収束させる。
    void AddInterwovenRibbon(std::span<const Vector3> path, float fade, float phase);
    void AddCrystal(Vector3 center, float angle, float size, Vector4 color);
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
    DirectXCommon *dxCommon_ = nullptr;
    Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature_;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> pipeline_;
    Microsoft::WRL::ComPtr<ID3D12Resource> vertexBuffer_;
    Vertex *vertices_ = nullptr;
    D3D12_VERTEX_BUFFER_VIEW bufferView_{};
    size_t vertexCount_ = 0;
};
