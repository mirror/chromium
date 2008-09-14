@echo off
REM
REM This script is called by the buildbot to run tests.
REM It is run from the v8 top-level directory. It runs
REM all unit tests in debug and release mode.
REM
tools\build -Q output=simple check
exit %ERRORLEVEL%
