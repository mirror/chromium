@echo off

:: This batch file assumes that the correct version of python can be found in
:: the current directory, and that you have Visual Studio 8 installed in the
:: default location.

setlocal
set PATH=%~dp0\..;%WINDIR%\system32;%PROGRAMFILES%\Microsoft Visual Studio 8\Common7\IDE

"%~dp0\python_24\python.exe" "%~dp0\chrome-update.py" %*
