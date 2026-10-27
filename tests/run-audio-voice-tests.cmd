@echo off
setlocal
where cl >nul 2>nul
if errorlevel 1 exit /b 1
pushd "%~dp0.."
if not exist artifacts\tests\audio-voice mkdir artifacts\tests\audio-voice
cl /nologo /std:c++20 /EHsc /utf-8 /W3 /I . tests\AudioVoiceTests.cpp Engine\Audio\Audio.cpp /Fe:artifacts\tests\AudioVoiceTests.exe /Fo:artifacts\tests\audio-voice\
if errorlevel 1 (popd & exit /b 1)
artifacts\tests\AudioVoiceTests.exe
set "testResult=%ERRORLEVEL%"
popd
exit /b %testResult%
