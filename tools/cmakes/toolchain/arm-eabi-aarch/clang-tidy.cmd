@echo off
setlocal

powershell.exe -file %REPO_APP_ROOT%/tools/cmakes/toolchain/arm-eabi-aarch/clang-tidy.ps1 %*
exit %errorlevel%