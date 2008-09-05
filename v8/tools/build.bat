@echo off
REM 
REM This is a simple win32 wrapper for executing build.py.
REM
%~p0..\..\third_party\python_24\python.exe %~p0build.py %*
