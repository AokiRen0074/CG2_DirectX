#include <Windows.h>
#include <dxcapi.h>
#include <wrl.h>
#include <iostream>
#include <stdexcept>
using Microsoft::WRL::ComPtr;
void Check(HRESULT result) { if (FAILED(result)) throw std::runtime_error("DXC operation failed"); }
int main() {
    try {
        ComPtr<IDxcUtils> utils;
        ComPtr<IDxcCompiler3> compiler;
        Check(DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&utils)));
        Check(DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&compiler)));
        ComPtr<IDxcIncludeHandler> includes;
        Check(utils->CreateDefaultIncludeHandler(&includes));
        struct Shader { const wchar_t* name; const wchar_t* profile; };
        const Shader shaders[] = {
            {L"Object3d.VS.hlsl", L"vs_6_0"}, {L"Object3d.PS.hlsl", L"ps_6_0"},
            {L"Object3d.NoTexture.VS.hlsl", L"vs_6_0"}, {L"Object3d.Notexture.PS.hlsl", L"ps_6_0"},
            {L"Sprite.PS.hlsl", L"ps_6_0"},
            {L"Particle.VS.hlsl", L"vs_6_0"}, {L"Particle.PS.hlsl", L"ps_6_0"},
            {L"Bloom.hlsl", L"cs_6_0"}, {L"EmissionBlur.hlsl", L"cs_6_0"},
            {L"PostProcess.VS.HLSL", L"vs_6_0"}, {L"PostProcess.PS.hlsl", L"ps_6_0"}
        };
        for (const auto& shader : shaders) {
            const auto path = std::wstring(L"Resources/Shaders/") + shader.name;
            ComPtr<IDxcBlobEncoding> source;
            Check(utils->LoadFile(path.c_str(), nullptr, &source));
            DxcBuffer buffer{source->GetBufferPointer(), source->GetBufferSize(), DXC_CP_UTF8};
            const wchar_t* args[] = {path.c_str(), L"-E", L"main", L"-T", shader.profile, L"-I", L"Resources/Shaders", L"-WX"};
            ComPtr<IDxcResult> result;
            Check(compiler->Compile(&buffer, args, _countof(args), includes.Get(), IID_PPV_ARGS(&result)));
            HRESULT status;
            Check(result->GetStatus(&status));
            if (FAILED(status)) {
                ComPtr<IDxcBlobUtf8> errors;
                Check(result->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&errors), nullptr));
                std::cerr << errors->GetStringPointer();
                return 1;
            }
        }
        std::cout << "PASS: all 11 engine shader entry points.\n";
    } catch (const std::exception& e) { std::cerr << e.what(); return 1; }
}
