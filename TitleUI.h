#pragma once
#include "NeonText.h"
#include "NeonModel.h" 
#include "WorldTransform.h" 
#include "ViewProjection.h"
#include <string>

class TitleUI {
public:
	~TitleUI(); 
	void Initialize(DirectXCommon* dxCommon);
	void Update(float deltaTime);
	void Draw();
	void DrawImGui();

	void SetActive(bool isActive) { isActive_ = isActive; }
	bool IsActive() const { return isActive_; }

	void PlayStartAnimation() { isStarting_ = true; startTimer_ = 0.0f; }
	void Reset() { isStarting_ = false; startTimer_ = 0.0f; }

private:
	void RebuildText();

	char promptString_[64] = "PUSH SPACE TO START";

	bool isActive_ = false;
	float timer_ = 0.0f;

	bool isStarting_ = false;
	float startTimer_ = 0.0f;

	DirectXCommon* dxCommon_ = nullptr;


	NeonModel* titleModel_ = nullptr;
	WorldTransform titleTransform_;
	uint32_t whiteTex_ = 0u;

	NeonText* promptText_ = nullptr;
	ViewProjection uiViewProjection_;

	// ImGui parameters
	float titleColor_[3] = { 0.0f, 1.0f, 1.0f };
	float titleIntensity_ = 5.0f;
	float titleOffsetX_ = 0.0f;  
	float titleOffsetY_ = 2.0f;   
	float titleOffsetZ_ = 0.0f;  
	float titleScale_ = 1.0f;  
	float titleRotX_ = 0.0f;    
	float titleRotY_ = 0.0f;      

	float promptColor_[3] = { 1.0f, 1.0f, 1.0f };
	float promptIntensity_ = 3.0f;
	float promptOffsetX_ = -7.5f;
	float promptOffsetY_ = -3.0f;
	float promptScale_ = 0.8f;

	float textRadius_ = 0.01f;
	float textSoftness_ = 5.0f;
	float textLengthOffset_ = 0.0f;
};