#include "ProceduralNeon.h"
#include <cmath>



ProceduralNeon::~ProceduralNeon() {
	// ComPtrが自動で解放を行うため記述不要
}

void ProceduralNeon::Initialize(DirectXCommon* dxCommon) {
	auto device = dxCommon->GetDevice();
	D3D12_HEAP_PROPERTIES uploadHeap{};
	uploadHeap.Type = D3D12_HEAP_TYPE_UPLOAD;

	D3D12_RESOURCE_DESC resDesc{};
	resDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	resDesc.Height = 1;
	resDesc.DepthOrArraySize = 1;
	resDesc.MipLevels = 1;
	resDesc.SampleDesc.Count = 1;
	resDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

	// 1. 頂点バッファの構築 (4頂点)
	UINT vbSize = sizeof(VertexData) * 12;
	resDesc.Width = vbSize;
	device->CreateCommittedResource(&uploadHeap, D3D12_HEAP_FLAG_NONE, &resDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&vertexBuffer_));

	vbView_.BufferLocation = vertexBuffer_->GetGPUVirtualAddress();
	vbView_.SizeInBytes = vbSize;
	vbView_.StrideInBytes = sizeof(VertexData);
	vertexBuffer_->Map(0, nullptr, reinterpret_cast<void**>(&mappedVertex_));

	// 2. 行列バッファの構築 (256バイトアライメント必須)
	resDesc.Width = (sizeof(TransformData) + 0xff) & ~0xff;
	device->CreateCommittedResource(&uploadHeap, D3D12_HEAP_FLAG_NONE, &resDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&transformBuffer_));
	transformBuffer_->Map(0, nullptr, reinterpret_cast<void**>(&mappedTransform_));

	// 3. マテリアルバッファの構築 (256バイトアライメント必須)
	resDesc.Width = (sizeof(MaterialData) + 0xff) & ~0xff;
	device->CreateCommittedResource(&uploadHeap, D3D12_HEAP_FLAG_NONE, &resDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&materialBuffer_));
	materialBuffer_->Map(0, nullptr, reinterpret_cast<void**>(&mappedMaterial_));
}

// ==========================================
// 1. SetMaterial の固定値を解除する
// ==========================================
void ProceduralNeon::SetMaterial(float intensity, const Vector3& color, float radius, float softness, float tubeLength) {
	mappedMaterial_->color[0] = color.x;
	mappedMaterial_->color[1] = color.y;
	mappedMaterial_->color[2] = color.z;
	mappedMaterial_->color[3] = 1.0f;
	mappedMaterial_->intensity = intensity;
	mappedMaterial_->radius = radius;      // 🌟 ここ！ 0.4fの固定を解除し、radiusを受け取る！
	mappedMaterial_->softness = softness;
	mappedMaterial_->tubeLength = tubeLength;
}

// ==========================================
// 2. Update の計算を修正する
// ==========================================
void ProceduralNeon::Update(const Vector3& start, const Vector3& end, float thickness, const Vector3& cameraPos, const ViewProjection& viewProj) {
	Vector3 lineDir = Normalize(Subtract(end, start));
	Vector3 camToStart = Normalize(Subtract(cameraPos, start));
	Vector3 right = Normalize(Cross(camToStart, lineDir));
	if (Length(right) < 0.001f) { right = { 1.0f, 0.0f, 0.0f }; }

	// 🌟 修正：前後に少し「透明な余白」を作る（光が途切れるのを防ぐため）
	float glowMargin = 0.5f;
	Vector3 canvasStart = Subtract(start, Multiply(glowMargin, lineDir));
	Vector3 canvasEnd = Add(end, Multiply(glowMargin, lineDir));

	Vector3 rightOffset = Multiply(thickness / 2.0f, right);

	// 4つの角の座標（余白を持たせたキャンバス）
	Vector3 bl = Subtract(canvasStart, rightOffset);
	Vector3 br = Add(canvasStart, rightOffset);
	Vector3 tl = Subtract(canvasEnd, rightOffset);
	Vector3 tr = Add(canvasEnd, rightOffset);

	auto setVert = [&](int idx, const Vector3& p, float u, float v) {
		mappedVertex_[idx].pos = p;
		mappedVertex_[idx].normal = { 0, 0, -1 };
		mappedVertex_[idx].uv = { u, v };
		};

	// 表面 (6頂点)
	setVert(0, bl, 0.0f, 1.0f);
	setVert(1, br, 1.0f, 1.0f);
	setVert(2, tl, 0.0f, 0.0f);
	setVert(3, br, 1.0f, 1.0f);
	setVert(4, tr, 1.0f, 0.0f);
	setVert(5, tl, 0.0f, 0.0f);

	// 裏面 (6頂点)
	setVert(6, bl, 0.0f, 1.0f);
	setVert(7, tl, 0.0f, 0.0f);
	setVert(8, br, 1.0f, 1.0f);
	setVert(9, br, 1.0f, 1.0f);
	setVert(10, tl, 0.0f, 0.0f);
	setVert(11, tr, 1.0f, 0.0f);

	// ==========================================
	// 🌟 最重要修正：カメラへの張り付きを直す！
	// 頂点はすでにワールド座標にあるので、ここでは「何もしない行列（単位行列）」を渡す
	// ==========================================
	mappedTransform_->matWorld = {
		1.0f, 0.0f, 0.0f, 0.0f,
		0.0f, 1.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f
	};
}

void ProceduralNeon::Draw(DirectXCommon* dxCommon) {
	auto commandList = dxCommon->GetCommandList();

	// 頂点バッファとトポロジー（四角形）をセット
	commandList->IASetVertexBuffers(0, 1, &vbView_);
	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// 定数バッファをセット（0:行列, 1:マテリアル）
	// ※注意：もしエンジンがDescriptorTableを使っている場合、ここは動かない可能性があります。
	// その場合は「直前に描画したネオンの設定を完全に間借りする」という荒業がSimpleな解決策になります。
	commandList->SetGraphicsRootConstantBufferView(0, transformBuffer_->GetGPUVirtualAddress());
	commandList->SetGraphicsRootConstantBufferView(3, materialBuffer_->GetGPUVirtualAddress());

	// 4頂点で描画
	commandList->DrawInstanced(12, 1, 0, 0);
}