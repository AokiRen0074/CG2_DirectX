@echo off
setlocal
where cl >nul 2>nul
if errorlevel 1 exit /b 1
pushd "%~dp0.."
if not exist artifacts\tests\utilities mkdir artifacts\tests\utilities
cl /nologo /std:c++20 /EHsc /utf-8 /W3 /I Engine /I Engine\Math tests\EngineUtilityTests.cpp Engine\Math\TimedCall.cpp Engine\Math\MathUtility.cpp Engine\Math\Vector3.cpp Engine\Collision\CollisionManager.cpp /Fe:artifacts\tests\EngineUtilityTests.exe /Fo:artifacts\tests\utilities\
if errorlevel 1 (popd & exit /b 1)
artifacts\tests\EngineUtilityTests.exe
set "testResult=%ERRORLEVEL%"
popd
exit /b %testResult%
