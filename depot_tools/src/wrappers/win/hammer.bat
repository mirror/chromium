@echo off

IF EXIST "%cd%\third_party\python_24\python.exe" GOTO srcdir

@rem We're in a submodule directory, look relative to the parent.
"%cd%\..\third_party\python_24\python.exe" "%cd%\..\third_party\scons\scons.py" %*
exit /b %ERRORLEVEL%

:srcdir
"%cd%\third_party\python_24\python.exe" "%cd%\third_party\scons\scons.py" %*
