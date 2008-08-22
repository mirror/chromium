@echo off

:: This batch file assumes that the correct version of svn and python can be
:: found in the current directory.

setlocal
set PATH=%~dp0\svn;%WINDIR%\system32

xcopy %~dp0..\wrappers\*.* %~dp0..\..\bin /e /c /d /y /q > nul

%~dp0..\python_24\python.exe %~dp0\gclient.py %*
