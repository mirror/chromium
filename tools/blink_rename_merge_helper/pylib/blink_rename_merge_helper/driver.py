# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import argparse
import hashlib
import os
import shutil
import subprocess
import sys
import tempfile

_BEFORE_RENAME_COMMIT = '5e27d4b8d16d9830e52a44a44b4ff501a2a2e667'
_RENAME_COMMIT = '1c4d759e44259650dfb2c426a7f997d2d0bc73dc'
_AFTER_RENAME_COMMIT = 'b0bf8e8ed34ba40acece03baa19446a5d91b009d'
_DEVNULL = open(os.devnull, 'w')
_SCRIPT_DIR = os.path.dirname(os.path.realpath(__file__))
_GIT_CONFIG_BRANCH_RECORDS = 'branch.%s.blink-rename-resolver-records'


# Helper to install a merge tool in a temporary scope.
class _MergeTool(object):

  def __init__(self):
    pass

  def __enter__(self):
    _check_call_git(
        ['config', 'merge.blink-rename.name', 'blink rename merge helper'])
    # Note: while it would be possible to encode the path to the records
    # directory here, it's easier to pass it as an environmental variable, to
    # avoid weird escaping issues.
    _check_call_git([
        'config', 'merge.blink-rename.driver',
        '%s %%O %%A %%B %%P' % os.path.join(_SCRIPT_DIR, 'merge.py')
    ])
    _check_call_git(['config', 'merge.blink-rename.recursive', 'binary'])

  def __exit__(self, exc_type, exc_value, traceback):
    _check_call_git(['config', '--remove-section', 'merge.blink-rename'])


def _call_gclient(args):
  if sys.platform == 'win32':
    args = ['gclient.bat'] + args
  else:
    args = ['gclient'] + args
  return subprocess.call(args)


def _call_gn(args):
  if sys.platform == 'win32':
    args = ['gn.exe'] + args
  else:
    args = ['gn'] + args
  # For GN, eat output, since only the return value is important.
  return subprocess.call(args, stdout=_DEVNULL, stderr=_DEVNULL)


def _call_ninja(args):
  if sys.platform == 'win32':
    args = ['ninja.exe'] + args
  else:
    args = ['ninja'] + args
  return subprocess.call(args)


def _build_git_command(args):
  if sys.platform == 'win32':
    return ['git.bat'] + args
  else:
    return ['git'] + args


def _call_git(args, **kwargs):
  return subprocess.call(_build_git_command(args), **kwargs)


def _check_call_git(args, **kwargs):
  return subprocess.check_call(_build_git_command(args), **kwargs)


def _check_call_git_and_get_output(args, **kwargs):
  return subprocess.check_output(_build_git_command(args), **kwargs)


def _is_clean_tree():
  return _call_git(['diff-index', '--quiet', 'HEAD']) == 0


def _ensure_clean_tree():
  if not _is_clean_tree():
    print 'ERROR: cannot proceed with a dirty tree. Please commit or stash '
    print '       changes.'
    # DO NOT COMMIT.
    return
    sys.exit(1)


def _get_branch_info():
  current_branch = _check_call_git_and_get_output(
      ['rev-parse', '--abbrev-ref', 'HEAD']).strip()
  print 'INFO: current branch: %s' % current_branch

  tracking_branch = None
  try:
    tracking_branch = _check_call_git_and_get_output(
        ['rev-parse', '--abbrev-ref',
         '%s@{upstream}' % current_branch]).strip()
  except subprocess.CalledProcessError:
    # Likely failed because there's no tracking branch info. Fall through and
    # fail out.
    pass
  if not tracking_branch:
    print 'ERROR: no tracking branch found. Bailing out...'
    sys.exit(1)

  print 'INFO: tracking branch: %s' % tracking_branch
  return current_branch, tracking_branch


def _commit_is_ancestor_of(ancestor, commit):
  # merge-base --is-ancestor returns 0 if |ancestor| is the ancestor of
  # |commit|.
  return _call_git(['merge-base', '--is-ancestor', ancestor, commit]) == 0


def _ensure_origin_contains_commit():
  if not _commit_is_ancestor_of(_RENAME_COMMIT, 'origin/master'):
    _check_call_git(['fetch', 'origin'])


def _prompt_yes_or_no(question, default='yes'):
  choices = {
      'yes': True,
      'y': True,
      'no': False,
      'n': False,
  }
  assert default in choices

  if default == 'yes':
    prompt = '[Y/n]'
  elif default == 'no':
    prompt = '[y/N]'
  else:
    prompt = '[y/n]'

  while True:
    sys.stdout.write('%s %s? ' % (question, prompt))
    choice = raw_input().lower()
    if default and not choice:
      return choices[default]
    elif choice in choices:
      return choices[choice]
    else:
      print 'Please answer Yes or No.'


def _prompt_for_squash(commits_in_branch):
  print('WARNING: there are %d commits in branch that are not upstream.' %
        commits_in_branch)
  print '         Squashing into one commit is required to continue.'
  if _prompt_yes_or_no('Automatically squash into one commit'):
    auto_squasher = os.path.join(_SCRIPT_DIR, 'auto_squasher.py')
    return _call_git(
        ['rebase', '-i', 'HEAD~%d' % commits_in_branch],
        env=dict(os.environ,
                 GIT_SEQUENCE_EDITOR='python %s' % auto_squasher)) == 0
  else:
    # DO NOT COMMIT
    return
    sys.exit(1)


def _prepare_branch(current_branch,
                    tracking_branch,
                    build_dir,
                    jobs,
                    rebase=True):
  if not _commit_is_ancestor_of(_BEFORE_RENAME_COMMIT, tracking_branch):
    print 'ERROR: tracking branch not prepared yet; run --prepare on tracking '
    print '       branch first.'
    sys.exit(1)
  if (tracking_branch != 'origin/master' and
      _commit_is_ancestor_of(_RENAME_COMMIT, tracking_branch)):
    print 'ERROR: tracking branch already contains rename commit; bailing out '
    print '       since the tool cannot handle this automatically.'
    sys.exit(1)
  if _commit_is_ancestor_of(_RENAME_COMMIT, current_branch):
    print 'ERROR: current branch appears to already be updated.'
    sys.exit(1)

  commits_in_branch = int(
      _check_call_git_and_get_output([
          'rev-list', '--left-only', '--count', '%s...%s' % (current_branch,
                                                             tracking_branch)
      ]))
  if rebase and commits_in_branch != 1:
    if _prompt_for_squash(commits_in_branch):
      commits_in_branch = 1

  update_args = []
  if rebase:
    update_args.append('rebase')
  else:
    update_args.append('merge')
  if tracking_branch == 'origin/master':
    update_args.append(_BEFORE_RENAME_COMMIT)
  if _call_git(update_args) != 0:
    print 'ERROR: failed to update branch to the commit before the rename.'
    print '       Fix any conflicts and try running with --prepare again.'
    sys.exit(1)

  if _call_gclient(['sync']):
    print 'ERROR: gclient sync returned a non-zero exit code.'
    print '       Please fix the errors and try running with --prepare again.'
    sys.exit(1)

  changed_files = _check_call_git_and_get_output(
      ['diff', '--name-only',
       'HEAD~%d' % commits_in_branch]).strip().split('\n')
  # Filter changed files out to the ones GN knows about, so ninja doesn't fail
  # due to unknown targets.
  changed_files = [
      f for f in changed_files if _call_gn(['refs', build_dir, f]) == 0
  ]

  if not changed_files:
    print 'INFO: This branch does not appear to change files that this script '
    print '      can help automatically rebase. Exiting...'
    sys.exit(0)

  # 'touch' changed files to force a rebuild.
  for f in changed_files:
    os.utime(f, None)

  # -d keeprsp is only needed for Windows, but it doesn't hurt to have it
  # elsewhere.
  ninja_args = ['-C', build_dir, '-d', 'keeprsp']
  if jobs:
    ninja_args.extend(['-j', jobs])
  # Source files are specified relative to the root of the build directory.
  targets = [
      '%s^' % os.path.relpath(
          os.path.join(os.getcwd(), f), os.path.join(os.getcwd(), build_dir))
      for f in changed_files
  ]
  ninja_args.extend(targets)
  if _call_ninja(ninja_args):
    print 'ERROR: Cannot continue, ninja failed!'
    sys.exit(1)

  clang_scripts_dir = os.path.join('tools', 'clang', 'scripts')
  staging_dir = os.path.abspath(
      os.path.join(os.getcwd(), 'tools', 'blink_rename_merge_helper',
                   'staging'))
  blocklist_path = os.path.join(staging_dir, 'data', 'idl_blocklist.txt')
  clang_tool_args = [
      'python', os.path.join(clang_scripts_dir, 'run_tool.py'),
      '--tool-args=--method-blocklist=%s' % blocklist_path, '--generate-compdb',
      'rewrite_to_chrome_style', build_dir
  ]
  clang_tool_args.extend(changed_files)
  clang_tool_output = subprocess.check_output(
      clang_tool_args,
      env=dict(
          os.environ,
          PATH='%s%s%s' % (os.path.join(staging_dir, 'bin'), os.pathsep,
                           os.environ['PATH'])))

  # Extract the edits from the clang tool's output.
  p = subprocess.Popen(
      ['python', os.path.join(clang_scripts_dir, 'extract_edits.py')],
      stdin=subprocess.PIPE,
      stdout=subprocess.PIPE)
  edits, dummy_stderr = p.communicate(input=clang_tool_output)
  if p.returncode != 0:
    print 'ERROR: extracting edits from clang tool output failed.'
    sys.exit(1)

  # Clean the paths to make it easy for apply_edits.py to correctly handle the
  # input. The output path should be a path relative to the build directory,
  # using / as the directory separator. Note that while this logic really
  # belongs in extract_edits.py, putting it there won't help the rebase helper:
  # when rebasing, it will end up using the older version of extract_edits.py in
  # the branch that doesn't have these fixes.
  edit_lines = edits.splitlines()
  for i, line in enumerate(edit_lines):
    edit_type, path, offset, length, replacement = line.split(':::', 4)
    path = os.path.relpath(path, build_dir)
    edit_lines[i] = ':::'.join([edit_type, path, offset, length, replacement])
  edits = '\n'.join(edit_lines)

  # And apply them.
  p = subprocess.Popen(
      ['python', os.path.join(clang_scripts_dir, 'apply_edits.py'), build_dir],
      stdin=subprocess.PIPE)
  p.communicate(input=edits)
  if p.returncode != 0:
    print 'WARNING: failed to apply %d edits from clang tool.' % -p.returncode
    if not _prompt_yes_or_no('Continue (generally safe)', default='yes'):
      sys.exit(1)

  # Use git apply with --include
  apply_manual_patch_args = [
      'apply', os.path.join(staging_dir, 'data', 'manual.patch')
  ]
  for f in changed_files:
    apply_manual_patch_args.append('--include=%s' % f)
  if _call_git(apply_manual_patch_args) != 0:
    print 'ERROR: failed to apply manual patches. Please manually resolve '
    print '       conflicts and re-run the tool with --finish-prepare.'

  _finish_prepare_branch(current_branch)


def _finish_prepare_branch(current_branch):
  _check_call_git(['cl', 'format'])

  # Record changed files in a temporary data store for later use in conflict
  # resolution.
  files_to_save = _check_call_git_and_get_output(
      ['diff', '--name-only']).strip().split()
  if not files_to_save:
    print 'INFO: no changed files. Exiting...'
    sys.exit(0)

  record_dir = tempfile.mkdtemp(prefix='%s-records' % current_branch)
  print 'INFO: saving changed files to %s' % record_dir

  for file_to_save in files_to_save:
    print 'Saving %s' % file_to_save
    shutil.copyfile(file_to_save,
                    os.path.join(record_dir,
                                 hashlib.sha256(file_to_save).hexdigest()))

  _check_call_git(['config', _GIT_CONFIG_BRANCH_RECORDS % current_branch],
                  record_dir)
  print 'INFO: finished preparing branch %s' % current_branch


def _update_branch(current_branch, tracking_branch, rebase=True):
  if not _commit_is_ancestor_of(_BEFORE_RENAME_COMMIT, tracking_branch):
    print 'ERROR: tracking branch not prepared yet; run --prepare on tracking '
    print '       branch first.'
    sys.exit(1)
  if not _commit_is_ancestor_of(_RENAME_COMMIT, tracking_branch):
    print 'ERROR: tracking branch not updated yet; run --update on tracking '
    print '       branch first.'
    sys.exit(1)
  if _commit_is_ancestor_of(_AFTER_RENAME_COMMIT, tracking_branch):
    print 'WARNING: tracking branch is already ahead of the rename commit.'
    print '         The reliability of the tool will be much lower.'
    if not _prompt_yes_or_no('Continue', default='no'):
      sys.exit(1)
  if not _commit_is_ancestor_of(_BEFORE_RENAME_COMMIT, current_branch):
    print 'ERROR: current branch not yet prepared; run --prepare first.'
    sys.exit(1)
  if _commit_is_ancestor_of(_RENAME_COMMIT, current_branch):
    print 'ERROR: current branch appears to already be updated.'
    sys.exit(1)
  prepared_records = None
  try:
    prepared_records = _check_call_git_and_get_output(
        ['config', '--get',
         _GIT_CONFIG_BRANCH_RECORDS % current_branch]).strip()
  except subprocess.CalledProcessError:
    # Likely failed because it's not set. Fall through and fail out
    pass
  if not prepared_records:
    print 'ERROR: current branch is not prepared yet; run --prepare first.'
    sys.exit(1)

  if not os.path.isdir(prepared_records):
    print 'ERROR: records directory %s is invalid.' % prepared_records
    sys.exit(1)

  with _MergeTool(prepared_records):
    args = []
    if rebase:
      args.append('rebase')
    else:
      args.append('merge')
    if tracking_branch == 'origin/master':
      args.append(_RENAME_COMMIT)
    if _call_git(
        args, env=dict(os.environ,
                       BLINK_RENAME_RECORDS_PATH=prepared_records)) != 0:
      print 'ERROR: failed to update'

  print 'INFO: updated branch %s' % current_branch


def run():
  # run.py made a poor life choice. Workaround that here by (hopefully) changing
  # the working directory back to the git repo root.
  os.chdir(os.path.join('..', '..'))

  parser = argparse.ArgumentParser()
  parser.add_argument(
      '-C', metavar='DIR', required=True, help='Path to build directory.')
  parser.add_argument(
      '-j', metavar='N', help='Number of ninja jobs to run in parallel.')
  update_policy = parser.add_mutually_exclusive_group(required=True)
  update_policy.add_argument(
      '--rebase', action='store_true', help='Update the branch using rebase.')
  update_policy.add_argument(
      '--merge', action='store_true', help='Update the branch using merge.')
  tool_mode = parser.add_mutually_exclusive_group(required=True)
  tool_mode.add_argument(
      '--prepare',
      action='store_true',
      help='Prepare the branch for updating across the rename commit.')
  tool_mode.add_argument(
      '--finish-prepare',
      action='store_true',
      help='Finish preparing the branch for updating across the rename commit.')
  tool_mode.add_argument(
      '--update',
      action='store_true',
      help='Update the branch across the rename commit.')
  args = parser.parse_args()

  _ensure_clean_tree()

  current_branch, tracking_branch = _get_branch_info()

  if tracking_branch != 'origin/master':
    print 'WARNING: The script is more fragile when the tracking branch '
    print '         is not origin/master.'
    # Default to danger mode.
    if not _prompt_yes_or_no('Continue', default='yes'):
      sys.exit(1)

  _ensure_origin_contains_commit()

  if args.prepare:
    _prepare_branch(current_branch, tracking_branch, args.C, args.j,
                    args.rebase)
  elif args.finish_prepare:
    _finish_prepare_branch(current_branch)
  else:
    _update_branch(current_branch, tracking_branch, args.rebase)
