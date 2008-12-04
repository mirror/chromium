@echo off
::=============================================================================
:: hammer.bat - Hammer wrapper for software construction toolkit for SCons
::
:: Environment variables used all the time:
::   HAMMER_OPTS        Command line options for hammer/SCons, in addition to
::                      any specified on the command line.
::
:: Environment variables used for IncrediBuild (Xoreax Grid Engine) support:
::   HAMMER_XGE         If set to 1, enable IncrediBuild
::   HAMMER_XGE_PATH    Path to IncrediBuild install.  Required if it was not
::                      installed in the default location
::                      (%ProgramFiles%\Xoreax\IncrediBuild).
::   HAMMER_XGE_OPTS    Additional options to pass to IncrediBuild.
::
:: Sample values for HAMMER_OPTS:
::   -j%NUMBER_OF_PROCESSORS%
::      Run parallel builds on all processor cores, unless explicitly
::      overridden on the command line.
::   -j12
::      Always run 12 builds in parallel; a good default if HAMMER_XGE=1.
::   -s -k
::      Don't print commands; keep going on build failures.

::=============================================================================
:: Hammer setup

setlocal

:: Preserve a copy of the PATH (in case we need it later, mainly for cygwin).
set PRESCONS_PATH=%PATH%

:: Find the path to the src directory.
:: Handle being run either from a module or the src directory.
IF EXIST "%cd%\third_party\python_24\python.exe" GOTO SRCDIR
set SRC_DIR=%cd%\..
goto PICKED_SITE_SCONS
:SRCDIR
set SRC_DIR=%cd%
:PICKED_SITE_SCONS

:: Specify site_scons directories.
set HAMMER_OPTS=%HAMMER_OPTS% --site-dir="%SRC_DIR%\site_scons"

:: Run SCons via software construction toolkit wrapper.
:: Remove -O and -OO from the following line to make asserts execute
set HAMMER_CMD="%SRC_DIR%\third_party\python_24\python.exe" -x -O -OO "%SRC_DIR%\third_party\scons\scons.py" %HAMMER_OPTS% %*

:: ============================================================================
:: Incredibuild support

if not defined HAMMER_XGE set HAMMER_XGE=0
if %HAMMER_XGE% neq 1 goto END_XGE_SETUP

:: Allow IncrediBuild to intercept tools run by python; run cl remotely.
set HAMMER_XGE_OPTS=%HAMMER_XGE_OPTS% /profile="%~dp0\profile.xml"

:: Add IncrediBuild back into the path (from the default location, if not
:: explicitly specified)
if not defined HAMMER_XGE_PATH (
  set HAMMER_XGE_PATH=%ProgramFiles%\Xoreax\IncrediBuild
)
if not exist "%HAMMER_XGE_PATH%\xgConsole.exe" (
  echo Warning: xgConsole.exe not found in %HAMMER_XGE_PATH%
  echo Not using IncrediBuild.
  goto END_XGE_SETUP
)
set PATH=%PATH%;%HAMMER_XGE_PATH%

:: Use IncrediBuild to wrap the call to python.
set HAMMER_CMD=XGConsole.exe %HAMMER_XGE_OPTS% /command="%HAMMER_CMD%"

:END_XGE_SETUP
:: ============================================================================
:: Run whatever command we came up with

%HAMMER_CMD%
