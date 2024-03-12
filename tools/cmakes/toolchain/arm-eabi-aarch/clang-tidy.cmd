@echo off
setlocal

%REPO_APP_PATH_llvm.win64%/bin/clang-tidy.exe %* -isystem %REPO_APP_PATH_gcc.arm.eabi.aarch-win64%\arm-none-eabi\include
exit %errorlevel%