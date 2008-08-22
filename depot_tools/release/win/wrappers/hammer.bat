@echo off

setlocal
set PATH=%~dp0..\release\svn;%PATH%

%~dp0..\third_party\python_24\python.exe %~dp0..\third_party\scons\scons.py %*
