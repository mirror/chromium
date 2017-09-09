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

class BlameContext(object):
  def __init__(self, token, row, column, commit=None):
    self.token = token
    self.row = row
    self.column = column
    self.commit = commit

  def __str__(self):
    return "BlameContext(%s, %s, %s, %s)" % (
        self.token, self.row, self.column, self.commit)

def tokenize_file(file_contents):
  tokens = []
  in_identifier = False
  identifier_start = 0
  identifier = u''
  row = 0
  column = 0
  for c in file_contents + u' ':
    if c.isalnum() or c == u'_':
      if in_identifier:
        identifier += c
      else:
        in_identifier = True
        identifier_start = column
        identifier = c
    elif c.isspace():
      if in_identifier:
        tokens.append(BlameContext(identifier, row, identifier_start))
      in_identifier = False
    else:
      if in_identifier:
        tokens.append(BlameContext(identifier, row, identifier_start))
      in_identifier = False
      tokens.append(BlameContext(c, row, column))

    if c == u'\n':
      row += 1
      column = 0
    else:
      column += 1
  return tokens

def get_diff(contents1, contents2):
  file1 = tempfile.NamedTemporaryFile()
  file1.write(contents1)
  file1.flush()
  file2 = tempfile.NamedTemporaryFile()
  file2.write(contents2)
  file2.flush()

  cmd_diff = subprocess.Popen(['diff', '-u0', file1.name, file2.name],
                              stdout=subprocess.PIPE)
  stdout, _ = cmd_diff.communicate()
  return stdout

def get_range_from_hunk_header(part):
  if ',' in part:
    part_parts = part.split(',')
    start = int(part_parts[0])
    amount = int(part_parts[1])
    if amount == 0:
      return (start, start)
    return (start - 1, start + amount - 1)
  else:
    return (int(part) - 1, int(part))

def get_index_changes(patched_tokens, file_tokens):
  prev_file_hunk_end = 0
  prev_patched_hunk_end = 0
  removed_lines = []
  added_lines = []
  changed_lines = {}
  for line in get_diff('\n'.join(patched_tokens),
                       '\n'.join(file_tokens)).split('\n'):
    line = line.strip()
    if line == '':
      continue
    if line.startswith("@@") and line.endswith("@@"):
      parts = line.split(' ')
      removed = parts[1].lstrip('-')
      removed_start, removed_end = get_range_from_hunk_header(removed)
      added = parts[2].lstrip('+')
      added_start, added_end = get_range_from_hunk_header(added)
      for i in range(removed_start, removed_end):
        removed_lines.append(i)
      for i in range(added_start, added_end):
        added_lines.append(i)
      for i in range(0, removed_start - prev_patched_hunk_end):
        changed_lines[prev_file_hunk_end + i] = prev_patched_hunk_end + i
      prev_patched_hunk_end = removed_end
      prev_file_hunk_end = added_end
  for i in range(0, len(patched_tokens) - prev_patched_hunk_end):
    changed_lines[prev_file_hunk_end + i] = prev_patched_hunk_end + i
  return removed_lines, added_lines, changed_lines

def uberblame(filename):
  file_contents = codecs.open(filename, 'r', 'utf-8').read()
  file_blame = tokenize_file(file_contents)
  file_tokens = [context.token for context in file_blame]

  uber_blame = (file_contents, file_blame)

  prev_commit = None
  prev_file_name = None
  git_log = subprocess.Popen(['git', 'log', '--follow', '--pretty=format:"%h"',
                              '--stat', '--name-only', filename],
                             stdout=subprocess.PIPE)
  while True:
    commit = git_log.stdout.readline().rstrip().strip('"')
    if commit == u'':
      break
    file_name = git_log.stdout.readline().rstrip()
    git_log.stdout.readline()

    if prev_commit != None:
      git_diff = subprocess.Popen(['git', 'diff', prev_commit, commit, '--',
                                   prev_file_name, file_name],
                                  stdout=subprocess.PIPE)
      diff, _ = git_diff.communicate()
      if git_diff.returncode != 0:
        git_log.kill()
        sys.exit(1)

      file_at_commit = tempfile.NamedTemporaryFile()
      file_at_commit.write(file_contents)
      file_at_commit.flush()

      cmd_patch = subprocess.Popen(['patch', '-o', '-', file_at_commit.name],
                                   stdin=subprocess.PIPE,
                                   stdout=subprocess.PIPE,
                                   stderr=subprocess.PIPE)
      patched, _ = cmd_patch.communicate(diff)

      file_at_commit.close()

      if cmd_patch.returncode != 0:
        git_log.kill()
        sys.exit(1)

      patched_tokens = [context.token for context in tokenize_file(patched)]
      removed_lines, added_lines, changed_lines = get_index_changes(
          patched_tokens, file_tokens)
      for added_line_index in added_lines:
        blame_context = file_blame[added_line_index]
        if blame_context != None:
          blame_context.commit = prev_commit

      patched_file_blame = [None]*len(patched_tokens)
      for file_index in changed_lines:
        patched_index = changed_lines[file_index]
        patched_file_blame[patched_index] = file_blame[file_index]

      file_contents = patched
      file_tokens = patched_tokens
      file_blame = patched_file_blame

    prev_commit = commit
    prev_file_name = file_name
  for blame_context in file_blame:
    if blame_context != None:
      blame_context.commit = prev_commit
  return uber_blame

def visualize_uberblame(ub):
  file_contents, file_blame = ub
  html = [ '<html><head><style>' ]
  html.append('body {font-family: "Courier New";}')
  html.append('pre { display: inline; }')
  html.append('</style></head><body>')
  blame_index = 0
  row = 0
  for line in file_contents.split('\n'):
    column = 0
    in_link = False
    html.append('<pre>')
    for c in line.rstrip('\n'):
      if blame_index < len(file_blame):
        blame_context = file_blame[blame_index]
        if (row == blame_context.row and
            column == blame_context.column + len(blame_context.token)):
          html.append('</a>')
          in_link = False
          blame_index += 1
        blame_context = file_blame[blame_index]
        if row == blame_context.row and column == blame_context.column:
          html.append(('<a href="https://chromium.googlesource.com/' +
                       'chromium/src/+/%s">') % blame_context.commit)
          in_link = True
      html.append(cgi.escape(c))
      column += 1
    if in_link:
      html.append('</a>')
      blame_index += 1
    html.append('</pre><br>')
    row += 1
  html.append('</body></html>')
  # Keep the temporary file around so the browser has time to open it.
  # TODO(thomasanderson): spin up a temporary web server to serve this
  # file so we don't leak this file.
  html_file = tempfile.NamedTemporaryFile(delete=False, suffix='.html')
  html_file.write(''.join(html))
  html_file.flush()
  webbrowser.open('file://' + html_file.name)

def main():
  if len(sys.argv) != 2:
    print 'Usage: %s file' % sys.argv[0]
    sys.exit(1)

  filename = sys.argv[1]
  visualize_uberblame(uberblame(filename))
  return 0

if __name__ == '__main__':
  sys.exit(main())
