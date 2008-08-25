@echo off

:: This batch file assumes that the correct version of svn and python can be
:: found in the parent directory.

setlocal
set PATH=%~dp0svn;%WINDIR%\system32
set opt=-q

svn %opt% up %~dp0..

%~dp0..\python_24\python.exe %~dp0\gclient.py %*
