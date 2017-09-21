#!/usr/bin/env python2
#
# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from __future__ import print_function

import argparse
import logging
import os

HTML_FILE_EXTENSION = '.html'

# FIXME(mmoroz): should I use third_party/llvm-build/Release+Asserts/bin/
# or rely on having a proper $PATH specified by the user?
LLVM_BIN_PATH = '/usr/local/google/home/mmoroz/Projects/llvm/install/bin/'
LLVM_COV_PATH = os.path.join(LLVM_BIN_PATH, 'llvm-cov')
LLVM_PROFDATA_PATH = os.path.join(LLVM_BIN_PATH, 'llvm-profdata')

LLVM_PROFILE_FILE_NAME = 'coverage.profraw'
LLVM_COVERAGE_FILE_NAME = 'coverage.profdata'

REPORT_FILENAME = 'report.html'

REPORT_TEMPLATE = '''<!DOCTYPE html>
<html>
<head><style>
th, td {{ border: 1px solid black; padding: 5px 10px; }}
</style></head>
<body>
{table_data}
</body></html>'''

SINGLE_FILE_START_MARKER = '<!doctype html><html>'
SINGLE_FILE_END_MARKER = '</body></html>'

SOURCE_FILENAME_START_MARKER = (
    "<body><div class='centered'><table><div class='source-name-title'><pre>")
SOURCE_FILENAME_END_MARKER = '</pre>'

STYLE_START_MARKER = '<style>'
STYLE_END_MARKER = '</style>'
STYLE_FILENAME = 'style.css'

ZERO_FUNCTION_FILE_TEXT = 'Files which contain no functions'


def create_output_dir(dir_path):
  if not os.path.exists(dir_path):
    os.mkdir(dir_path)
    return

  if os.path.isdir(dir_path):
    logging.warning('%s already exists.' % dir_path)
    return

  logging.error('%s exists and does not point to a directory.' % dir_path)
  raise Exception('Invalid --output argument specified.')


def extract_and_fix_filename(data, source_dir):
  filename_start = data.find(SOURCE_FILENAME_START_MARKER)
  if filename_start == -1:
    logging.error('Failed to extract source code filename.')
    raise Exception('Failed to process coverage dump.')

  filename_start += len(SOURCE_FILENAME_START_MARKER)
  filename_end = data[filename_start:].find(SOURCE_FILENAME_END_MARKER)
  if filename_end == -1:
    logging.error('Failed to extract source code filename.')
    raise Exception('Failed to process coverage dump.')

  filename_end += filename_start

  filename = data[filename_start : filename_end]

  source_dir = os.path.abspath(source_dir)

  if not filename.startswith(source_dir):
    logging.error('Invalid source code path ("%s") specified.\n'
                  'Coverage dump refers to "%s".' % (source_dir, filename))
    raise Exception('Failed to process coverage dump.')

  filename = filename[len(source_dir) : ]
  filename = filename.lstrip('/\\')

  # Replace the filename with the shorter version.
  data = data[ : filename_start] + filename + data[filename_end : ]
  return filename, data


def generate_report(report_data):
  table_data = '<table>\n'
  report_lines = report_data.splitlines()

  # Write header.
  table_data += '  <tr>\n'
  for column in report_lines[0].split('  '):
    if not column:
      continue
    table_data += '    <th>%s</th>\n' % column
  table_data += '  </tr>\n'

  for line in report_lines[1:-1]:
    if not line or line.startswith('---'):
      continue

    if line.startswith(ZERO_FUNCTION_FILE_TEXT):
      table_data += '  <tr>\n'
      table_data += '    <td><b>%s</b></td>\n' % line
      table_data += '  </tr>\n'
      continue

    table_data += '  <tr>\n'

    columns = line.split()

    # First column is a file name, build a link.
    table_data += '    <td><a href="/%s">%s</a></td>\n' % (
        columns[0] + HTML_FILE_EXTENSION, columns[0])

    for column in line.split()[1:]:
      table_data += '    <td>%s</td>\n' % column
    table_data += '  </tr>\n'

  # Write the last "TOTAL" row.
  table_data += '  <tr style="font-weight:bold">\n'
  for column in report_lines[-1].split():
    table_data += '    <td>%s</td>\n' % column
  table_data += '  </tr>\n'
  table_data += '</table>\n'

  return REPORT_TEMPLATE.format(table_data=table_data)


def generate_sources(command, output_dir, source_dir, coverage_file):
  llvm_cov_command = '%s show -format=html %s -instr-profile=%s' % (
      LLVM_COV_PATH, command.split()[0], coverage_file)

  data = os.popen(llvm_cov_command).read()

  # Extract css style from the data.
  style_start = data.find(STYLE_START_MARKER)
  style_end = data.find(STYLE_END_MARKER)
  if style_end <= style_start or style_start == -1:
    logging.error('Failed to extract CSS style from coverage report.')
    raise Exception('Failed to process coverage dump.')

  style_data = data[style_start + len(STYLE_START_MARKER) : style_end]
  with open(os.path.join(output_dir, STYLE_FILENAME), 'w') as file_handle:
    file_handle.write(style_data)

  # Extract every single source code file.
  while True:
    file_start = data.find(SINGLE_FILE_START_MARKER)
    if file_start == -1:
      break

    file_end = data.find(SINGLE_FILE_END_MARKER)
    if file_end == -1:
      break

    file_end += len(SINGLE_FILE_END_MARKER)

    file_data = data[file_start : file_end]
    data = data[file_end : ]

    # Remove <style> as it's always the same and has been extracted separately.
    file_data = replace_style_with_css(file_data, STYLE_FILENAME)

    filename, file_data = extract_and_fix_filename(file_data, source_dir)
    filepath = os.path.join(output_dir, filename)
    dirname = os.path.dirname(filepath)

    try:
      os.makedirs(dirname)
    except OSError:
      pass

    with open(filepath + HTML_FILE_EXTENSION, 'w') as file_handle:
      file_handle.write(file_data)


def generate_summary(command, output_dir, coverage_file):
  llvm_cov_command = '%s report %s -instr-profile=%s' % (LLVM_COV_PATH,
                                                         command.split()[0],
                                                         coverage_file)
  data = os.popen(llvm_cov_command).read()
  report = generate_report(data)

  with open(os.path.join(output_dir, REPORT_FILENAME), 'w') as file_handle:
    file_handle.write(report)


def process_coverage_dump(command, profile_file, coverage_file):
  binary_path = command.split()[0]
  if not os.path.exists(binary_path):
    logging.error('Cannot find the executable file: "%s".' % binary_path)
    raise Exception('Failed to process coverage dump.')


  merge_command = '%s merge -sparse %s -o %s' % (LLVM_PROFDATA_PATH,
                                                 profile_file, coverage_file)
  os.popen(merge_command)

  if not os.path.exists(coverage_file) or not os.path.getsize(coverage_file):
    logging.error('%s is either not created or empty.' % coverage_file)
    raise Exception('Failed to merge coverage information after command run.')


def replace_style_with_css(data, css_file_path):
  style_start = data.find(STYLE_START_MARKER)
  style_end = data.find(STYLE_END_MARKER)
  if style_end <= style_start or style_start == -1:
    logging.error('Failed to extract CSS style from coverage report.')
    raise Exception('Failed to process coverage dump.')

  style_end += len(STYLE_END_MARKER)

  css_include = (
      '<link rel="stylesheet" type="text/css" href="/%s">' % css_file_path)
  data = '\n'.join([ data[ : style_start], css_include, data[style_end : ] ])
  return data


def run_command(command, profile_file):
  logging.info('Running "%s".' % command)
  os.environ['LLVM_PROFILE_FILE'] = profile_file
  print(os.popen(command).read())
  logging.info('Finished command execution.')

  if not os.path.exists(profile_file) or not os.path.getsize(profile_file):
    logging.error('%s is either not created or empty.' % profile_file)
    raise Exception('Failed to dump coverage information during command run.')


def main():
  parser = argparse.ArgumentParser(description='Code coverage helper script.')
  parser.add_argument('--command', required=True,
                      help='The command to generate code coverage of')
  parser.add_argument('--source', required=True,
                      help='Directory with the source code (e.g. chromium/src)')
  parser.add_argument('--output', required=True,
                      help='Directory where result will be written to.')

  args = parser.parse_args()

  create_output_dir(args.output)
  profile_file = os.path.join(args.output, LLVM_PROFILE_FILE_NAME)
  run_command(args.command, profile_file)

  coverage_file = os.path.join(args.output, LLVM_COVERAGE_FILE_NAME)
  process_coverage_dump(args.command, profile_file, coverage_file)

  generate_summary(args.command, args.output, coverage_file)
  generate_sources(args.command, args.output, args.source, coverage_file)

  print('Done. The next steps would be:\n'
        '1. cd %s && python -m SimpleHTTPServer\n'
        '2. open http://127.0.0.1:8000/report.html' % args.output)


if __name__ == "__main__":
  main()
