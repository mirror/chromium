@echo off

:: Update "..\release" every time this script is run to stay up-to-date
:: with the latest depot_tools release.
call %~dp0bootstrap\update.bat %~dp0..\release

%~dp0..\release\bin\gclient.bat %*
