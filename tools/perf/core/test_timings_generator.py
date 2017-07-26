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
  def duration(self):
    if 'duration' not in self._raw:
      import pdb; pdb.set_trace()
    return self._raw['duration']

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
    return 'SwarmingResult(%s)[name=%s]' % (self.task_id, self.benchmark_name)

def _query_swarming(args, limit, fname):
  """Queries swarming for task data.

  Args:
    args: Arguments to pass to the RPC to swarming.
    limit: The number of results to retrieve from swarming.
    fname: The file where this data should be downloaded to on disk. If this
           file is present, it will be read and returned, and no RPC will be
           made.
  """
  if not os.path.exists(fname):
    args_list = []
    for k, v in args.items():
      if k == 'tags':
        for inner in v.items():
          args_list.append('tags=' + ':'.join(inner))
      else:
        args_list.append('='.join((str(k), str(v))))

    cmd_args = [
      'python',
      swarming_py_path(),
      'query',
      '--swarming=https://chromium-swarm.appspot.com',
      '--json=%s' % fname,
      '--limit=%s' % limit,
      'tasks/list?%s' % '&'.join((
          arg for arg in args_list)),
    ]

    subprocess.call(cmd_args)

  with open(fname) as f:
    return json.load(f)


def query_swarming(args, limit, fname):
  """Wrapper around _query_swarming.

  Wraps the results from _query_swarming with a SwarmingResult object.
  """
  return [
      SwarmingResult(d) for d in _query_swarming(
          args, limit, fname).get('items', [])]

CACHED_DATA_DIR = os.path.expanduser("~/perf_swarming_data_cache")

def get_builder_build_data(builder, build=None, refresh=False):
  """Gets swarming data for a particular build on a particular builder

  If no build is specified, it attempts to guess the latest build.
  If refresh is true, it refreshes the build data.
  """
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
    if not build:
      print 'refreshing current build for', builder
    if os.path.exists(fname):
      os.unlink(fname)

  return query_swarming(args, 450, fname)

def get_builder_data(builder, num_runs, refresh):
  """Gets swarming data for a builder.

  Args:
    builder: The name of the builder to get data for.
    num_runs: Number of runs to return.
    refresh: If the current build number should be refreshed.
  """
  start = get_builder_build_data(builder, refresh=refresh)
  bnum = None
  for item in start:
    new_bnum = item.tags.get('buildnumber')
    if not new_bnum:
      continue

    bnum = int(new_bnum)
    break
  lst = [task.state == 'PENDING' or task.state == 'RUNNING' for task in start
         if task.tags.get('buildnumber') == str(bnum)]
  if any(lst):
    bnum -= 1

  data = {}
  while num_runs:
    per_bnum = get_builder_build_data(builder, bnum)
    data[bnum] = per_bnum
    num_runs -= 1
    bnum -= 1

  return data

MATCH_RE = re.compile(r'\[.*OK \] (.*) \(([0-9]+) ms\)')
TASK_OUTPUT_DIR = 'task_output'

def get_task_stdout(task_id):
  """Gets the stdout of a task from swarming"""
  args = [
    'python',
    swarming_py_path(),
    'collect',
    '-S', 'chromium-swarm.appspot.com',
    '--task-output-dir=%s' % TASK_OUTPUT_DIR,
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

  timings = collections.defaultdict(list)
  out = get_task_stdout(task_id)
  for line in out:
    line = line.decode('utf-8').strip()
    res = MATCH_RE.search(line)
    if res:
      url = res.group(1)
      if url.startswith('c:\\b\\s\\w') or url.startswith(
          '/b/s/w') or url.startswith('/b/swarming/w'):
        # Ignore trace processing times
        print 'HI', task_id
        continue
      timings[url].append(float(res.group(2)) / 1000)

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
  builder, num, refresh = builder
  return get_builder_data(builder, num, refresh)

def refresh_times(configs, platforms, builders, refresh, parallel):
  with open(get_story_times_path()) as f:
    times = json.load(f)

  if not platforms:
    platforms = list(times.keys())
    platforms = ['win', 'mac', 'linux', 'android']

  bot_per_platform = collections.defaultdict(list)
  for bot, cfg in configs['chromium.perf']['testers'].items():
    if builders and bot not in builders:
      continue
    bot_per_platform[cfg['platform']].append(bot)

  all_builders = [
      builder for lst in bot_per_platform.values() for builder in lst]

  if parallel:
    p = multiprocessing.Pool(None)
    map_func = p.map
  else:
    p = None
    map_func = map

  print 'caching builder data'
  try:
    map_func(_bd, [(builder, 5, refresh) for builder in all_builders])
  finally:
    if p:
      p.close()

  print '=' * 80
  print 'done caching builder data'
  print '=' * 80

  new_times = {}
  for plat, lst in bot_per_platform.items():
    new_times[plat] = per_platform = {}

    for builder in lst:
      print builder
      per_platform[builder] = per_builder = {}

      # benchmark -> story -> list of times
      benchmark_times = collections.defaultdict(
          lambda: collections.defaultdict(list))
      benchmark_overall_times = collections.defaultdict(list)

      data = get_builder_data(builder, 5, False)

      if parallel:
        p = multiprocessing.Pool(None)
        map_func = p.map
      else:
        p = None
        map_func = map
      try:
        for bnum, tasks in data.items():
          tasks = filter(
              lambda t: t.state not in ('EXPIRED', 'BOT_DIED'), tasks)
          for task in tasks:
            benchmark_overall_times[task.benchmark_name].append(task.duration)

          simple = [(t.benchmark_name, t.task_id) for t in tasks
                    if 'reference' not in t.benchmark_name]
          res = map_func(_story_timings, simple)
          for (timings, name), t in zip(res, tasks):
            if not name:
              continue

            for test, times in timings.items():
              benchmark_times[name][test].append(times)
      finally:
        if p:
          p.close()

      for benchmark_name, stories in benchmark_times.items():
        per_builder[benchmark_name] = story_times = {}
        for story_name, times in stories.items():
          story_times[story_name] = [statistics.Median(
              [inner[i] for inner in times if i < len(inner)])
                                     for i in range(len(times[0]))]
          if len(story_times[story_name]) == 1:
            story_times[story_name] = story_times[story_name][0]

  with open(get_story_times_path(), 'w') as f:
    dump_json(new_times, f)


def get_args():
  parser = argparse.ArgumentParser(
      description=('Regenerate perf test timings'))

  parser.add_argument('--platforms', action='append', default=None)
  parser.add_argument('--builders', action='append')
  parser.add_argument('--refresh', '-r', action='store_true', default=False)
  parser.add_argument('--parallel', '-p', action='store_true', default=False)
  return parser


def dump_json(data, f):
  """Utility method to dump json which is indented, sorted, and readable"""
  return json.dump(data, f, indent=2, sort_keys=True, separators=(',', ': '))

def dumps_json(data):
  """Utility method to dump json which is indented, sorted, and readable"""
  return json.dumps(data, indent=2, sort_keys=True, separators=(',', ': '))

def main(args, configs):
  return refresh_times(configs, args.platforms, args.builders, args.refresh,
                       args.parallel)
