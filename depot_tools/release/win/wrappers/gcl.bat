@echo off

setlocal
set PATH=%~dp0..\release\svn;%PATH%

%~dp0..\release\python_24\python.exe %~dp0..\release\gcl.py %*
