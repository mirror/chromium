#!/usr/bin/python
# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Runs Closure compiler on JavaScript files to check for errors and produce
minified output.

This whole file is a fork of and old version of
//third_party/closure_compiler/compile2.py.
"""

import argparse
import logging
import os
import re
import subprocess
import sys


class Checker(object):
  """Runs the Closure compiler on given source files to typecheck them
  and produce minified output."""

  _JAR_COMMAND = [
    "java",
    "-jar",
    "-Xms1024m",
    "-client",
    "-XX:+TieredCompilation"
  ]

  def __init__(self, compiler_jar, verbose=False):
    """
    Args:
      verbose: Whether this class should output diagnostic messages.
    """
    self._compiler_jar = compiler_jar
    self._verbose = verbose
    #self._error_filter = error_filter.PromiseErrorFilter()

  def _run_jar(self, jar, args):
    """Runs a .jar from the command line with arguments.

    Args:
      jar: A file path to a .jar file
      args: A list of command line arguments to be passed when running the .jar.

    Return:
      (exit_code, stderr) The exit code of the command (e.g. 0 for success) and
          the stderr collected while running |jar| (as a string).
    """
    shell_command = " ".join(self._JAR_COMMAND + [jar] + args)
    logging.info("Running jar: %s", shell_command)

    devnull = open(os.devnull, "w")
    kwargs = {"stdout": devnull, "stderr": subprocess.PIPE, "shell": True}
    process = subprocess.Popen(shell_command, **kwargs)
    _, stderr = process.communicate()
    return process.returncode, stderr

  def _run_js_check(
      self, sources,
      module_output_path_prefix,
      externs=None, closure_args=None):
    """Check |sources| for type errors.

    Args:
      sources: Files to check.
      externs: @extern files that inform the compiler about custom globals.
      closure_args: Arguments passed directly to the Closure compiler.

    Returns:
      (errors, stderr) A parsed list of errors (strings) found by the compiler
          and the raw stderr (as a string).
    """
    args = ["--js=%s" % s for s in sources]

    assert module_output_path_prefix
    args += ["--module_output_path_prefix={}".format(
        module_output_path_prefix)]

    args += ["--externs=%s" % e for e in externs or []]

    closure_args = closure_args or []
    closure_args += ["summary_detail_level=3", "continue_after_errors"]
    args += ["--%s" % arg for arg in closure_args]

    _, stderr = self._run_jar(self._compiler_jar, args)

    errors = stderr.strip().split("\n\n")
    maybe_summary = errors.pop()

    errors = [e for e in errors if ": ERROR -" in e]

    if re.search("[0-9]+ error.*[0-9]+ warning", maybe_summary):
      logging.info("%s", stderr.strip())
    else:
      # Missing summary. Running the jar failed. Bail.
      logging.error("No summary on stderr:\n" + stderr)
      sys.exit(1)

    return errors, stderr

  def check_multiple(
      self,
      sources,
      module_output_path_prefix=None,
      externs=None,
      closure_args=None):
    """Closure compile a set of files and check for errors.

    Args:
      sources: An array of files to check.
      externs: @extern files that inform the compiler about custom globals.
      closure_args: Arguments passed directly to the Closure compiler.

    Returns:
      (found_errors, stderr) A boolean indicating whether errors were found and
          the raw Closure Compiler stderr (as a string).
    """
    errors, stderr = self._run_js_check(
        sources,
        module_output_path_prefix=module_output_path_prefix,
        externs=externs,
        closure_args=closure_args)
    return bool(errors), stderr


if __name__ == "__main__":
  parser = argparse.ArgumentParser(
      description="Typecheck JavaScript using Closure compiler")
  parser.add_argument(
      "sources", nargs=argparse.ONE_OR_MORE,
      help="Path to a source file to typecheck")
  parser.add_argument("-e", "--externs", nargs=argparse.ZERO_OR_MORE)
  parser.add_argument(
      "-m", "--module_output_path_prefix",
      help="A file where the compiled output is written to",
      required=True)
  parser.add_argument(
      "-c", "--closure_args", nargs=argparse.ZERO_OR_MORE,
      help="Arguments passed directly to the Closure compiler")
  parser.add_argument(
      "-v", "--verbose", action="store_true",
      help="Show more information as this script runs")
  parser.add_argument("--compiler_jar")

  opts = parser.parse_args()

  logging.basicConfig(
      format="(%(levelname)s) %(message)s",
      level=logging.INFO if opts.verbose else logging.WARNING)

  externs = opts.externs or []

  checker = Checker(compiler_jar=opts.compiler_jar, verbose=opts.verbose)
  found_errors, stderr = checker.check_multiple(
      opts.sources,
      module_output_path_prefix=opts.module_output_path_prefix,
      externs=externs, closure_args=opts.closure_args)
  if found_errors:
    print stderr
    sys.exit(1)
