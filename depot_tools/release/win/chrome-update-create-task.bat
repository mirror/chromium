@echo off
setlocal

:: This script will create a scheduled task to run chrome-update every day
:: at the time you specify.  This script expects to be live in
:: depot_tools\latest.
::
:: Usage: this-script <time to run task> <path to chrome trunk>

set Out=%USERPROFILE%\chrome-update-task.bat
set TaskTime=%1
set Trunk=%~f2

if not exist "%Trunk%" (
  echo Usage: %~n0 ^<time^> ^<c:\path\to\chrome\trunk^>
  echo ^<time^> is the time in HH:MM:SS format at which to run the task.
  echo Example: %~n0 02:00:00 c:\src\chrome\trunk
  exit 1
)

if not exist "%Out%" goto CreateScript

echo WARNING: %Out% already exists.
set Choice=
set /P Choice=Overwrite file [Y/N]?
if not "%Choice%"=="y" goto CreateTask

:CreateScript

echo.
echo Creating %Out%

echo>"%Out%" @echo off
echo>>"%Out%" call "%~dp0\..\bootstrap\update.bat" "%~dp0\..\latest"
echo>>"%Out%" "%~dp0\chrome-update.bat" "%Trunk%" ^> "%Trunk%\chrome-update-results.txt" 

:CreateTask

echo.
echo ***********************************************************************
echo Creating a Scheduled Task to run chrome-update each day at %TaskTime%.
echo The batch file being run will live at %Out%.
echo.
echo WARNING: The password you enter will be displayed in cleartext.
echo If you're paranoid, you can enter blank here and then fix the password
echo by editing the scheduled task manually from the Control Panel.
echo ***********************************************************************
echo.
schtasks /create /tn chrome-update /tr "\"%Out%\"" /sc daily /st %TaskTime%
