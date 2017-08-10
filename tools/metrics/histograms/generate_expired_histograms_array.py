#!/usr/bin/env python
# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import argparse
import datetime
import hashlib
import logging
import os
import re
import sys

from tools.metrics.histograms import extract_histograms
from tools.metrics.histograms import merge_xml

_SCRIPT_NAME = "generate_expired_histograms_array.py"
_HEADER = """// Generated from {script_name}. Do not edit!

#ifndef {include_guard}
#define {include_guard}

namespace {namespace} {{

// Contains hashes of expired histograms.
const {hash_datatype} kExpiredHistogramsHashes[] = {{
{hashes}
}};

const size_t kNumExpiredHistograms = {hashes_size};

}}  // namespace {namespace}

#endif  // {include_guard}
"""


class Error(Exception):
  pass


def _GetExpiredHistograms(histograms):
  """Filters histograms to find expired ones.

  Args:
    histograms(Dict[str, Dict]): hisotgram descriptions in form {name: content}.

  Returns:
    List of strings with names of expired histograms.
  """
  expired_histograms_names = []
  for name, content in histograms.items():
    if "obsolete" in content or "expiry_date" not in content:
      continue
    expiry_date_str = content["expiry_date"]
    try:
      expiry_date = datetime.datetime.strptime(
          expiry_date_str, extract_histograms.EXPIRY_DATE_PATTERN).date()
    except ValueError:
      logging.error("Unable to parse expiry date {date} in {name}".format(
          date=expiry_date_str, name=name))
      continue
    today = datetime.datetime.now().date()
    if expiry_date < today:
      expired_histograms_names.append(name)
  return expired_histograms_names


def _HashName(name):
  """Returns hash for the given histogram |name|."""
  return "0x" + hashlib.md5(name).hexdigest()[:16]


def _GetHashToNameMap(histograms_names):
  """Returns dictionary {hash: histogram_name}."""
  hash_to_name_map = dict()
  for name in histograms_names:
    hash_to_name_map[_HashName(name)] = name
  return hash_to_name_map


def _GenerateHeaderFileContent(header_filename, namespace, hash_datatype,
                               histograms_map):
  """Generates file content definig array with hashes of expired histograms."""
  include_guard = re.sub("[^A-Z]", "_", header_filename.upper()) + "_"
  hashes = "\n".join([
      "  {hash},  // {name}".format(hash=value, name=histograms_map[value])
      for value in sorted(histograms_map.keys())
  ])
  return _HEADER.format(
      script_name=_SCRIPT_NAME,
      include_guard=include_guard,
      namespace=namespace,
      hash_datatype=hash_datatype,
      hashes=hashes,
      hashes_size=len(histograms_map))


def _GenerateFile(arguments):
  descriptions = merge_xml.MergeFiles(arguments.inputs)
  histograms, had_errors = (
      extract_histograms.ExtractHistogramsFromDom(descriptions))
  if had_errors:
    logging.error("Error parsing inputs.")
    raise Error()
  expired_histograms_names = _GetExpiredHistograms(histograms)
  expired_histograms_map = _GetHashToNameMap(expired_histograms_names)
  header_file_content = _GenerateHeaderFileContent(
      arguments.header_filename, arguments.namespace, arguments.hash_datatype,
      expired_histograms_map)
  with open(os.path.join(arguments.output_dir, arguments.header_filename),
            "w") as generated_file:
    generated_file.write(header_file_content)


def _ParseArguments():
  arg_parser = argparse.ArgumentParser(
      description="Generate array of expired histograms' hashes.")
  arg_parser.add_argument(
      "--output_dir",
      "-o",
      required=True,
      help="Base directory to for generated files.")
  arg_parser.add_argument(
      "--header_filename",
      "-H",
      required=True,
      help="File name of the generated header file.")
  arg_parser.add_argument(
      "--hash_datatype",
      "-d",
      required=True,
      help="Datatype of a histogram hash.")
  arg_parser.add_argument(
      "--namespace",
      "-N",
      default="",
      help="Namespace of the generated factory function (code will be in "
      "the global namespace if this is omitted).")
  arg_parser.add_argument(
      "inputs",
      nargs="+",
      help="Paths to .xml files with histogram descriptions.")
  return arg_parser.parse_args()


def main():
  arguments = _ParseArguments()
  _GenerateFile(arguments)


if __name__ == "__main__":
  sys.exit(main())
