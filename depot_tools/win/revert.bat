@echo off

setlocal
set PATH=%~dp0release\svn;%PATH%

"%~dp0release\python_24\python.exe" "%~dp0release\revert.py" %*
