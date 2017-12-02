#!/usr/bin/env python
# Copyright 2016 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Script to generate the majority of the JSON files in the src/testing/buildbot
directory. Maintaining these files by hand is too unwieldy.
"""

import argparse
import ast
import collections
import copy
import json
import os
import string
import sys

THIS_DIR = os.path.dirname(os.path.abspath(__file__))


class BBGenErr(Exception):
  pass


class BaseGenerator(object):
  def __init__(self, bb_gen):
    self.bb_gen = bb_gen

  def generate(self, waterfall, name, config, input_tests, exceptions):
    raise NotImplementedError()

  def sort(self, tests):
    raise NotImplementedError()


class GTestGenerator(BaseGenerator):
  def __init__(self, bb_gen):
    super(GTestGenerator, self).__init__(bb_gen)

  def generate(self, waterfall, name, config, input_tests, exceptions):
    # The relative ordering of some of the tests is important to
    # minimize differences compared to the handwritten JSON files, since
    # Python's sorts are stable and there are some tests with the same
    # key (see gles2_conform_d3d9_test and similar variants). Avoid
    # losing the order by avoiding coalescing the dictionaries into one.
    gtests = []
    for test_name, test_config in sorted(input_tests.iteritems()):
      test = self.bb_gen.generate_gtest(
        waterfall, name, config, test_name, test_config, exceptions)
      if test:
        # generate_gtest may veto the test generation on this tester.
        gtests.append(test)
    return gtests

  def sort(self, tests):
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
    return sorted(tests, cmp=cmp_gtests)


class IsolatedScriptTestGenerator(BaseGenerator):
  def __init__(self, bb_gen):
    super(IsolatedScriptTestGenerator, self).__init__(bb_gen)

  def generate(self, waterfall, name, config, input_tests, exceptions):
    isolated_scripts = []
    for test_name, test_config in sorted(input_tests.iteritems()):
      test = self.bb_gen.generate_isolated_script_test(
        waterfall, name, config, test_name, test_config,
        exceptions)
      if test:
        isolated_scripts.append(test)
    return isolated_scripts

  def sort(self, tests):
    return sorted(tests, key=lambda x: x['name'])


class ScriptGenerator(BaseGenerator):
  def __init__(self, bb_gen):
    super(ScriptGenerator, self).__init__(bb_gen)

  def generate(self, waterfall, name, config, input_tests, exceptions):
    scripts = []
    for test_name, test_config in sorted(input_tests.iteritems()):
      test = self.bb_gen.generate_script_test(
        waterfall, name, config, test_name, test_config,
        exceptions)
      if test:
        scripts.append(test)
    return scripts

  def sort(self, tests):
    return sorted(tests, key=lambda x: x['name'])


class JUnitGenerator(BaseGenerator):
  def __init__(self, bb_gen):
    super(JUnitGenerator, self).__init__(bb_gen)

  def generate(self, waterfall, name, config, input_tests, exceptions):
    scripts = []
    for test_name, test_config in sorted(input_tests.iteritems()):
      test = self.bb_gen.generate_junit_test(
        waterfall, name, config, test_name, test_config, exceptions)
      if test:
        scripts.append(test)
    return scripts

  def sort(self, tests):
    return sorted(tests, key=lambda x: x['test'])


class CTSGenerator(BaseGenerator):
  def __init__(self, bb_gen):
    super(CTSGenerator, self).__init__(bb_gen)

  def generate(self, waterfall, name, config, input_tests, exceptions):
    # These only contain one entry and it's the contents of the input tests'
    # dictionary, verbatim.
    cts_tests = []
    cts_tests.append(input_tests)
    return cts_tests

  def sort(self, tests):
    return sorted(tests, cmp=lambda x, y: 0)


class InstrumentationTestGenerator(BaseGenerator):
  def __init__(self, bb_gen):
    super(InstrumentationTestGenerator, self).__init__(bb_gen)

  def generate(self, waterfall, name, config, input_tests, exceptions):
    scripts = []
    for test_name, test_config in sorted(input_tests.iteritems()):
      test = self.bb_gen.generate_instrumentation_test(
        waterfall, name, config, test_name, test_config, exceptions)
      if test:
        scripts.append(test)
    return scripts

  def sort(self, tests):
    return sorted(tests, key=lambda x: x['test'])


class BBJSONGenerator(object):
  def __init__(self):
    self.this_dir = THIS_DIR
    self.args = None
    self.waterfalls = None
    self.test_suites = None
    self.exceptions = None

  def generate_abs_file_path(self, relative_path):
    return os.path.join(self.this_dir, relative_path)

  def read_file(self, relative_path):
    with open(self.generate_abs_file_path(relative_path)) as fp:
      return fp.read()

  def load_pyl_file(self, filename):
    try:
      return ast.literal_eval(self.read_file(filename))
    except (SyntaxError, ValueError) as e:
      raise BBGenErr('Failed to parse pyl file "%s": %s' % (filename, e))

  def is_android(self, tester_config):
    return tester_config.get('os_type') == 'android'

  def should_run_on_tester(self, waterfall, tester_name, tester_config,
                           test_name, test_config, exceptions):
    # TODO(kbr): until this script is merged with the GPU test generator, a few
    # arguments will be unused.
    del waterfall
    del tester_config
    # Currently, the only reason a test should not run on a given tester is that
    # it's in the exceptions. (Once the GPU waterfall generation script is
    # incorporated here, the rules will become more complex.)  gtests may have
    # both "test" and "name" fields, and usually, if the "name" field is
    # specified, it means that the same test is being repurposed multiple times
    # with different command line arguments. To handle this case, prefer to
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

  def get_test_modifications(self, test, test_name, tester_name, exceptions):
    # gtests may have both "test" and "name" fields, and usually, if the "name"
    # field is specified, it means that the same test is being repurposed
    # multiple times with different command line arguments. To handle this case,
    # prefer to lookup per the "name" field of the test itself, as opposed to
    # the "test_name", which is actually the "test" field.
    exception = None
    if 'name' in test:
      exception = exceptions.get(test['name'])
    else:
      exception = exceptions.get(test_name)
    if not exception:
      return None
    return exception.get('modifications', {}).get(tester_name)

  def get_test_key_removals(self, test_name, tester_name, exceptions):
    exception = exceptions.get(test_name)
    if not exception:
      return []
    return exception.get('key_removals', {}).get(tester_name, [])

  def dictionary_merge(self, a, b, path=None, update=True):
    """http://stackoverflow.com/questions/7204805/
        python-dictionaries-of-dictionaries-merge
    merges b into a
    """
    if path is None:
      path = []
    for key in b:
      if key in a:
        if isinstance(a[key], dict) and isinstance(b[key], dict):
          self.dictionary_merge(a[key], b[key], path + [str(key)])
        elif a[key] == b[key]:
          pass # same leaf value
        elif isinstance(a[key], list) and isinstance(b[key], list):
          # TODO(kbr): this only works properly if the two arrays are
          # the same length, which is currently always the case in the
          # swarming dimension_sets that we have to merge. It will fail
          # to merge / override 'args' arrays which are different
          # length.
          for idx in xrange(len(b[key])):
            try:
              a[key][idx] = self.dictionary_merge(a[key][idx], b[key][idx],
                                                  path + [str(key), str(idx)],
                                                  update=update)
            except (IndexError, TypeError):
              raise BBGenErr('Error merging list keys ' + str(key) +
                              ' and indices ' + str(idx) + ' between ' +
                              str(a) + ' and ' + str(b))
        elif update:
          a[key] = b[key]
        else:
          raise BBGenErr('Conflict at %s' % '.'.join(path + [str(key)]))
      else:
        a[key] = b[key]
    return a

  def initialize_swarming_dictionary_for_test(self, generated_test,
                                              tester_config):
    if 'swarming' not in generated_test:
      generated_test['swarming'] = {}
    generated_test['swarming'].update({
      'can_use_on_swarming_builders': tester_config.get('use_swarming', True)
    })
    if 'swarming' in tester_config:
      if 'dimension_sets' not in generated_test['swarming']:
        generated_test['swarming']['dimension_sets'] = copy.deepcopy(
          tester_config['swarming']['dimension_sets'])
      self.dictionary_merge(generated_test['swarming'],
                            tester_config['swarming'])
    # Apply any Android-specific Swarming dimensions after the generic ones.
    if 'android_swarming' in generated_test:
      if self.is_android(tester_config):
        self.dictionary_merge(generated_test['swarming'],
                              generated_test['android_swarming'])
      del generated_test['android_swarming']

  def clean_swarming_dictionary(self, swarming_dict):
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

  def update_and_cleanup_test(self, test, test_name, tester_name, exceptions):
    # See if there are any exceptions that need to be merged into this
    # test's specification.
    modifications = self.get_test_modifications(test, test_name, tester_name,
                                                exceptions)
    if modifications:
      test = self.dictionary_merge(test, modifications)
    for k in self.get_test_key_removals(test_name, tester_name, exceptions):
      del test[k]
    self.clean_swarming_dictionary(test['swarming'])
    return test

  def generate_gtest(self, waterfall, tester_name, tester_config, test_name,
                     test_config, exceptions):
    if not self.should_run_on_tester(
        waterfall, tester_name, tester_config, test_name, test_config,
        exceptions):
      return None
    result = copy.deepcopy(test_config)
    if 'test' in result:
      result['name'] = test_name
    else:
      result['test'] = test_name
    self.initialize_swarming_dictionary_for_test(result, tester_config)
    if self.is_android(tester_config) and tester_config.get('use_swarming',
                                                            True):
      if not tester_config.get('skip_merge_script', False):
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
      if not tester_config.get('skip_output_links', False):
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
    result = self.update_and_cleanup_test(result, test_name, tester_name,
                                          exceptions)
    return result

  def generate_isolated_script_test(self, waterfall, tester_name, tester_config,
                                    test_name, test_config, exceptions):
    if not self.should_run_on_tester(waterfall, tester_name, tester_config,
                                     test_name, test_config, exceptions):
      return None
    result = copy.deepcopy(test_config)
    result['isolate_name'] = result.get('isolate_name', test_name)
    result['name'] = test_name
    self.initialize_swarming_dictionary_for_test(result, tester_config)
    result = self.update_and_cleanup_test(result, test_name, tester_name,
                                          exceptions)
    return result

  def generate_script_test(self, waterfall, tester_name, tester_config,
                           test_name, test_config, exceptions):
    if not self.should_run_on_tester(waterfall, tester_name, tester_config,
                                     test_name, test_config, exceptions):
      return None
    result = {
      'name': test_name,
      'script': test_config['script']
    }
    return result

  def generate_junit_test(self, waterfall, tester_name, tester_config,
                          test_name, test_config, exceptions):
    if not self.should_run_on_tester(waterfall, tester_name, tester_config,
                                     test_name, test_config, exceptions):
      return None
    result = {
      'test': test_name,
    }
    return result

  def generate_instrumentation_test(self, waterfall, tester_name, tester_config,
                                    test_name, test_config, exceptions):
    if not self.should_run_on_tester(waterfall, tester_name, tester_config,
                                     test_name, test_config, exceptions):
      return None
    result = copy.deepcopy(test_config)
    result['test'] = test_name
    return result

  def get_test_generator_map(self):
    return {
      'cts_tests': CTSGenerator(self),
      'gtest_tests': GTestGenerator(self),
      'instrumentation_tests': InstrumentationTestGenerator(self),
      'isolated_scripts': IsolatedScriptTestGenerator(self),
      'junit_tests': JUnitGenerator(self),
      'scripts': ScriptGenerator(self),
    }

  def generate_waterfall_json(self, waterfall, filename, exceptions):
    all_tests = {}
    for builder, config in waterfall.get('builders', {}).iteritems():
      all_tests[builder] = config
    generator_map = self.get_test_generator_map()
    for name, config in waterfall['testers'].iteritems():
      tests = {}
      if 'additional_compile_targets' in config:
        tests['additional_compile_targets'] = config[
          'additional_compile_targets']
      test_suites = config['test_suites']
      for test_type, input_tests in test_suites.iteritems():
        if test_type not in generator_map:
          raise BBGenErr('Unknown test type ' + test_type +
                         ' in test suites for ' + name)
        test_generator = generator_map[test_type]
        tests[test_type] = test_generator.sort(test_generator.generate(
          waterfall, name, config, input_tests, exceptions))
      all_tests[name] = tests
    all_tests['AAAAA1 AUTOGENERATED FILE DO NOT EDIT'] = {}
    all_tests['AAAAA2 See generate_buildbot_json.py to make changes'] = {}
    with open(self.generate_abs_file_path(filename), 'wb') as fp:
      json.dump(all_tests, fp, indent=2, separators=(',', ': '), sort_keys=True)
      fp.write('\n')

  def link_waterfalls_to_test_suites(self):
    for waterfall in self.waterfalls:
      for tester_name, tester in waterfall['testers'].iteritems():
        for suite, value in tester['test_suites'].iteritems():
          if not value in self.test_suites:
            raise BBGenErr(
              'Test suite %s from machine %s on waterfall %s not present in ' \
              'test_suites.pyl' % (value, tester_name, waterfall['name']))
          tester['test_suites'][suite] = self.test_suites[value]

  def check_composition_test_suites(self):
    # Pre-pass to catch errors reliably.
    for name, value in self.test_suites.iteritems():
      if isinstance(value, list):
        for entry in value:
          if isinstance(self.test_suites[entry], list):
            raise BBGenErr('Composition test suites may not refer to other ' \
                           'composition test suites (error found while ' \
                           'processing %s)' % name)

  def resolve_composition_test_suites(self):
    self.check_composition_test_suites()
    for name, value in self.test_suites.iteritems():
      if isinstance(value, list):
        # Resolve this to a dictionary.
        full_suite = {}
        for entry in value:
          suite = self.test_suites[entry]
          full_suite.update(suite)
        self.test_suites[name] = full_suite

  def load_configuration_files(self):
    self.waterfalls = self.load_pyl_file('waterfalls.pyl')
    self.test_suites = self.load_pyl_file('test_suites.pyl')
    self.exceptions = self.load_pyl_file('test_suite_exceptions.pyl')

  def resolve_configuration_files(self):
    self.resolve_composition_test_suites()
    self.link_waterfalls_to_test_suites()

  def parse_args(self, argv):
    parser = argparse.ArgumentParser()
    parser.add_argument(
      '-c', '--check', action='store_true', help=
      'Do consistency checks of configuration and generated files and then '
      'exit. Used during presubmit. Causes the tool to not generate any files.')
    parser.add_argument(
      'waterfall_filters', metavar='waterfalls', type=str, nargs='*',
      help='Optional list of waterfalls to generate.')
    self.args = parser.parse_args(argv)

  def check_consistency(self):
    self.load_configuration_files()
    self.check_composition_test_suites()
    # All test suites must be referenced.
    suites_seen = set()
    generator_map = self.get_test_generator_map()
    for waterfall in self.waterfalls:
      for bot_name, tester in waterfall['testers'].iteritems():
        for suite_type, suite in tester['test_suites'].iteritems():
          if suite_type not in generator_map:
            raise BBGenErr('Unknown test suite type ' + suite_type +
                           ' in bot ' + bot_name + ' on waterfall ' +
                           waterfall['name'])
          suites_seen.add(suite)
    # Since we didn't resolve the configuration files, this set
    # includes both composition test suites and regular ones.
    resolved_suites = set()
    for suite_name in suites_seen:
      suite = self.test_suites[suite_name]
      if isinstance(suite, list):
        for sub_suite in suite:
          resolved_suites.add(sub_suite)
      resolved_suites.add(suite_name)
    # At this point, every key in test_suites.pyl should be referenced.
    missing_suites = set(self.test_suites.keys()) - resolved_suites
    if missing_suites:
      raise BBGenErr('The following test suites were unreferenced by bots on '
                     'the waterfalls: ' + str(missing_suites))

    # All test suite exceptions must refer to bots on the waterfall.
    all_bots = set()
    missing_bots = set()
    for waterfall in self.waterfalls:
      for bot_name, tester in waterfall['testers'].iteritems():
        all_bots.add(bot_name)
    for exception in self.exceptions.itervalues():
      for removal in exception.get('remove_from', []):
        if removal not in all_bots:
          missing_bots.add(removal)
      for mod in exception.get('modifications', {}).iterkeys():
        if mod not in all_bots:
          missing_bots.add(mod)
    if missing_bots:
      raise BBGenErr('The following nonexistent machines were referenced in '
                     'the test suite exceptions: ' + str(missing_bots))


  def generate_waterfalls(self):
    self.load_configuration_files()
    self.resolve_configuration_files()
    filters = self.args.waterfall_filters
    for waterfall in self.waterfalls:
      should_gen = not filters or waterfall['name'] in filters
      if should_gen:
        self.generate_waterfall_json(waterfall, waterfall['name'] + '.json',
                                     self.exceptions)

  def main(self, argv):
    self.parse_args(argv)
    if self.args.check:
      self.check_consistency()
    else:
      self.generate_waterfalls()
    return 0

if __name__ == "__main__":
  generator = BBJSONGenerator()
  sys.exit(generator.main(sys.argv[1:]))
