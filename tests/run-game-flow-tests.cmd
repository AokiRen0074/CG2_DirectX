@echo off
setlocal
where cl >nul 2>nul
if errorlevel 1 (
    echo Open a Visual Studio x64 Native Tools Command Prompt first.
    exit /b 1
)
pushd "%~dp0.."
if not exist artifacts\tests mkdir artifacts\tests
cl /nologo /std:c++20 /EHsc /W4 /WX /I Application\Character tests\SpearGameFlowTests.cpp /Fe:artifacts\tests\SpearGameFlowTests.exe /Fo:artifacts\tests\SpearGameFlowTests.obj
if errorlevel 1 (
    popd
    exit /b 1
)
artifacts\tests\SpearGameFlowTests.exe
set "testResult=%ERRORLEVEL%"
popd
exit /b %testResult%
