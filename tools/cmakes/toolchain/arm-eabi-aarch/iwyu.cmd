@echo off
setlocal

%REPO_APP_PATH_include-what-you-use%/include-what-you-use.exe %*
exit %errorlevel%
