@echo off

setlocal

:: we silently update the depot_tools when it already exists
IF NOT EXIST %1 GOTO message

IF EXIST %1\.svn GOTO svn
exit /b 0

:message
echo checking out latest depot_tools...

set url="http://src.chromium.org/svn/trunk/depot_tools/release/win"
set opt=-q

:svn
"%~dp0svn\svn.exe" co %opt% %url% %1
