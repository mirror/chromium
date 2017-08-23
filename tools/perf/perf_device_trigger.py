#!/usr/bin/env python
"""Special perf swarming device triggering script.

This script does custom swarming triggering logic, to enable device affinity
for our bots, while lumping all trigger calls under one buildbot step.

This script must have roughly the same command line interface as swarming.py
trigger. This is a sample swarming.py command line invocation;

python
swarming.py
trigger
--swarming
https://chromium-swarm.appspot.com
--isolate-server
https://isolateserver.appspot.com
--priority
25
--shards
1
--task-name
blink_perf.bindings/Android/f48fbb5c45/Android Nexus5X Perf/442
--dump-json
/tmp/tmprKbXxa.json
--expiration
72000
--io-timeout
600
--hard-timeout
10800
--dimension
android_devices
1
--dimension
os
Android
--dimension
pool
Chrome-perf
--tag
buildername:Android Nexus5X Perf
--tag
buildnumber:442
--tag
data:f48fbb5c45050b2c15d3a93db6e6ff366a96f7c2
--tag
master:chromium.perf
--tag
name:blink_perf.bindings
--tag
os:Android
--tag
project:chromium
--tag
purpose:CI
--tag
purpose:post-commit
--tag
slavename:slave208-c1
--tag
stepname:blink_perf.bindings on Android
--isolated
f48fbb5c45050b2c15d3a93db6e6ff366a96f7c2
--
blink_perf.bindings
-v
--upload-results
--output-format=chartjson
--browser=android-chromium
--isolated-script-test-output=${ISOLATED_OUTDIR}/output.json
--isolated-script-test-chartjson-output=${ISOLATED_OUTDIR}/chartjson-output.json
"""

import argparse
import subprocess
import sys

def subprocess_call(script, args):
  print 'RUN SCRIPT', script
  print 'WITH ARGS', ' '.join(args)
  return 0
  return subprocess.call(sys.executable, script, args)

def add_bot_id_to_args(all_args, bot_id):
  assert '--' in all_args, (
      'Malformed trigger command; -- argument expected but not found')
  dash_ind = all_args.index('--')

  return all_args[:dash_ind] + [
      '--dimension', 'id', bot_id] + all_args[dash_ind:]

def main():
  parser = argparse.ArgumentParser()
  parser.add_argument('--bot-id', action='append', required=True)
  parser.add_argument('--swarming-py-path', required=True)
  args, remaining = parser.parse_known_args(sys.argv)
  # Remove script as part of args
  remaining = remaining[1:]

  for bot_id in args.bot_id:
    args_to_pass = add_bot_id_to_args(remaining[:], bot_id)
    subprocess_call(args.swarming_py_path, args_to_pass)
  return 0

if __name__ == '__main__':
  sys.exit(main())
