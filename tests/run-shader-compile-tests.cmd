@echo off
setlocal
where cl >nul 2>nul
if errorlevel 1 exit /b 1
pushd "%~dp0.."
if not exist artifacts\tests\shaders mkdir artifacts\tests\shaders
cl /nologo /std:c++20 /EHsc /utf-8 /W3 tests\ShaderCompileTests.cpp dxcompiler.lib /Fe:artifacts\tests\ShaderCompileTests.exe /Fo:artifacts\tests\shaders\
if errorlevel 1 (popd & exit /b 1)
copy /y dxcompiler.dll artifacts\tests\ >nul
copy /y dxil.dll artifacts\tests\ >nul
artifacts\tests\ShaderCompileTests.exe
set "testResult=%ERRORLEVEL%"
popd
exit /b %testResult%
