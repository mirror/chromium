@echo off

setlocal
set url="https://svn/chrome/branches/depot_tools_release_branch"
set opt=-q

:: we silently update the depot_tools when it already exists
IF NOT EXIST %1. (
  echo checking out latest depot_tools...
)

%~dp0svn\svn.exe co %opt% %url% %1
