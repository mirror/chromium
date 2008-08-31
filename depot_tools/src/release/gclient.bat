@echo off

:: This batch file assumes that the correct version of svn and python can be
:: found in the parent directory.

setlocal
set PATH=%~dp0svn;%WINDIR%\system32
set opt=-q

IF NOT EXIST %dp0..\.svn GOTO gclient
svn %opt% up "%~dp0.."

:gclient
"%~dp0python_24\python.exe" "%~dp0\gclient.py" %*
