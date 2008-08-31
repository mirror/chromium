@echo off

setlocal
set url="svn://chrome-svn.corp.google.com/chrome/trunk/depot_tools/development/release/win"
set opt=-q

:: we silently update the depot_tools when it already exists
IF NOT EXIST %1 GOTO message

IF EXIST %1\.svn GOTO svn

exit /b 0

:message
echo checking out latest depot_tools...

:svn
"%~dp0svn\svn.exe" co %opt% %url% %1
