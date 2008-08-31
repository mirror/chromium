@echo off

:: Update the "release" subdirectory every time this script is run to stay
:: up-to-date with the latest depot_tools release.
call "%~dp0bootstrap\update.bat" "%~dp0release"

"%~dp0release\gclient.bat" %*
