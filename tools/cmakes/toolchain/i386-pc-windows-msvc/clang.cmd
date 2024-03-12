@echo off
setlocal

set VCToolsInstallDir=%REPO_APP_PATH_microsoft.windows.msvc.bins%

set REPO_APP_CACHED_PATH=""
set Path=%Path%;%REPO_APP_PATH_microsoft.windows.msvc.bins%\bin\x86
set Path=%Path%;%REPO_APP_PATH_llvm.win64%\bin

set Include=%Include%;%REPO_APP_PATH_microsoft.windows.msvc.bins%\Include
set Include=%Include%;%REPO_APP_PATH_microsoft.windows.sdk.cpp%\c\Include\10.0.19041.0\cppwinrt
set Include=%Include%;%REPO_APP_PATH_microsoft.windows.sdk.cpp%\c\Include\10.0.19041.0\shared
set Include=%Include%;%REPO_APP_PATH_microsoft.windows.sdk.cpp%\c\Include\10.0.19041.0\ucrt
set Include=%Include%;%REPO_APP_PATH_microsoft.windows.sdk.cpp%\c\Include\10.0.19041.0\um
set Include=%Include%;%REPO_APP_PATH_microsoft.windows.sdk.cpp%\c\Include\10.0.19041.0\winrt

set Lib=%lib%;%REPO_APP_PATH_microsoft.windows.msvc.bins%\lib\x86
set Lib=%lib%;%REPO_APP_PATH_microsoft.windows.sdk.cpp.x86%\c\um\x86
set Lib=%lib%;%REPO_APP_PATH_microsoft.windows.sdk.cpp.x86%\c\ucrt\x86

%REPO_APP_PATH_llvm.win64%/bin/clang.exe %*