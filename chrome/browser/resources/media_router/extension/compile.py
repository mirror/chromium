#!/usr/bin/python
# Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Runs Closure compiler on JavaScript files to check for errors and produce
minified output.

This whole file is a fork of and old version of
//third_party/closure_compiler/compile2.py.

TODO(jrw): Clean up dead and commented-out code, or port the necessary
functionality to the current version of compile2.py.
"""

import argparse
import logging
import os
import re
import subprocess
import sys
import tempfile

# import build.inputs
# import processor
# import error_filter


_CURRENT_DIR = os.path.join(os.path.dirname(__file__))


def RemoveDuplicates(items):
  "Removes duplicate items from a list."
  seen = set()
  result = []
  for item in items:
    if item not in seen:
      result.append(item)
      seen.add(item)
  return result


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

  _MAP_FILE_FORMAT = "%s.map"

  def __init__(self, compiler_jar, verbose=False):
    """
    Args:
      verbose: Whether this class should output diagnostic messages.
    """
    self._compiler_jar = compiler_jar
    self._temp_files = []
    self._verbose = verbose
    #self._error_filter = error_filter.PromiseErrorFilter()

  def _nuke_temp_files(self):
    """Deletes any temp files this class knows about."""
    if not self._temp_files:
      return

    self._log_debug("Deleting temp files: %s" % ", ".join(self._temp_files))
    for f in self._temp_files:
      os.remove(f)
    self._temp_files = []

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

  def _get_line_number(self, match):
    """When chrome is built, it preprocesses its JavaScript from:

      <include src="blah.js">
      alert(1);

    to:

      /* contents of blah.js inlined */
      alert(1);

    Because Closure Compiler requires this inlining already be done (as
    <include> isn't valid JavaScript), this script creates temporary files to
    expand all the <include>s.

    When type errors are hit in temporary files, a developer doesn't know the
    original source location to fix. This method maps from /tmp/file:300 back to
    /original/source/file:100 so fixing errors is faster for developers.

    Args:
      match: A re.MatchObject from matching against a line number regex.

    Returns:
      The fixed up /file and :line number.
    """
    real_file = self._processor.get_file_from_line(match.group(1))
    return "%s:%d" % (os.path.abspath(real_file.file), real_file.line_number)

  def _filter_errors(self, errors):
    """Removes some extraneous errors. For example, we ignore:

      Variable x first declared in /tmp/expanded/file

    Because it's just a duplicated error (it'll only ever show up 2+ times).
    We also ignore Promose-based errors:

      found   : function (VolumeInfo): (Promise<(DirectoryEntry|null)>|null)
      required: (function (Promise<VolumeInfo>): ?|null|undefined)

    as templates don't work with Promises in all cases yet. See
    https://github.com/google/closure-compiler/issues/715 for details.

    Args:
      errors: A list of string errors extracted from Closure Compiler output.

    Return:
      A slimmer, sleeker list of relevant errors (strings).
    """
    first_declared_in = lambda e: " first declared in " not in e
    return self._error_filter.filter(filter(first_declared_in, errors))

  def _clean_up_error(self, error):
    """Reverse the effects that funky <include> preprocessing steps have on
    errors messages.

    Args:
      error: A Closure compiler error (2 line string with error and source).

    Return:
      The fixed up error string.
    """
    expanded_file = self._expanded_file
    fixed = re.sub("%s:(\d+)" % expanded_file, self._get_line_number, error)
    return fixed.replace(expanded_file, os.path.abspath(self._file_arg))

  def _format_errors(self, errors):
    """Formats Closure compiler errors to easily spot compiler output.

    Args:
      errors: A list of strings extracted from the Closure compiler's output.

    Returns:
      A formatted output string.
    """
    contents = "\n## ".join("\n\n".join(errors).splitlines())
    return "## %s" % contents if contents else ""

  def _create_temp_file(self, contents):
    """Creates an owned temporary file with |contents|.

    Args:
      content: A string of the file contens to write to a temporary file.

    Return:
      The filepath of the newly created, written, and closed temporary file.
    """
    with tempfile.NamedTemporaryFile(mode="wt", delete=False) as tmp_file:
      self._temp_files.append(tmp_file.name)
      tmp_file.write(contents)
    return tmp_file.name

  def _run_js_check(
      self, sources, out_file,
      module_output_path_prefix,
      externs=None, closure_args=None):
    """Check |sources| for type errors.

    Args:
      sources: Files to check.
      out_file: A file where the compiled output is written to.
      externs: @extern files that inform the compiler about custom globals.
      closure_args: Arguments passed directly to the Closure compiler.

    Returns:
      (errors, stderr) A parsed list of errors (strings) found by the compiler
          and the raw stderr (as a string).
    """
    args = ["--js=%s" % s for s in sources]

    if out_file:
      out_dir = os.path.dirname(out_file)
      if out_dir and not os.path.exists(out_dir):
        os.makedirs(out_dir)

    checks_only = 'checks_only' in closure_args

    if not checks_only:
      if out_file:
        args += ["--js_output_file=%s" % out_file]
        args += ["--create_source_map=%s" % (self._MAP_FILE_FORMAT % out_file)]
      else:
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
      self._nuke_temp_files()
      sys.exit(1)

    if errors and out_file:
      if os.path.exists(out_file):
        os.remove(out_file)
      if os.path.exists(self._MAP_FILE_FORMAT % out_file):
        os.remove(self._MAP_FILE_FORMAT % out_file)
    elif checks_only:
      # Compile succeeded but --checks_only disables --js_output_file from
      # actually writing a file. Write a file ourselves so incremental builds
      # still work.
      with open(out_file, 'w') as f:
        f.write('')

    return errors, stderr

  def check(
      self,
      source_file,
      out_file=None,
      depends=None,
      externs=None,
      closure_args=None):
    """Closure compiler |source_file| while checking for errors.

    Args:
      source_file: A file to check.
      out_file: A file where the compiled output is written to.
      depends: Files that |source_file| requires to run (e.g. earlier <script>).
      externs: @extern files that inform the compiler about custom globals.
      closure_args: Arguments passed directly to the Closure compiler.

    Returns:
      (found_errors, stderr) A boolean indicating whether errors were found and
          the raw Closure compiler stderr (as a string).
    """
    logging.info("FILE: %s", source_file)

    if source_file.endswith("_externs.js"):
      logging.info("Skipping externs: %s", source_file)
      return

    self._file_arg = source_file

    cwd, tmp_dir = os.getcwd(), tempfile.gettempdir()
    rel_path = lambda f: os.path.join(os.path.relpath(cwd, tmp_dir), f)

    depends = depends or []
    includes = [rel_path(f) for f in depends + [source_file]]
    contents = ['<include src="%s">' % i for i in includes]
    meta_file = self._create_temp_file("\n".join(contents))
    logging.info("Meta file: %s", meta_file)

    self._processor = processor.Processor(meta_file)
    self._expanded_file = self._create_temp_file(self._processor.contents)
    logging.info("Expanded file: %s", self._expanded_file)

    errors, stderr = self._run_js_check(
        [self._expanded_file],
        out_file=out_file,
        module_output_path_prefix=module_output_path_prefix,
        externs=externs,
        closure_args=closure_args)
    filtered_errors = self._filter_errors(errors)
    cleaned_errors = map(self._clean_up_error, filtered_errors)
    output = self._format_errors(cleaned_errors)

    if cleaned_errors:
      prefix = "\n" if output else ""
      logging.error("Error in: %s%s%s", source_file, prefix, output)
    elif output:
      logging.info("Output: %s", output)

    self._nuke_temp_files()
    return bool(cleaned_errors), stderr

  def check_multiple(
      self,
      sources,
      out_file=None,
      module_output_path_prefix=None,
      externs=None,
      closure_args=None):
    """Closure compile a set of files and check for errors.

    Args:
      sources: An array of files to check.
      out_file: A file where the compiled output is written to.
      externs: @extern files that inform the compiler about custom globals.
      closure_args: Arguments passed directly to the Closure compiler.

    Returns:
      (found_errors, stderr) A boolean indicating whether errors were found and
          the raw Closure Compiler stderr (as a string).
    """
    errors, stderr = self._run_js_check(
        sources, out_file=out_file,
        module_output_path_prefix=module_output_path_prefix,
        externs=externs,
        closure_args=closure_args)
    self._nuke_temp_files()
    return bool(errors), stderr


if __name__ == "__main__":
  parser = argparse.ArgumentParser(
      description="Typecheck JavaScript using Closure compiler")
  parser.add_argument("sources", nargs=argparse.ONE_OR_MORE,
                      help="Path to a source file to typecheck")
  single_file_group = parser.add_mutually_exclusive_group()
  single_file_group.add_argument(
      "--single_file", dest="single_file",
      action="store_true",
      help="Process each source file individually")
  # TODO(twellington): remove --no_single_file and use
  # len(opts.sources).
  single_file_group.add_argument(
      "--no_single_file", dest="single_file",
      action="store_false",
      help="Process all source files as a group")
  parser.add_argument("-d", "--depends", nargs=argparse.ZERO_OR_MORE)
  parser.add_argument("-e", "--externs", nargs=argparse.ZERO_OR_MORE)
  output_group = parser.add_mutually_exclusive_group(required=True)
  output_group.add_argument(
      "-o", "--out_file",
      help="A file where the compiled output is written to")
  output_group.add_argument(
      "-m", "--module_output_path_prefix",
      help="A file where the compiled output is written to")
  parser.add_argument(
      "-c", "--closure_args", nargs=argparse.ZERO_OR_MORE,
      help="Arguments passed directly to the Closure compiler")
  parser.add_argument(
      "-v", "--verbose", action="store_true",
      help="Show more information as this script runs")
  parser.add_argument("--compiler_jar")

  parser.set_defaults(single_file=True)
  opts = parser.parse_args()

  logging.basicConfig(
      format="(%(levelname)s) %(message)s",
      level=logging.INFO if opts.verbose else logging.WARNING)

  depends = opts.depends or []
  externs = opts.externs or []
  sources = opts.sources
  if not opts.module_output_path_prefix:
    # TODO(devlin): should we run normpath() on this first and/or do this for
    # depends as well?
    externs = RemoveDuplicates(externs)
    sources = RemoveDuplicates(sources)

  checker = Checker(compiler_jar=opts.compiler_jar, verbose=opts.verbose)
  if opts.single_file:
    for source in sources:
      # Normalize source to the current directory.
      source = os.path.normpath(os.path.join(os.getcwd(), source))
      depends, externs = build.inputs.resolve_recursive_dependencies(
          source, depends, externs)

      found_errors, _ = checker.check(
          source, out_file=opts.out_file,
          depends=depends, externs=externs,
          closure_args=opts.closure_args)
      if found_errors:
        sys.exit(1)
  else:
    found_errors, stderr = checker.check_multiple(
        sources, out_file=opts.out_file,
        module_output_path_prefix=opts.module_output_path_prefix,
        externs=externs, closure_args=opts.closure_args)
    if found_errors:
      print stderr
      sys.exit(1)
