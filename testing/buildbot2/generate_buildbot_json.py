#!/usr/bin/env python
# Copyright 2016 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Script to generate the majority of the JSON files in the src/testing/buildbot
directory. Maintaining these files by hand is too unwieldy.
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


def is_android(tester_config):
  return tester_config.get('os_type') == 'android'


def should_run_on_tester(waterfall, tester_name, tester_config, test_name,
                         test_config, exceptions):
  # TODO(kbr): until this script is merged with the GPU test generator, a few
  # arguments will be unused.
  del waterfall
  del tester_config
  # Currently, the only reason a test should not run on a given tester is that
  # it's in the exceptions. (Once the GPU waterfall generation script is
  # incorporated here, the rules will become more complex.)
  # gtests may have both "test" and "name" fields, and usually, if the "name"
  # field is specified, it means that the same test is being repurposed multiple
  # times with different command line arguments. To handle this case, prefer to
  # lookup per the "name" field of the test itself, as opposed to the
  # "test_name", which is actually the "test" field.
  exception = None
  if 'name' in test_config:
    exception = exceptions.get(test_config['name'])
  else:
    exception = exceptions.get(test_name)
  if not exception:
    return True
  remove_from = exception.get('remove_from')
  if not remove_from:
    return True
  return tester_name not in remove_from


def get_test_modifications(test, test_name, tester_name, exceptions):
  # gtests may have both "test" and "name" fields, and usually, if the "name"
  # field is specified, it means that the same test is being repurposed multiple
  # times with different command line arguments. To handle this case, prefer to
  # lookup per the "name" field of the test itself, as opposed to the
  # "test_name", which is actually the "test" field.
  exception = None
  if 'name' in test:
    exception = exceptions.get(test['name'])
  else:
    exception = exceptions.get(test_name)
  if not exception:
    return None
  return exception.get('modifications', {}).get(tester_name)


def get_test_key_removals(test_name, tester_name, exceptions):
  exception = exceptions.get(test_name)
  if not exception:
    return []
  return exception.get('key_removals', {}).get(tester_name, [])


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
        # TODO(kbr): this only works properly if the two arrays are
        # the same length (which is currently always the case in the
        # swarming dimension_sets that we have to merge).
        for idx in xrange(len(b[key])):
          try:
            a[key][idx] = dictionary_merge(a[key][idx], b[key][idx],
                                           path + [str(key), str(idx)],
                                           update=update)
          except (IndexError, TypeError):
            raise Exception('Error merging list keys ' + str(key) +
                            ' and indices ' + str(idx) + ' between ' +
                            str(a) + ' and ' + str(b))
      elif update:
        a[key] = b[key]
      else:
        raise Exception('Conflict at %s' % '.'.join(path + [str(key)]))
    else:
      a[key] = b[key]
  return a


def initialize_swarming_dictionary_for_test(generated_test, tester_config):
  if 'swarming' not in generated_test:
    generated_test['swarming'] = {}
  generated_test['swarming'].update({
    'can_use_on_swarming_builders': tester_config.get('use_swarming', True)
  })
  if 'swarming' in tester_config:
    if 'dimension_sets' not in generated_test['swarming']:
      generated_test['swarming']['dimension_sets'] = copy.deepcopy(
        tester_config['swarming']['dimension_sets'])
    dictionary_merge(generated_test['swarming'], tester_config['swarming'])
  # Apply any Android-specific Swarming dimensions after the generic ones.
  if 'android_swarming' in generated_test:
    if is_android(tester_config):
      dictionary_merge(generated_test['swarming'],
                       generated_test['android_swarming'])
    del generated_test['android_swarming']


def clean_swarming_dictionary(swarming_dict):
  # Clean out redundant entries from a test's "swarming" dictionary.
  # This is really only needed to retain 100% parity with the
  # handwritten JSON files, and can be removed once all the files are
  # autogenerated.
  if 'shards' in swarming_dict:
    if swarming_dict['shards'] == 1:
      del swarming_dict['shards']
  if not swarming_dict['can_use_on_swarming_builders']:
    # Remove all other keys.
    for k in swarming_dict.keys():
      if k != 'can_use_on_swarming_builders':
        del swarming_dict[k]


def update_and_cleanup_test(test, test_name, tester_name, exceptions):
  # See if there are any exceptions that need to be merged into this
  # test's specification.
  modifications = get_test_modifications(test, test_name, tester_name,
                                         exceptions)
  if modifications:
    test = dictionary_merge(test, modifications)
  for k in get_test_key_removals(test_name, tester_name, exceptions):
    del test[k]
  clean_swarming_dictionary(test['swarming'])
  return test


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
  initialize_swarming_dictionary_for_test(result, tester_config)
  if is_android(tester_config):
    if tester_config.get('use_swarming', True) and not tester_config.get(
        'skip_merge_script', False):
      result['merge'] = {
        'args': [
          '--bucket',
          'chromium-result-details',
          '--test-name',
          test_name
        ],
        'script': '//build/android/pylib/results/presentation/' \
          'test_results_presentation.py',
      }
    result['swarming']['cipd_packages'] = [
      {
        'cipd_package': 'infra/tools/luci/logdog/butler/${platform}',
        'location': 'bin',
        'revision': 'git_revision:ff387eadf445b24c935f1cf7d6ddd279f8a6b04c',
      }
    ]
    result['swarming']['output_links'] = [
      {
        'link': [
          'https://luci-logdog.appspot.com/v/?s',
          '=android%2Fswarming%2Flogcats%2F',
          '${TASK_ID}%2F%2B%2Funified_logcats',
        ],
        'name': 'shard #${SHARD_INDEX} logcats',
      },
    ]
  result = update_and_cleanup_test(result, test_name, tester_name, exceptions)
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
  result = copy.deepcopy(test_config)
  result['isolate_name'] = result.get('isolate_name', test_name)
  result['name'] = test_name
  initialize_swarming_dictionary_for_test(result, tester_config)
  result = update_and_cleanup_test(result, test_name, tester_name, exceptions)
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


def generate_junit_test(waterfall, tester_name, tester_config,
                        test_name, test_config, exceptions):
  if not should_run_on_tester(waterfall, tester_name, tester_config, test_name,
                              test_config, exceptions):
    return None
  result = {
    'test': test_name,
  }
  return result


def generate_junit_tests(waterfall, tester_name, tester_config,
                         test_dictionary, exceptions):
  scripts = []
  for test_name, test_config in sorted(test_dictionary.iteritems()):
    test = generate_junit_test(
      waterfall, tester_name, tester_config, test_name, test_config, exceptions)
    if test:
      scripts.append(test)
  return scripts


def generate_instrumentation_test(waterfall, tester_name, tester_config,
                                  test_name, test_config, exceptions):
  if not should_run_on_tester(waterfall, tester_name, tester_config, test_name,
                              test_config, exceptions):
    return None
  result = copy.deepcopy(test_config)
  result['test'] = test_name
  return result


def generate_instrumentation_tests(waterfall, tester_name, tester_config,
                                   test_dictionary, exceptions):
  scripts = []
  for test_name, test_config in sorted(test_dictionary.iteritems()):
    test = generate_instrumentation_test(
      waterfall, tester_name, tester_config, test_name, test_config, exceptions)
    if test:
      scripts.append(test)
  return scripts


def generate_cts_tests(test_dictionary):
  # These only contain one entry and it's the contents of the test
  # dictionary, verbatim.
  cts_tests = []
  cts_tests.append(test_dictionary)
  return cts_tests


def cmp_gtests(a, b):
  # Prefer to compare based on the "test" key.
  val = cmp(a['test'], b['test'])
  if val != 0:
    return val
  if 'name' in a and 'name' in b:
    return cmp(a['name'], b['name'])
  if 'name' not in a and 'name' not in b:
    return 0
  # Prefer to put variants of the same test after the first one.
  if 'name' in a:
    return 1
  # 'name' is in b.
  return -1


def generate_waterfall_json(waterfall, filename, exceptions):
  all_tests = {}
  for builder, config in waterfall.get('builders', {}).iteritems():
    all_tests[builder] = config
  for name, config in waterfall['testers'].iteritems():
    tests = {}
    if 'additional_compile_targets' in config:
      tests['additional_compile_targets'] = config['additional_compile_targets']
    test_suites = config['test_suites']
    for test_type, input_tests in test_suites.iteritems():
      if test_type == 'gtests':
        tests['gtest_tests'] = sorted(generate_gtests(waterfall, name, config,
                                                      input_tests, exceptions),
                                      cmp=cmp_gtests)
      elif test_type == 'isolated_scripts':
        tests['isolated_scripts'] = sorted(generate_isolated_script_tests(
          waterfall, name, config, input_tests, exceptions),
                                           key=lambda x: x['name'])
      elif test_type == 'scripts':
        tests['scripts'] = sorted(generate_script_tests(
          waterfall, name, config, input_tests, exceptions),
                                  key=lambda x: x['name'])
      elif test_type == 'junit_tests':
        tests['junit_tests'] = sorted(generate_junit_tests(
          waterfall, name, config, input_tests, exceptions),
                                      key=lambda x: x['test'])
      elif test_type == 'cts_tests':
        tests['cts_tests'] = generate_cts_tests(input_tests)
      elif test_type == 'instrumentation_tests':
        tests['instrumentation_tests'] = sorted(generate_instrumentation_tests(
          waterfall, name, config, input_tests, exceptions),
                                                key=lambda x: x['test'])
      else:
        raise 'Unknown test type ' + test_type + ' in test suites for ' + name
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


def resolve_composition_test_suites(test_suites):
  # Pre-pass to catch errors reliably.
  for name, value in test_suites.iteritems():
    if isinstance(value, list):
      for entry in value:
        if isinstance(test_suites[entry], list):
          raise Exception('Composition test suites may not refer to other ' \
                          'composition test suites (error found while ' \
                          'processing %s)' % name)
  for name, value in test_suites.iteritems():
    if isinstance(value, list):
      # Resolve this to a dictionary.
      full_suite = {}
      for entry in value:
        suite = test_suites[entry]
        full_suite.update(suite)
      test_suites[name] = full_suite


def main():
  waterfalls = load_pyl_file('waterfalls.pyl')
  test_suites = load_pyl_file('test_suites.pyl')
  resolve_composition_test_suites(test_suites)
  exceptions = load_pyl_file('test_suite_exceptions.pyl')

  link_waterfalls_to_test_suites(waterfalls, test_suites)

  for waterfall in waterfalls:
    generate_waterfall_json(waterfall, waterfall['name'] + '.json', exceptions)
  return 0

if __name__ == "__main__":
  sys.exit(main())
