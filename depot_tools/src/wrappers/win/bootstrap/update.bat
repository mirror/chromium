@echo off

setlocal ENABLEEXTENSIONS ENABLEDELAYEDEXPANSION

:: We silently update the depot_tools when it already exists.
IF NOT EXIST %1 GOTO message

IF EXIST %1\.svn GOTO svn
:: If not versioned, we can't update.
exit /b 0

:message
echo Checking out latest depot_tools...

:svn
:: Retrieve the root url
"%~dp0svn\svn.exe" info "%~dp0.." > "%~dp0root.txt"
set url=
for /F "usebackq tokens=1,2" %%a in ("%~dp0root.txt") do if "%%a" == "URL:" set url=%%b
del "%~dp0root.txt"
if "%url%"=="" set url="$DOWNLOAD_URL/$RELEASE_ARCH"

:: Massage the url.
set url=%url:~,-3%release/win
set opt=-q

"%~dp0svn\svn.exe" co %opt% %url% %1
