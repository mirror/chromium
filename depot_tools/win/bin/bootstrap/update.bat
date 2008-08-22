@echo off

setlocal
set url="svn://chrome-svn.corp.google.com/chrome/trunk/depot_tools/release/win"
set opt=-q

:: we silently update the depot_tools when it already exists
IF NOT EXIST %1. (
  echo checking out latest depot_tools...
)

%~dp0svn\svn.exe co %opt% %url% %1
