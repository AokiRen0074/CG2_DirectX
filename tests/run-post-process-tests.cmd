@echo off
setlocal
where cl >nul 2>nul
if errorlevel 1 exit /b 1
pushd "%~dp0.."
if not exist artifacts\tests\postprocess mkdir artifacts\tests\postprocess
cl /nologo /std:c++20 /EHsc /utf-8 /W3 /WX /MD /DUNICODE /D_UNICODE /I . /I Engine /I Engine\Base /I Engine\2D /I Engine\Math /I externals\DirectXTex tests\PostProcessTests.cpp Engine\PostProcess\Bloom.cpp Engine\PostProcess\FullscreenPass.cpp Engine\PostProcess\ScenePostProcess.cpp Engine\Base\DirectXCommon.cpp Engine\Base\WindowApp.cpp Engine\Base\Logger.cpp Engine\2D\TextureManager.cpp externals\DirectXTex\Bin\Desktop_2022_Win10\x64\Release\DirectXTex.lib /Fe:artifacts\tests\PostProcessTests.exe /Fo:artifacts\tests\postprocess\ /link user32.lib ole32.lib windowscodecs.lib dxguid.lib
if errorlevel 1 (popd & exit /b 1)
copy /y dxcompiler.dll artifacts\tests\ >nul
copy /y dxil.dll artifacts\tests\ >nul
artifacts\tests\PostProcessTests.exe
set "testResult=%ERRORLEVEL%"
popd
exit /b %testResult%
