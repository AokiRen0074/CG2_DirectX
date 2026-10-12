# DirectX Shader Compiler runtime

The x64 dxcompiler.dll and dxil.dll shipped with this project are stored in bin/x64.
MSBuild copies them beside the executable after each successful build.
Headers and the dxcompiler import library come from the installed Windows SDK.
Upstream project and license: https://github.com/microsoft/DirectXShaderCompiler