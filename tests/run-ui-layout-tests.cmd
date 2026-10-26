@echo off
setlocal
where cl >nul 2>nul
if errorlevel 1 exit /b 1
pushd "%~dp0.."
if not exist artifacts\tests mkdir artifacts\tests
cl /nologo /std:c++20 /EHsc /W4 /WX /I . /I Application\Character tests\SpearUiLayoutTests.cpp /Fe:artifacts\tests\SpearUiLayoutTests.exe /Fo:artifacts\tests\SpearUiLayoutTests.obj
if errorlevel 1 (popd & exit /b 1)
artifacts\tests\SpearUiLayoutTests.exe Resources\UI\PrismUI\layout.json
set "testResult=%ERRORLEVEL%"
popd
exit /b %testResult%
