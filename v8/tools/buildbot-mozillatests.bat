@echo off
REM
REM This script is called by the buildbot to run mozilla tests.
REM It is run from the v8 top-level directory.
REM
tools\build -Q mode=release output=bot test=mozilla check
exit %ERRORLEVEL%
