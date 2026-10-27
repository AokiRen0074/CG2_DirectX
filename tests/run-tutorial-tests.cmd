@echo off
setlocal
where cl >nul 2>nul
if errorlevel 1 exit /b 1
pushd "%~dp0.."
if not exist artifacts\tests mkdir artifacts\tests
cl /nologo /std:c++20 /EHsc /utf-8 /W4 /WX /I . tests\SpearTutorialTests.cpp /Fe:artifacts\tests\SpearTutorialTests.exe /Fo:artifacts\tests\SpearTutorialTests.obj
if errorlevel 1 (popd & exit /b 1)
artifacts\tests\SpearTutorialTests.exe
set "testResult=%ERRORLEVEL%"
popd
exit /b %testResult%
