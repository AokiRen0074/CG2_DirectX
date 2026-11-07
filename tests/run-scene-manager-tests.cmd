@echo off
setlocal
where cl >nul 2>nul
if errorlevel 1 (
    echo Open a Visual Studio x64 Native Tools Command Prompt first.
    exit /b 1
)
pushd "%~dp0.."
if not exist artifacts\tests mkdir artifacts\tests
cl /nologo /std:c++20 /EHsc /utf-8 /W4 /WX /I Engine tests\SceneManagerTests.cpp Engine\Scene\SceneManager.cpp /Fe:artifacts\tests\SceneManagerTests.exe /Fo:artifacts\tests\
if errorlevel 1 (
    popd
    exit /b 1
)
artifacts\tests\SceneManagerTests.exe
set "testResult=%ERRORLEVEL%"
popd
exit /b %testResult%