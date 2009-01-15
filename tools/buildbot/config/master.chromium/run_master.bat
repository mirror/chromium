@echo off
setlocal
title Chromium Master
set PYTHONPATH=..\scripts;..\scripts\master;..\scripts\common;..\scripts\private;..\pylibs
set PATH=%~dp0..\depot_tools\release\python_24\;%PATH%
python ..\scripts\common\twistd --no_save -y buildbot.tac
