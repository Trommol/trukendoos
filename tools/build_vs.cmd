@echo off
setlocal

set Path=

call "C:\BuildTools\Common7\Tools\VsDevCmd.bat" -arch=x64
if errorlevel 1 exit /b %errorlevel%

"C:\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" -S . -B build-vs -G "Visual Studio 17 2022" -A x64
if errorlevel 1 exit /b %errorlevel%

"C:\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" --build build-vs --config Release
if errorlevel 1 exit /b %errorlevel%

