#pragma once
#include "Rail.h"
#include "RailCamera.h" 
#include "DebugCamera.h"

class RailEditor {
public:
    void Initialize(Rail* rail);
    void Update(DebugCamera* dCamera, RailCamera* rCamera);


private:
    Rail* rail_ = nullptr;
    int selectedIndex_ = -1; 
};