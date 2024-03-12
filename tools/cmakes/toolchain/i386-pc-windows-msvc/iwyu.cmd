@echo off
setlocal

set Include=%Include%;%REPO_APP_PATH_microsoft.windows.msvc.bins%\Include
set Include=%Include%;%REPO_APP_PATH_microsoft.windows.sdk.cpp%\c\Include\10.0.19041.0\cppwinrt
set Include=%Include%;%REPO_APP_PATH_microsoft.windows.sdk.cpp%\c\Include\10.0.19041.0\shared
set Include=%Include%;%REPO_APP_PATH_microsoft.windows.sdk.cpp%\c\Include\10.0.19041.0\ucrt
set Include=%Include%;%REPO_APP_PATH_microsoft.windows.sdk.cpp%\c\Include\10.0.19041.0\um
set Include=%Include%;%REPO_APP_PATH_microsoft.windows.sdk.cpp%\c\Include\10.0.19041.0\winrt

%REPO_APP_PATH_include-what-you-use%/include-what-you-use.exe %*
exit %errorlevel%
