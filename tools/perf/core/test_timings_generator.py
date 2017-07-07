#!/usr/bin/env python
# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Script to generate chromium.perf.json and chromium.perf.fyi.json in
the src/testing/buildbot directory and benchmark.csv in the src/tools/perf
directory. Maintaining these files by hand is too unwieldy.
"""
import argparse
import collections
import json
import os
import multiprocessing
import re
import sys
import subprocess

from core import path_util
path_util.AddTelemetryToPath()

from telemetry.util import statistics


def get_story_times_path():
  # Format is {
  #   platform: {
  #     benchmark: {
  #       story: time in seconds (float)
  #     }
  #   }
  # }
  return os.path.join(
      path_util.GetChromiumSrcDir(), 'tools', 'perf', 'core',
      'story_avg_times.json')


def swarming_py_path():
  return os.path.join(
      path_util.GetChromiumSrcDir(), 'tools', 'swarming_client', 'swarming.py')


class SwarmingResult(object):
  def __init__(self, raw):
    self._raw = raw

  @property
  def task_id(self):
    return self._raw['task_id']

  @property
  def tags(self):
    return {
        tag.split(':')[0]: tag.split(':', 2)[1]
        for tag in self._raw['tags']
    }

  @property
  def state(self):
    return self._raw['state']

  @property
  def benchmark_name(self):
    return self._raw['name'].split('/')[0].split(' ')[0]

  def __repr__(self):
    return 'SwarmingResult(%s)' % self.task_id

def _query_swarming(args, limit, fname):
  def inner():
    if os.path.exists(fname):
      try:
        with open(fname) as f:
          return json.load(f)
      except ValueError:
        print 'exception while reading', fname
        raise

    args_l = []
    for k, v in args.items():
      if k != 'tags':
        args_l.append('='.join((str(k), str(v))))
      else:
        for inner in v.items():
          args_l.append('tags=' + ':'.join(inner))

    cmd_args = [
      'python',
      swarming_py_path(),
      'query',
      '--swarming=https://chromium-swarm.appspot.com',
      '--json=%s' % fname,
      '--limit=%s' % limit,
      'tasks/list?%s' % '&'.join((
          x for x in args_l)),
    ]
    # print(' '.join(cmd_args))

    subprocess.call(cmd_args)

    # process data to remove meddlers :P
    with open(fname) as f:
      data = json.load(f)

    real = []
    dumpit = False
    for x in data.get('items', []):
      if x['user']:
        dumpit = True
        continue

      real.append(x)
    data['items'] = real

    if dumpit:
      with open(fname, 'w') as f:
        json.dump(data, f)

    return data
  return [SwarmingResult(d) for d in inner()['items']]


def query_swarming(id, limit, fname, last_ts=None):
  args = {
      'tags': {
          'id': id,
          'pool': 'Chrome-perf',
      },
      'include_performance_stats': True,
  }
  return _query_swarming(args, limit, fname)

CACHED_DATA_DIR = os.path.expanduser("~/perf_swarming_data_cache")

def get_builder_build_data(builder, build=None, refresh=False):
  args = {
      'tags': {
          'buildername': builder,
          'pool': 'Chrome-perf',
      },
      'include_performance_stats': True,
  }
  if build:
    args['tags']['buildnumber'] = str(build)

  fname = '%s/bdata_%s_%s.json' % (
      CACHED_DATA_DIR, builder.replace(' ', '_'), build if build else 'current')
  if refresh:
    if os.path.exists(fname):
      os.unlink(fname)
  limit = 450

  return _query_swarming(args, limit, fname)

def get_builder_data(builder, num_runs, refresh):
  start = get_builder_build_data(builder, refresh=refresh)
  bnum = None
  for item in start:
    new_bnum = item.tags.get('buildnumber')
    if not new_bnum:
      continue

    bnum = int(new_bnum)
    break
  if any(task.state == 'PENDING' for task in start
         if task.tags.get('buildnumber') == str(bnum)):
    bnum -= 1

  data = {}
  while num_runs:
    if bnum < 3 and 'Nexus5X' in builder:
      break
    print('bnum %d for %s' % (bnum, builder))
    per_bnum = get_builder_build_data(builder, bnum)
    data[bnum] = per_bnum
    num_runs -= 1
    bnum -= 1

  return data

MATCH_RE = re.compile(r'\[.*OK \] (.*) \(([0-9]+) ms\)')

def get_task_stdout(task_id):
  args = [
    'python',
    swarming_py_path(),
    'collect',
    '-S', 'chromium-swarm.appspot.com',
    '--task-output-dir=foo',
    task_id,
  ]
  p = subprocess.Popen(args, stdout=subprocess.PIPE)
  out, _ = p.communicate()
  return out.splitlines()

def get_story_timings(task_id):
  fname = '%s/stimes_%s.json' % (CACHED_DATA_DIR, task_id)
  if os.path.exists(fname):
    with open(fname) as f:
      return json.load(f)

  timings = {}
  out = get_task_stdout(task_id)
  for line in out:
    line = line.decode('utf-8').strip()
    res = MATCH_RE.search(line)
    if res:
      # These are temp things, ignore them
      url = res.group(1)
      if url.startswith('c:\\b\\s\\w') or url.startswith(
          '/b/s/w') or url.startswith('/b/swarming/w'):
        continue
      timings[url] = float(res.group(2)) / 1000

  with open(fname, 'w') as f:
    json.dump(timings, f)

  return timings


def _story_timings(name):
  name, task_id = name
  if 'reference' in name:
    return None, None
  timings = get_story_timings(task_id)
  return timings, name

def _bd(builder):
  builder, num = builder
  return get_builder_data(builder, num, False)

def refresh_times(configs, platforms, args):
  with open(get_story_times_path()) as f:
    times = json.load(f)

  if not platforms:
    platforms = list(times.keys())

  bot_per_platform = collections.defaultdict(list)
  for bot, cfg in configs['chromium.perf']['testers'].items():
    bot_per_platform[cfg['platform']].append(bot)

  all_builders = [
      builder for lst in bot_per_platform.values() for builder in lst]

  if True:
    p = multiprocessing.Pool(None)
    map_func = p.map
  else:
    p = None
    map_func = map

  try:
    map_func(_bd, [(builder, 3) for builder in all_builders])
  finally:
    if p:
      p.close()

  print '=' * 80
  print 'done caching builder data'
  print '=' * 80

  new_times = {}
  for plat, lst in bot_per_platform.items():
    new_times[plat] = {}

    for builder in lst:
      new_times[plat][builder] = {}

      # benchmark -> story -> list of times
      benchmark_times = collections.defaultdict(
          lambda: collections.defaultdict(list))

      data = get_builder_data(builder, 3, False)

      if True:
        p = multiprocessing.Pool(None)
        map_func = p.map
      else:
        p = None
        map_func = map
      try:
        for bnum, tasks in data.items():
          print 'bnum %d stdout' % bnum
          simple = [(t.benchmark_name, t.task_id) for t in tasks
                    if 'reference' not in t.benchmark_name]
          res = map_func(_story_timings, simple)
          for timings, name in res:
            if not name:
              continue

            for test, time in timings.items():
              benchmark_times[name][test].append(time)
      finally:
        if p:
          p.close()

      for benchmark_name, stories in benchmark_times.items():
        new_times[plat][builder][benchmark_name] = story_times = {}
        for story_name, times in stories.items():
          story_times[story_name] = statistics.Median(times)


  with open(get_story_times_path(), 'w') as f:
    dump_json(new_times, f)


def get_args():
  parser = argparse.ArgumentParser(
      description=('Regenerate perf test timings'))

  parser.add_argument('--platforms', action='append', default=None)
  parser.add_argument('--buildnum-offset')
  return parser


def dump_json(data, f):
  """Utility method to dump json which is indented, sorted, and readable"""
  return json.dump(data, f, indent=2, sort_keys=True, separators=(',', ': '))

def dumps_json(data):
  """Utility method to dump json which is indented, sorted, and readable"""
  return json.dumps(data, indent=2, sort_keys=True, separators=(',', ': '))

def main(args, benchmarks, configs):
  return refresh_times(configs, args.platforms, args)
