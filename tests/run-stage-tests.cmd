@echo off
setlocal
rem Run from a Visual Studio x64 Native Tools Command Prompt.
where cl >nul 2>nul
if errorlevel 1 (
    echo Open an x64 Native Tools Command Prompt for Visual Studio first.
    exit /b 1
)
pushd "%~dp0.."
if not exist "artifacts\tests" mkdir "artifacts\tests"
cl /nologo /std:c++20 /EHsc /utf-8 /W3 /I . tests\SpearStageTests.cpp /Fe:artifacts\tests\SpearStageTests.exe /Fo:artifacts\tests\SpearStageTests.obj
if errorlevel 1 (
    popd
    exit /b 1
)
artifacts\tests\SpearStageTests.exe
set "testResult=%ERRORLEVEL%"
popd
exit /b %testResult%
