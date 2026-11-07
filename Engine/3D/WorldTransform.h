#pragma once
#include "Vector3.h"
#include "Matrix4x4.h"
#include "ViewProjection.h"
#include <d3d12.h>
#include <wrl.h>

// GPUに送るためのデータ構造
struct ConstBufferDataWorldTransform {
	Matrix4x4 WVP;
	Matrix4x4 World;
};

struct WorldTransform {
	//SRT
	Vector3 scale_ = { 1.0f,1.0f,1.0f };
	Vector3 rotation_ = { 0.0f,0.0f,0.0f };
	Vector3 translation_ = { 0.0f,0.0f,0.0f };
	
	// 計算結果のワールド行列
	Matrix4x4 matWorld_;

	// 親子関係用
	const WorldTransform* parent_ = nullptr;

	Microsoft::WRL::ComPtr<ID3D12Resource> constBuff_;
	ConstBufferDataWorldTransform* constMap_ = nullptr;

	static ID3D12Device* sDevice;

	static void SetDevice(ID3D12Device* device) {
		sDevice = device;
	}

	// 初期化
	void Initialize();


	// 行列の更新と定数バッファへの転送
	void UpdateMatrix(const ViewProjection& viewProjection);

	void TransferMatrix();

};