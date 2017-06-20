#!/usr/bin/env python
# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Makes sure network traffic annoations are syntactically and semantically
correct and all required functions are annotated.
"""

import os
# import optparse
# import re
# import sys

# import proto_checker
# import cpp_checker
# import java_checker
# import results

# from builddeps import DepsBuilder
# from rules import Rule, Rules

class NetworkTrafficAnnotationChecker():
  """...
  """

  EXTENSIONS = [
      '.cc',
      '.mm',
  ]

  def __init__(self):
    pass


  def CheckFiles(self, file_paths):
    """...
    """
    pruned_file_paths = [
        file_path for file_path in file_paths if IsCPPFile(file_path)]

    if not pruned_file_paths:
      return None, None


def PrintUsage():
  print """Usage: python checkdeps.py [--root <root>] [tocheck]

  --root ROOT Specifies the repository root. This defaults to "../../.."
              relative to the script file. This will be correct given the
              normal location of the script in "<root>/tools/checkdeps".

  --(others)  There are a few lesser-used options; run with --help to show them.

  tocheck  Specifies the directory, relative to root, to check. This defaults
           to "." so it checks everything.

Examples:
  python checkdeps.py
  python checkdeps.py --root c:\\source chrome"""


def main():
  # option_parser = optparse.OptionParser()
  # option_parser.add_option(
  #     '', '--root',
  #     default='', dest='base_directory',
  #     help='Specifies the repository root. This defaults '
  #          'to "../../.." relative to the script file, which '
  #          'will normally be the repository root.')
  # option_parser.add_option(
  #     '', '--extra-repos',
  #     action='append', dest='extra_repos', default=[],
  #     help='Specifies extra repositories relative to root repository.')
  # option_parser.add_option(
  #     '', '--ignore-temp-rules',
  #     action='store_true', dest='ignore_temp_rules', default=False,
  #     help='Ignore !-prefixed (temporary) rules.')
  # option_parser.add_option(
  #     '', '--generate-temp-rules',
  #     action='store_true', dest='generate_temp_rules', default=False,
  #     help='Print rules to temporarily allow files that fail '
  #          'dependency checking.')
  # option_parser.add_option(
  #     '', '--count-violations',
  #     action='store_true', dest='count_violations', default=False,
  #     help='Count #includes in violation of intended rules.')
  # option_parser.add_option(
  #     '', '--skip-tests',
  #     action='store_true', dest='skip_tests', default=False,
  #     help='Skip checking test files (best effort).')
  # option_parser.add_option(
  #     '-v', '--verbose',
  #     action='store_true', default=False,
  #     help='Print debug logging')
  # option_parser.add_option(
  #     '', '--json',
  #     help='Path to JSON output file')
  # option_parser.add_option(
  #     '', '--no-resolve-dotdot',
  #     action='store_false', dest='resolve_dotdot', default=True,
  #     help='resolve leading ../ in include directive paths relative '
  #          'to the file perfoming the inclusion.')

  # options, args = option_parser.parse_args()

  # deps_checker = DepsChecker(options.base_directory,
  #                            extra_repos=options.extra_repos,
  #                            verbose=options.verbose,
  #                            ignore_temp_rules=options.ignore_temp_rules,
  #                            skip_tests=options.skip_tests,
  #                            resolve_dotdot=options.resolve_dotdot)
  # base_directory = deps_checker.base_directory
  # # Default if needed, normalized

  # # Figure out which directory we have to check.
  # start_dir = base_directory
  # if len(args) == 1:
  #   # Directory specified. Start here. It's supposed to be relative to the
  #   # base directory.
  #   start_dir = os.path.abspath(os.path.join(base_directory, args[0]))
  # elif len(args) >= 2 or (options.generate_temp_rules and
  #                         options.count_violations):
  #   # More than one argument, or incompatible flags, we don't handle this.
  #   PrintUsage()
  #   return 1

  # if not start_dir.startswith(deps_checker.base_directory):
  #   print 'Directory to check must be a subdirectory of the base directory,'
  #   print 'but %s is not a subdirectory of %s' % (start_dir, base_directory)
  #   return 1

  # print 'Using base directory:', base_directory
  # print 'Checking:', start_dir

  # if options.generate_temp_rules:
  #   deps_checker.results_formatter = results.TemporaryRulesFormatter()
  # elif options.count_violations:
  #   deps_checker.results_formatter = results.CountViolationsFormatter()

  # if options.json:
  #   deps_checker.results_formatter = results.JSONResultsFormatter(
  #       options.json, deps_checker.results_formatter)

  # deps_checker.CheckDirectory(start_dir)
  # return deps_checker.Report()


if '__main__' == __name__:
  sys.exit(main())
