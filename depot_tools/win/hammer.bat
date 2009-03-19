@echo off

IF EXIST "%cd%\third_party\python_24\python.exe" GOTO srcdir

@rem We're in a submodule directory, look relative to the parent.
"%cd%\..\third_party\python_24\python.exe" "%cd%\..\third_party\scons\scons.py" "--site-dir=..\site_scons" %*
goto omega

:srcdir
"%cd%\third_party\python_24\python.exe" "%cd%\third_party\scons\scons.py" --site-dir=site_scons %*
goto omega

@rem Per the following page:
@rem   http://code-bear.com/bearlog/2007/06/01/getting-the-exit-code-from-a-batch-file-that-is-run-from-a-python-program/
@rem Just calling "exit /b" passes back an exit code, but in a way
@rem that does NOT get picked up correctly when executing the .bat
@rem file from the Python subprocess module.  Using "call" as the
@rem last command in the .bat file makes it work as expected.

:returncode
exit /b %ERRORLEVEL%

:omega
call :returncode %ERRORLEVEL%
