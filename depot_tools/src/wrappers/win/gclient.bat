@echo off

IF "%DEPOT_TOOLS_UPDATE%" == "0" GOTO gclient

IF NOT EXIST %~dp0.svn GOTO gclient

:: Update the "release" subdirectory to stay
:: up-to-date with the latest depot_tools release.
call "%~dp0bootstrap\update.bat" "%~dp0release"

:gclient
"%~dp0release\gclient.bat" %*
