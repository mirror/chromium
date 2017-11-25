#!/usr/bin/env python
# Copyright 2016 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Script to generate chromium.gpu.json and chromium.gpu.fyi.json in
the src/testing/buildbot directory. Maintaining these files by hand is
too unwieldy.
"""

import ast
import collections
import copy
import json
import os
import string
import sys

THIS_DIR = os.path.dirname(os.path.abspath(__file__))
SRC_DIR = os.path.dirname(os.path.dirname(THIS_DIR))


def generate_abs_file_path(path):
  return os.path.join(SRC_DIR, 'testing', 'buildbot2', path)


def read_file(path):
  with open(path) as fp:
    return fp.read()


def load_pyl_file(filename):
  try:
    return ast.literal_eval(read_file(generate_abs_file_path(filename)))
  except (SyntaxError, ValueError) as e:
    raise Exception('Failed to parse pyl file "%s": %s' %
                    (filename, e))


def should_run_on_tester(waterfall, tester_name, tester_config, test_name,
                         test_config, exceptions):
  # TODO(kbr): until this script is merged with the GPU test generator, a few
  # arguments will be unused.
  del waterfall
  del tester_config
  del test_config
  # Currently, the only reason a test should not run on a given tester
  # is that it's in the exceptions. (Once the GPU waterfall generation
  # script is incorporated here, the rules will become more complex.)
  exception = exceptions.get(test_name)
  if not exception:
    return True
  remove_from = exception.get('remove_from')
  if not remove_from:
    return True
  return tester_name not in remove_from


def get_test_modifications(test_name, tester_name, exceptions):
  exception = exceptions.get(test_name)
  if not exception:
    return None
  return exception.get('modifications', {}).get(tester_name)


def dictionary_merge(a, b, path=None, update=True):
  """http://stackoverflow.com/questions/7204805/
      python-dictionaries-of-dictionaries-merge
  merges b into a
  """
  if path is None:
    path = []
  for key in b:
    if key in a:
      if isinstance(a[key], dict) and isinstance(b[key], dict):
        dictionary_merge(a[key], b[key], path + [str(key)])
      elif a[key] == b[key]:
        pass # same leaf value
      elif isinstance(a[key], list) and isinstance(b[key], list):
        for idx in b[key]:
          a[key][idx] = dictionary_merge(a[key][idx], b[key][idx],
                                         path + [str(key), str(idx)],
                                         update=update)
      elif update:
        a[key] = b[key]
      else:
        raise Exception('Conflict at %s' % '.'.join(path + [str(key)]))
    else:
      a[key] = b[key]
  return a


def generate_gtest(waterfall, tester_name, tester_config, test_name,
                   test_config, exceptions):
  if not should_run_on_tester(
      waterfall, tester_name, tester_config, test_name, test_config,
      exceptions):
    return None
  result = copy.deepcopy(test_config)
  if 'test' in result:
    result['name'] = test_name
  else:
    result['test'] = test_name
  if not 'swarming' in result:
    result['swarming'] = {}
  result['swarming'].update({
    'can_use_on_swarming_builders': True,
  })
  # See if there are any exceptions that need to be merged into this
  # test's specification.
  modifications = get_test_modifications(test_name, tester_name, exceptions)
  if modifications:
    result = dictionary_merge(result, modifications)
  return result


def generate_gtests(waterfall, tester_name, tester_config, test_dictionary,
                    exceptions):
  # The relative ordering of some of the tests is important to
  # minimize differences compared to the handwritten JSON files, since
  # Python's sorts are stable and there are some tests with the same
  # key (see gles2_conform_d3d9_test and similar variants). Avoid
  # losing the order by avoiding coalescing the dictionaries into one.
  gtests = []
  for test_name, test_config in sorted(test_dictionary.iteritems()):
    test = generate_gtest(waterfall, tester_name, tester_config,
                          test_name, test_config, exceptions)
    if test:
      # generate_gtest may veto the test generation on this tester.
      gtests.append(test)
  return gtests


def generate_isolated_script_test(waterfall, tester_name, tester_config,
                                  test_name, test_config, exceptions):
  if not should_run_on_tester(waterfall, tester_name, tester_config, test_name,
                              test_config, exceptions):
    return None
  swarming = {
    'can_use_on_swarming_builders': True,
  }
  if 'swarming' in test_config:
    swarming.update(test_config['swarming'])
  result = {
    'isolate_name': test_config.get('isolate_name', test_name),
    'name': test_name,
    'swarming': swarming,
  }
  if 'args' in test_config:
    result['args'] = test_config['args']
  if 'merge' in test_config:
    result['merge'] = test_config['merge']
  # See if there are any exceptions that need to be merged into this
  # test's specification.
  modifications = get_test_modifications(test_name, tester_name, exceptions)
  if modifications:
    result = dictionary_merge(result, modifications)
  return result


def generate_isolated_script_tests(waterfall, tester_name, tester_config,
                                   test_dictionary, exceptions):
  isolated_scripts = []
  for test_name, test_config in sorted(test_dictionary.iteritems()):
    test = generate_isolated_script_test(
      waterfall, tester_name, tester_config, test_name, test_config, exceptions)
    if test:
      isolated_scripts.append(test)
  return isolated_scripts


def generate_script_test(waterfall, tester_name, tester_config,
                         test_name, test_config, exceptions):
  if not should_run_on_tester(waterfall, tester_name, tester_config, test_name,
                              test_config, exceptions):
    return None
  result = {
    'name': test_name,
    'script': test_config['script']
  }
  return result


def generate_script_tests(waterfall, tester_name, tester_config,
                          test_dictionary, exceptions):
  scripts = []
  for test_name, test_config in sorted(test_dictionary.iteritems()):
    test = generate_script_test(
      waterfall, tester_name, tester_config, test_name, test_config, exceptions)
    if test:
      scripts.append(test)
  return scripts


def generate_waterfall_json(waterfall, filename, exceptions):
  all_tests = {}
  for builder, config in waterfall.get('builders', {}).iteritems():
    all_tests[builder] = config
  for name, config in waterfall['testers'].iteritems():
    tests = {}
    test_suites = config['test_suites']
    if 'gtests' in test_suites:
      tests['gtest_tests'] = sorted(generate_gtests(waterfall, name, config,
                                                    test_suites['gtests'],
                                                    exceptions),
                                    key=lambda x: x['test'])
    if 'isolated_scripts' in test_suites:
      tests['isolated_scripts'] = sorted(generate_isolated_script_tests(
        waterfall, name, config, test_suites['isolated_scripts'], exceptions),
                                         key=lambda x: x['name'])
    if 'scripts' in test_suites:
      tests['scripts'] = sorted(generate_script_tests(
        waterfall, name, config, test_suites['scripts'], exceptions),
                                key=lambda x: x['name'])
    all_tests[name] = tests
  out_waterfall = all_tests
  if 'output_order' in waterfall:
    out_waterfall = collections.OrderedDict()
  out_waterfall['AAAAA1 AUTOGENERATED FILE DO NOT EDIT'] = {}
  out_waterfall['AAAAA2 See generate_buildbot_json.py to make changes'] = {}
  if 'output_order' in waterfall:
    for machine_name in waterfall['output_order']:
      out_waterfall[machine_name] = all_tests[machine_name]
  with open(generate_abs_file_path(filename), 'wb') as fp:
    # TODO(kbr): once all waterfalls are autogenerated, delete the output_order
    # lists and related code.
    json.dump(out_waterfall, fp, indent=2, separators=(',', ': '),
              sort_keys=True)
    fp.write('\n')


def link_waterfalls_to_test_suites(waterfalls, test_suites):
  for waterfall in waterfalls:
    for tester_name, tester in waterfall['testers'].iteritems():
      for suite, value in tester['test_suites'].iteritems():
        if not value in test_suites:
          raise Exception(
            'Test suite %s from machine %s on waterfall %s not present in ' \
            'test_suites.pyl' % (value, tester_name, waterfall['name']))
        tester['test_suites'][suite] = test_suites[value]


def main():
  waterfalls = load_pyl_file('waterfalls.pyl')
  test_suites = load_pyl_file('test_suites.pyl')
  exceptions = load_pyl_file('test_suite_exceptions.pyl')

  link_waterfalls_to_test_suites(waterfalls, test_suites)

  for waterfall in waterfalls:
    generate_waterfall_json(waterfall, waterfall['name'] + '.json', exceptions)
  return 0

if __name__ == "__main__":
  sys.exit(main())
