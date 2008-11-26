#!/usr/bin/python
#!/usr/bin/python
# Copyright (c) 2006-2008 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
#
# Tool to quickly revert a change.

import exceptions
import optparse
import os
import sys
import xml

import gcl
import gclient

class ModifiedFile(exceptions.Exception):
  pass
class NoModifiedFile(exceptions.Exception):
  pass
class NoBlameList(exceptions.Exception):
  pass
class OutsideOfCheckout(exceptions.Exception):
  pass


def getTexts(nodelist):
  """Return a list of texts in the children of a list of DOM nodes."""
  rc = []
  for node in nodelist:
    if node.nodeType == node.TEXT_NODE:
      rc.append(node.data)
    else:
      rc.extend(getTexts(node.childNodes))
  return rc


def RunShellXML(command, print_output=False, keys=None):
  output = gcl.RunShell(command, print_output)
  try:
    dom = xml.dom.minidom.parseString(output)
    if not keys:
      return dom
    result = {}
    for key in keys:
      result[key] = getTexts(dom.getElementsByTagName(key))
  except xml.parsers.expat.ExpatError:
    print "Failed to parse output:\n%s" % output
    raise
  return result


def UniqueFast(list):
  list = [item for item in set(list)]
  list.sort()
  return list


def GetRepoBase():
  """Returns the repository base of the root local checkout."""
  xml_data = RunShellXML(['svn', 'info', '.', '--xml'], keys=['root', 'url'])
  root = xml_data['root'][0]
  url = xml_data['url'][0]
  if not root or not url:
    raise exceptions.Exception("I'm confused by your checkout")
  if not url.startswith(root):
    raise exceptions.Exception("I'm confused by your checkout", url, root)
  return url[len(root):] + '/'


def Revert(revisions, force=False, commit=True, send_email=True, message=None,
           reviewers=None):
  """Reverts many revisions in one change list.

  If force is True, it will override local modifications.
  If commit is True, a commit is done after the revert.
  If send_mail is True, a review email is sent.
  If message is True, it is used as the change description.
  reviewers overrides the blames email addresses for review email."""

  # Use the oldest revision as the primary revision.
  changename = "revert%d" % revisions[len(revisions)-1]
  if not force and os.path.exists(gcl.GetChangelistInfoFile(changename)):
    print "Error, change %s already exist." % changename
    return 1

  # Move to the repository root and make the revision numbers sorted in
  # decreasing order.
  os.chdir(gcl.GetRepositoryRoot())
  revisions.sort(reverse=True)
  revisions_string = ",".join([str(rev) for rev in revisions])
  revisions_string_rev = ",".join([str(-rev) for rev in revisions])

  repo_base = GetRepoBase()
  files = []
  blames = []
  # Get all the modified files by the revision. We'll use this list to optimize
  # the svn merge.
  for revision in revisions:
    log = RunShellXML(["svn", "log", "-c", str(revision), "-v", "--xml"],
                        keys=['path', 'author'])
    for file in log['path']:
      # Remove the /trunk/src/ part. The + 1 is for the last slash.
      if not file.startswith(repo_base):
        raise OutsideOfCheckout(file)
      files.append(file[len(repo_base):])
    blames.extend(log['author'])

  # On Windows, we need to fix the slashes once they got the url part removed.
  if sys.platform == 'win32':
    # On Windows, gcl expect the correct slashes.
    files = [file.replace('/', os.sep) for file in files]

  # Keep unique.
  files = UniqueFast(files)
  blames = UniqueFast(blames)
  if not reviewers:
    reviewers = blames
  else:
    reviewers = UniqueFast(reviewers)

  # Make sure there's something to revert.
  if not files:
    raise NoModifiedFile
  if not reviewers:
    raise NoBlameList

  if blames:
    print "Blaming %s\n" % ",".join(blames)
  if reviewers != blames:
    print "Emailing %s\n" % ",".join(reviewers)
  print "These files were modified in %s:" % revisions_string
  print "\n".join(files)
  print ""

  # Make sure these files are unmodified with svn status.
  status = gcl.RunShell(["svn", "status"] + files)
  if status:
    if force:
      gcl.RunShell(["svn", "revert"] + files)
    else:
      raise ModifiedFile(status)
  # svn up on each of these files
  gcl.RunShell(["svn", "up"] + files)

  files_status = {}
  # Extract the first level subpaths. Subversion seems to degrade
  # exponentially w.r.t. repository size during merges. Working at the root
  # directory is too rough for svn due to the repository size.
  roots = UniqueFast([file.split(os.sep)[0] for file in files])
  for root in roots:
    # Is it a subdirectory or a files?
    is_root_subdir = os.path.isdir(root)
    need_to_update = False
    if is_root_subdir:
      os.chdir(root)
      file_list = []
      # List the file directly since it is faster when there is only one file.
      for file in files:
        if file.startswith(root):
          file_list.append(file)
      if len(file_list) > 1:
        # Listing multiple files is not supported by svn merge.
        file_list = ['.']
        need_to_update = True
    else:
      # Oops, root was in fact a file in the root directory.
      file_list = [root]
      root = "."

    print "Reverting %s in %s/" % (revisions_string, root)
    if need_to_update:
      # Make sure '.' revision is high enough otherwise merge will be
      # unhappy.
      retcode = gcl.RunShellWithReturnCode(['svn', 'up', '.', '-N'])[1]
      if retcode:
        print 'svn up . -N failed in %s/.' % root
        return retcode

    command = ["svn", "merge", "-c", revisions_string_rev]
    command.extend(file_list)
    (output, retcode) = gcl.RunShellWithReturnCode(command, print_output=True)
    if retcode:
      print "'%s' failed:" % command
      return retcode

    # Grab the status
    lines = output.split('\n')
    for line in lines[1:]:
      if line.startswith('Skipped'):
        print ""
        raise ModifiedFile(line[9:-1])
      # Update the status.
      status = line[:5] + '  '
      file = line[5:]
      if is_root_subdir:
        files_status[root + os.sep + file] = status
      else:
        files_status[file] = status

    if is_root_subdir:
      os.chdir('..')

  # Transform files_status from a dictionary to a list of tuple.
  files_status = [(files_status[file], file) for file in files]

  description = "Reverting %s." % revisions_string
  if message:
    description += "\n\n"
    description += message
  # Don't use gcl.Change() since it prompts the user for infos.
  change_info = gcl.ChangeInfo(name=changename, issue='',
                               description=description, files=files_status)
  change_info.Save()

  upload_args = ['-r', ",".join(reviewers)]
  if send_email:
    upload_args.append('--send_mail')
  if commit:
    upload_args.append('--no_try')
  gcl.UploadCL(change_info, upload_args)

  retcode = 0
  if commit:
    gcl.Commit(change_info, ['--force'])
    # TODO(maruel):  gclient sync (to leave the local checkout in an usable
    # state)
    retcode = gclient.Main(["gclient.py", "sync"])
  return retcode


def Main(argv):
  usage = (
"""%prog [options] [revision numbers to revert]
Revert a set of revisions, send the review to Rietveld, sends a review email
and optionally commit the revert.""")

  parser = optparse.OptionParser(usage=usage)
  parser.add_option("-c", "--commit", default=False, action="store_true",
                    help="Commits right away.")
  parser.add_option("-f", "--force", default=False, action="store_true",
                    help="Forces the local modification even if a file is "
                         "already modified locally.")
  parser.add_option("-n", "--no_email", default=False, action="store_true",
                    help="Inhibits from sending a review email.")
  parser.add_option("-m", "--message", default=None,
                    help="Additional change description message.")
  parser.add_option("-r", "--reviewers", default=None,
                    help="Reviewers to send the email to. By default, the list "
                         "of commiters is used.")
  options, args = parser.parse_args(argv)
  revisions = []
  try:
    for item in args[1:]:
      revisions.append(int(item))
  except ValueError:
    parser.error("You need to pass revision numbers.")
  if not revisions:
    parser.error("You need to pass revision numbers.")
  retcode = 1
  try:
    retcode = Revert(revisions, options.force, options.commit,
                     not options.no_email, options.message, options.reviewers)
  except NoBlameList:
    print "Error: no one to blame."
  except NoModifiedFile:
    print "Error: no files to revert."
  except ModifiedFile, e:
    print "You need to revert these files since they were already modified:"
    print "".join(e.args)
    print "You can use the --force flag to revert the files."
  except OutsideOfCheckout, e:
    print "Your repository doesn't contain ", str(e)

  return retcode


if __name__ == "__main__":
  sys.exit(Main(sys.argv))
 