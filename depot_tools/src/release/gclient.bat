@echo off

:: This batch file assumes that the correct version of svn and python
:: can be found in the current directory.

setlocal

IF "%DEPOT_TOOLS_UPDATE%" == "0" GOTO gclient

IF NOT EXIST %~dp0..\.svn GOTO gclient

set PATH=%~dp0svn;%~dp0python_24;%PATH%
set opt=-q
svn %opt% up "%~dp0.."

:gclient
python.exe "%~dp0\gclient.py" %*
