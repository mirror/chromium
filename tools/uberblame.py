#!/usr/bin/env python
# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import cgi
import codecs
import subprocess
import sys
import tempfile
import webbrowser


class TokenContext(object):
  """Metadata about a token.

  Attributes:
    row: Row index of the token in the data file.
    column: Column index of the token in the data file.
    length: Number of characters in the token.
    commit: Hash of the git commit that added this token.
  """
  def __init__(self, row, column, length, commit=None):
    self.row = row
    self.column = column
    self.length = length
    self.commit = commit


def tokenize_data(data, context):
  """Tokenizes |data|.

  Args:
    data: String to tokenize.
    context: Indicates that we want token metadata rather than token strings.

  Returns:
    If |context| is False, returns a list of token strings.  Otherwise,
    returns a list of TokenContexts.
  """
  tokens = []
  in_identifier = False
  identifier_start = 0
  identifier = u''
  row = 0
  column = 0

  for c in data + ' ':
    if c.isalnum() or c == u'_':
      if in_identifier:
        identifier += c
      else:
        in_identifier = True
        identifier_start = column
        identifier = c
    else:
      if in_identifier:
        if context:
          tokens.append(TokenContext(row, identifier_start, len(identifier)))
        else:
          tokens.append(identifier)
      in_identifier = False
      if not c.isspace():
        if context:
          tokens.append(TokenContext(row, column, 1))
        else:
          tokens.append(c)

    if c == u'\n':
      row += 1
      column = 0
    else:
      column += 1
  return tokens


def compute_unified_diff(old_data, new_data):
  """Computes the diff between |old_data| and |new_data|.

  Args:
    old_data: A string containing the old file.
    new_data: A string containing the new file.

  Returns:
    The diff, in unified diff format.
  """
  old_data_file = tempfile.NamedTemporaryFile()
  old_data_file.write(old_data)
  old_data_file.flush()

  cmd_diff = ['diff', '-d', '-u0', old_data_file.name, '-']
  diff = subprocess.Popen(cmd_diff,
                          stdin=subprocess.PIPE,
                          stdout=subprocess.PIPE,
                          stderr=subprocess.PIPE)
  stdout, stderr = diff.communicate(new_data)

  old_data_file.close()
  # diff return codes:
  # 0: No differences found.
  # 1: Some differences found.
  # 2: Error.
  if diff.returncode != 0 and diff.returncode != 1:
    raise subprocess.CalledProcessError(diff.returncode, cmd_diff, stderr)
  return stdout


def apply_patch(data, diff):
  """Patches |data| with |diff|.

  Args:
    data: A string containing the old file.
    diff: A string containing the diff file.

  Returns:
    The patched file as a string.
  """
  data_file = tempfile.NamedTemporaryFile()
  data_file.write(data)
  data_file.flush()

  cmd_patch = ['patch', '-r', '-', '-o', '-', data_file.name]
  patch = subprocess.Popen(cmd_patch,
                           stdin=subprocess.PIPE,
                           stdout=subprocess.PIPE,
                           stderr=subprocess.PIPE)
  stdout, stderr = patch.communicate(diff)

  data_file.close()

  if patch.returncode != 0:
    raise subprocess.CalledProcessError(patch.returncode, cmd_patch, stderr)
  return stdout


def parse_chunk_header_file_range(file_range):
  """Parses a chunk header file range.

  Diff chunk headers have the form:
    @@ -<file-range> +<file-range> @@
  File ranges have the form:
    <start line number>,<number of lines changed>

  Args:
    file_range: A chunk header file range.

  Returns:
    A tuple (range_start, range_end).  The endpoints are adjusted such that
    iterating over [range_start, range_end) will give the changed indices.
  """
  if ',' in file_range:
    file_range_parts = file_range.split(',')
    start = int(file_range_parts[0])
    amount = int(file_range_parts[1])
    if amount == 0:
      return (start, start)
    return (start - 1, start + amount - 1)
  else:
    return (int(file_range) - 1, int(file_range))


def compute_changed_token_indices(previous_tokens, current_tokens):
  """Computes changed and added tokens.

  Args:
    previous_tokens: Tokens corresponding to the old file.
    current_tokens: Tokens corresponding to the new file.

  Returns:
    A tuple (added_tokens, changed_tokens).
      added_tokens: A list of indices into |current_tokens|.
      changed_tokens: A map of indices into |current_tokens| to
        indices into |previous_tokens|.
  """
  prev_file_chunk_end = 0
  prev_patched_chunk_end = 0
  added_tokens = []
  changed_tokens = {}
  for line in compute_unified_diff('\n'.join(previous_tokens),
                                   '\n'.join(current_tokens)).split('\n'):
    line = line.strip()
    if line == '':
      continue
    if line.startswith("@@") and line.endswith("@@"):
      parts = line.split(' ')
      removed = parts[1].lstrip('-')
      removed_start, removed_end = parse_chunk_header_file_range(removed)
      added = parts[2].lstrip('+')
      added_start, added_end = parse_chunk_header_file_range(added)
      for i in range(added_start, added_end):
        added_tokens.append(i)
      for i in range(0, removed_start - prev_patched_chunk_end):
        changed_tokens[prev_file_chunk_end + i] = prev_patched_chunk_end + i
      prev_patched_chunk_end = removed_end
      prev_file_chunk_end = added_end
  for i in range(0, len(previous_tokens) - prev_patched_chunk_end):
    changed_tokens[prev_file_chunk_end + i] = prev_patched_chunk_end + i
  return added_tokens, changed_tokens


def uberblame_aux(file_name, git_log_stdout):
  current_data = codecs.open(file_name, 'r', 'utf-8').read()
  current_tokens = tokenize_data(current_data, False)
  current_blame = tokenize_data(current_data, True)

  uber_blame = (current_data, current_blame)

  previous_commit = None
  previous_file_name = None
  while True:
    try:
      commit = git_log_stdout.next().rstrip('\n').strip('"')
      file_name = git_log_stdout.next().rstrip('\n')
    except StopIteration:
      break
    # Git log separates commit data by blank lines, except for the last commit.
    try:
      git_log_stdout.next()
    except StopIteration:
      pass

    if previous_commit != None:
      cmd_git_diff = ['git', 'diff', previous_commit, commit, '--',
                      previous_file_name, file_name]
      git_diff = subprocess.Popen(cmd_git_diff,
                                  stdout=subprocess.PIPE,
                                  stderr=subprocess.PIPE)
      diff, stderr = git_diff.communicate()
      if git_diff.returncode != 0:
        raise subprocess.CalledProcessError(git_diff.returncode, cmd_git_diff,
                                            stderr)

      previous_data = apply_patch(current_data, diff)
      previous_tokens = tokenize_data(previous_data, False)
      added_tokens, changed_tokens = compute_changed_token_indices(
          previous_tokens, current_tokens)
      for added_token_index in added_tokens:
        token_context = current_blame[added_token_index]
        if token_context != None:
          token_context.commit = previous_commit

      previous_blame = [None] * len(previous_tokens)
      for current_blame_index in changed_tokens:
        previous_blame_index = changed_tokens[current_blame_index]
        previous_blame[previous_blame_index] = (
            current_blame[current_blame_index])

      current_data = previous_data
      current_tokens = previous_tokens
      current_blame = previous_blame

    previous_commit = commit
    previous_file_name = file_name
  for token_context in current_blame:
    if token_context != None:
      token_context.commit = previous_commit
  return uber_blame


def uberblame(file_name):
  """Computes the uberblame of file |file_name|.

  Args:
    file_name: File to uberblame.

  Returns:
    A tuple (data, blame).
      data: File contents.
      blame: A list of TokenContexts.
  """
  cmd_git_log = ['git', 'log', '--follow', '--pretty=format:"%h"',
                 '--stat', '--name-only', file_name]
  git_log = subprocess.Popen(cmd_git_log,
                             stdout=subprocess.PIPE,
                             stderr=subprocess.PIPE)
  try:
    data, blame = uberblame_aux(file_name, git_log.stdout)
  except Exception as exception:
    git_log.kill()
    raise exception

  _, stderr = git_log.communicate()
  if git_log.returncode != 0:
    raise subprocess.CalledProcessError(git_log.returncode, cmd_git_log, stderr)
  return data, blame


def visualize_uberblame(data, blame):
  """Creates and displays a web page to visualize |blame|.

  Args:
    data: The data file as returned by uberblame().
    blame: A list of TokenContexts as returned by uberblame().
  """
  html = [ '<html><head><style>' ]
  html.append('body {font-family: "Courier New";}')
  html.append('pre { display: inline; }')
  html.append('</style></head><body><pre>')
  blame_index = 0
  row = 0
  for line in data.split('\n'):
    column = 0
    in_link = False
    for c in line.rstrip('\n'):
      if blame_index < len(blame):
        token_context = blame[blame_index]
        if (row == token_context.row and
            column == token_context.column + token_context.length):
          html.append('</a>')
          in_link = False
          blame_index += 1
        token_context = blame[blame_index]
        if row == token_context.row and column == token_context.column:
          html.append(('<a href="https://chromium.googlesource.com/' +
                       'chromium/src/+/%s">') % token_context.commit)
          in_link = True
      html.append(cgi.escape(c))
      column += 1
    if in_link:
      html.append('</a>')
      blame_index += 1
    html.append('\n')
    row += 1
  html.append('</pre></body></html>')
  # Keep the temporary file around so the browser has time to open it.
  # TODO(thomasanderson): spin up a temporary web server to serve this
  # file so we don't have to leak it.
  html_file = tempfile.NamedTemporaryFile(delete=False, suffix='.html')
  html_file.write(''.join(html))
  html_file.flush()
  webbrowser.open('file://' + html_file.name)


def main():
  if len(sys.argv) != 2:
    print 'Usage: %s file' % sys.argv[0]
    sys.exit(1)

  file_name = sys.argv[1]
  data, blame = uberblame(file_name)
  visualize_uberblame(data, blame)
  return 0


if __name__ == '__main__':
  sys.exit(main())
