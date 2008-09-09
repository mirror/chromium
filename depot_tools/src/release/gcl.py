#!/usr/bin/python
# Wrapper script around Rietveld's upload.py that groups files into
# changelists.

import getpass
import linecache
import os
import random
import re
import string
import subprocess
import sys
import tempfile
import upload
import urllib2

CODEREVIEW_SETTINGS = {
  # Default values.
  "CODE_REVIEW_SERVER": "codereview.chromium.org",
  "CC_LIST": "chromium-reviews@googlegroups.com",
  "VIEW_VC": "http://src.chromium.org/viewvc/chrome?view=rev&revision=",
}

# Use a shell for subcommands on Windows to get a PATH search, and because svn
# may be a batch file.
use_shell = sys.platform.startswith("win")


# globals that store the root of the current repositary and the directory where
# we store information about changelists.
repository_root = ""
gcl_info_dir = ""


def GetSVNFileInfo(file, field):
  """Returns a field from the svn info output for the given file."""
  output = RunShell(["svn", "info", file])
  for line in output.splitlines():
    search = field + ": "
    if line.startswith(search):
      return line[len(search):]
  return ""


def GetRepositoryRoot():
  """Returns the top level directory of the current repository."""
  global repository_root
  if not repository_root:
    cur_dir_repo_root = GetSVNFileInfo(os.getcwd(), "Repository Root")
    if not cur_dir_repo_root:
      ErrorExit("gcl run outside of repository")

    repository_root = os.getcwd()
    while True:
      parent = os.path.dirname(repository_root)
      if GetSVNFileInfo(parent, "Repository Root") != cur_dir_repo_root:
        break
      repository_root = parent
  # Now read the code review settings for this repository.
  settings_file = os.path.join(repository_root, "codereview.settings")
  if os.path.exists(settings_file):
    output = ReadFile(settings_file)
    for line in output.splitlines():
      if not line or line.startswith("#"):
        continue
      key, value = line.split(": ", 1)
      CODEREVIEW_SETTINGS[key] = value
  return repository_root


def GetCodeReviewSetting(key):
  """Returns a value for the given key for this repository."""
  return CODEREVIEW_SETTINGS.get(key, "")


def GetInfoDir():
  """Returns the directory where gcl info files are stored."""
  global gcl_info_dir
  if not gcl_info_dir:
    gcl_info_dir = os.path.join(GetRepositoryRoot(), '.svn', 'gcl_info')
  return gcl_info_dir


def ErrorExit(msg):
  """Print an error message to stderr and exit."""
  print >>sys.stderr, msg
  sys.exit(1)


def RunShell(command, print_output=False):
  """Executes a command and returns the output."""
  p = subprocess.Popen(command, stdout = subprocess.PIPE,
                       stderr = subprocess.STDOUT, shell = use_shell,
                       universal_newlines=True)
  if print_output:
    output_array = []
    while True:
      line = p.stdout.readline()
      if not line:
        break
      if print_output:
        print line.strip('\n')
      output_array.append(line)
    output = "".join(output_array)
  else:
    output = p.stdout.read()
  p.wait()
  p.stdout.close()
  return output


def ReadFile(filename):
  """Returns the contents of a file."""
  file = open(filename, 'r')
  result = file.read()
  file.close()
  return result


def WriteFile(filename, contents):
  """Overwrites the file with the given contents."""
  file = open(filename, 'w')
  file.write(contents)
  file.close()


class ChangeInfo:
  """Holds information about a changelist.
  
    issue: the Rietveld issue number, of "" if it hasn't been uploaded yet.
    description: the description.
    files: a list of 2 tuple containing (status, filename) of changed files,
           with paths being relative to the top repository directory.
  """
  def __init__(self, name="", issue="", description="", files=[]):
    self.name = name
    self.issue = issue
    self.description = description
    self.files = files

  def FileList(self):
    """Returns a list of files."""
    return [file[1] for file in self.files]

  def Save(self):
    """Writes the changelist information to disk."""
    data = SEPARATOR.join([self.issue,
                          "\n".join([f[0] + f[1] for f in self.files]),
                          self.description])
    WriteFile(GetChangelistInfoFile(self.name), data)

  def Delete(self):
    """Removes the changelist information from disk."""
    os.remove(GetChangelistInfoFile(self.name))

  def CloseIssue(self):
    """Closes the Rietveld issue for this changelist."""
    data = [("description", self.description),]
    ctype, body = upload.EncodeMultipartFormData(data, [])
    SendToRietveld("/" + self.issue + "/close", body, ctype)

  def UpdateRietveldDescription(self):
    """Sets the description for an issue on Rietveld."""
    data = [("description", self.description),]
    ctype, body = upload.EncodeMultipartFormData(data, [])
    SendToRietveld("/" + self.issue + "/description", body, ctype)

  
SEPARATOR = "\n-----\n"
# The info files have the following format:
# issue_id\n
# SEPARATOR\n
# filepath1\n
# filepath2\n
# .
# .
# filepathn\n
# SEPARATOR\n
# description


def GetChangelistInfoFile(changename):
  """Returns the file that stores information about a changelist."""
  if not changename or re.search(r'\W', changename):
    ErrorExit("Invalid changelist name: " + changename)
  return os.path.join(GetInfoDir(), changename)


def LoadChangelistInfo(changename, fail_on_not_found=True,
                       update_status=False):
  """Gets information about a changelist.
  
  Args:
    fail_on_not_found: if True, this function will quit the program if the
      changelist doesn't exist.
    update_status: if True, the svn status will be updated for all the files
      and unchanged files will be removed.
  
  Returns: a ChangeInfo object.
  """
  info_file = GetChangelistInfoFile(changename)
  if not os.path.exists(info_file):
    if fail_on_not_found:
      ErrorExit("Changelist " + changename + " not found.")
    return ChangeInfo(changename)
  data = ReadFile(info_file)
  split_data = data.split(SEPARATOR, 2)
  if len(split_data) != 3:
    os.remove(info_file)
    ErrorExit("Changelist file %s was corrupt and deleted" % info_file)
  issue = split_data[0]
  files = []
  for line in split_data[1].splitlines():
    status = line[:7]
    file = line[7:]
    files.append((status, file))
  description = split_data[2]  
  save = False
  if update_status:
    for file in files:
      filename = os.path.join(GetRepositoryRoot(), file[1])
      status = RunShell(["svn", "status", filename])[:7]
      if not status:  # File has been reverted.
        save = True
        files.remove(file)
      elif status != file[0]:
        save = True
        files[files.index(file)] = (status, file[1])
  change_info = ChangeInfo(changename, issue, description, files)
  if save:
    change_info.Save()
  return change_info


def GetCLs():
  """Returns a list of all the changelists in this repository."""
  return os.listdir(GetInfoDir())


def GenerateChangeName():
  """Generate a random changelist name."""
  random.seed()
  current_cl_names = GetCLs()
  while True:
    cl_name = (random.choice(string.ascii_lowercase) +
               random.choice(string.digits) +
               random.choice(string.ascii_lowercase) +
               random.choice(string.digits))
    if cl_name not in current_cl_names:
      return cl_name


def GetModifiedFiles():
  """Returns a set that maps from changelist name to (status,filename) tuples.

  Files not in a changelist have an empty changelist name.  Filenames are in
  relation to the top level directory of the current repositary.  Note that
  only the current directory and subdirectories are scanned, in order to
  improve performance while still being flexible.
  """
  files = {}
  
  # Since the files are normalized to the root folder of the repositary, figure
  # out what we need to add to the paths.
  dir_prefix = os.getcwd()[len(GetRepositoryRoot()):].strip(os.sep)

  # Get a list of all files in changelists.
  files_in_cl = {}
  for cl in GetCLs():
    change_info = LoadChangelistInfo(cl)
    for status, filename in change_info.files:
      files_in_cl[filename] = change_info.name

  # Get all the modified files.
  status = RunShell(["svn", "status"])
  for line in status.splitlines():
    if not len(line) or line[0] == "?":
      continue
    status = line[:7]
    filename = line[7:]
    if dir_prefix:
      filename = os.path.join(dir_prefix, filename)
    change_list_name = ""
    if filename in files_in_cl:
      change_list_name = files_in_cl[filename]
    files.setdefault(change_list_name, []).append((status, filename))

  return files


def GetFilesNotInCL():
  """Returns a list of tuples (status,filename) that aren't in any changelists.
  
  See docstring of GetModifiedFiles for information about path of files and
  which directories are scanned.
  """
  modified_files = GetModifiedFiles()
  if "" not in modified_files:
    return []
  return modified_files[""]


def SendToRietveld(request_path, payload=None,
                   content_type="application/octet-stream"):
  """Send a POST/GET to Rietveld.  Returns the response body."""
  def GetUserCredentials():
    """Prompts the user for a username and password."""
    email = raw_input("Email: ").strip()
    password = getpass.getpass("Password for %s: " % email)
    return email, password

  server = GetCodeReviewSetting("CODE_REVIEW_SERVER")
  rpc_server = upload.HttpRpcServer(server,
                                    GetUserCredentials,
                                    host_override=server,
                                    save_cookies=True)
  return rpc_server.Send(request_path, payload, content_type)


def GetIssueDescription(issue):
  """Returns the issue description from Rietveld."""
  return SendToRietveld("/" + issue + "/description")


def UnknownFiles(extra_args):
  """Runs svn status and prints unknown files.

  Any args in |extra_args| are passed to the tool to support giving alternate
  code locations.
  """
  args = ["svn", "status"]
  args += extra_args
  p = subprocess.Popen(args, stdout = subprocess.PIPE,
                       stderr = subprocess.STDOUT, shell = use_shell)
  while 1:
    line = p.stdout.readline()
    if not line:
      break
    if line[0] != '?':
      continue  # Not an unknown file to svn.
    # The lines look like this:
    # "?      foo.txt"
    # and we want just "foo.txt"
    print line[7:].strip()
  p.wait()
  p.stdout.close()


def Opened():
  """Prints a list of modified files in the current directory down."""
  files = GetModifiedFiles()
  cl_keys = files.keys()
  cl_keys.sort()
  for cl_name in cl_keys:
    if cl_name:
      note = ""
      if len(LoadChangelistInfo(cl_name).files) != len(files[cl_name]):
        note = " (Note: this changelist contains files outside this directory)"
      print "\n--- Changelist " + cl_name + note + ":"
    for file in files[cl_name]:
      print "".join(file)


def Help():
  print ("GCL is a wrapper for Subversion that simplifies working with groups "
         "of files.\n")
  print "Basic commands:"
  print "-----------------------------------------"
  print "   gcl change change_name"
  print ("      Add/remove files to a changelist.  Only scans the current "
         "directory and subdirectories.\n")
  print ("   gcl upload change_name [-r reviewer1@gmail.com,"
         "reviewer2@gmail.com,...] [--send_mail]")
  print "      Uploads the changelist to the server for review.\n"
  print "   gcl commit change_name"
  print "      Commits the changelist to the repository.\n"
  print "Advanced commands:"
  print "-----------------------------------------"
  print "   gcl delete change_name"
  print "      Deletes a changelist.\n"
  print "   gcl diff change_name"
  print "      Diffs all files in the changelist.\n"
  print "   gcl diff"
  print ("      Diffs all files in the current directory and subdirectories "
         "that aren't in a changelist.\n")
  print "   gcl changes"
  print "      Lists all the the changelists and the files in them.\n"
  print "   gcl nothave [optional directory]"
  print "      Lists files unknown to Subversion.\n"
  print "   gcl opened"
  print ("      Lists modified files in the current directory and "
         "subdirectories.\n")
  print "   gcl try change_name"
  print ("      Sends the change to the tryserver so a trybot can do a test"
         " run on your code.\n")


def GetEditor():
  editor = os.environ.get("SVN_EDITOR")
  if not editor:
    editor = os.environ.get("EDITOR")

  if not editor:
    if sys.platform.startswith("win"):
      editor = "notepad"
    else:
      editor = "vi"

  return editor


def GenerateDiff(files):
  """Returns a string containing the diff for the given file list."""
  diff = []
  for file in files:
    # Use svn info output instead of os.path.isdir because the latter fails
    # when the file is deleted.
    if GetSVNFileInfo(file, "Node Kind") == "directory":
      continue
    # If the user specified a custom diff command in their svn config file,
    # then it'll be used when we do svn diff, which we don't want to happen
    # since we want the unified diff.  Using --diff-cmd=diff doesn't always
    # work, since they can have another diff executable in their path that
    # gives different line endings.  So we use a bogus temp directory as the
    # config directory, which gets around these problems.
    if sys.platform.startswith("win"):
      parent_dir = tempfile.gettempdir()
    else:
      parent_dir = sys.path[0]  # tempdir is not secure.
    bogus_dir = os.path.join(parent_dir, "temp_svn_config")
    if not os.path.exists(bogus_dir):
      os.mkdir(bogus_dir)
    diff.append(RunShell(["svn", "diff", "--config-dir", bogus_dir, file]))
  return "".join(diff)


def UploadCL(change_info, args):
  if not change_info.FileList():
    print "Nothing to upload, changelist is empty."
    return

  upload_arg = ["upload.py", "-y", "-l"]
  upload_arg.append("--server=" + GetCodeReviewSetting("CODE_REVIEW_SERVER"))
  upload_arg.extend(args)

  desc_file = ""
  if change_info.issue:  # Uploading a new patchset.
    upload_arg.append("--message=''")
    upload_arg.append("--issue=" + change_info.issue)
  else: # First time we upload.
    handle, desc_file = tempfile.mkstemp(text=True)
    os.write(handle, change_info.description)
    os.close(handle)

    upload_arg.append("--cc=" + GetCodeReviewSetting("CC_LIST"))
    upload_arg.append("--description_file=" + desc_file + "")
    if change_info.description:
      subject = change_info.description[:77]
      if subject.find("\r\n") != -1:
        subject = subject[:subject.find("\r\n")]
      if subject.find("\n") != -1:
        subject = subject[:subject.find("\n")]
      if len(change_info.description) > 77:
        subject = subject + "..."
      upload_arg.append("--message=" + subject)
  
  # Change the current working directory before calling upload.py so that it
  # shows the correct base.
  os.chdir(GetRepositoryRoot())

  # If we have a lot of files with long paths, then we won't be able to fit
  # the command to "svn diff".  Instead, we generate the diff manually for
  # each file and concatenate them before passing it to upload.py.
  issue = upload.RealMain(upload_arg, GenerateDiff(change_info.FileList()))
  if issue and issue != change_info.issue:
    change_info.issue = issue
    change_info.Save()

  if desc_file:
    os.remove(desc_file)


def TryChange(change_info, args):
  """Create a diff file of change_info and send it to the try server."""
  try:
    import trychange
  except ImportError:
    ErrorExit("You need to install trychange.py to use the try server.")

  trychange.TryChange(args, change_info.name, change_info.FileList())


def Commit(change_info):
  if not change_info.FileList():
    print "Nothing to commit, changelist is empty."
    return

  commit_cmd = ["svn", "commit"]
  filename = ''
  if change_info.issue:
    # Get the latest description from Rietveld.
    change_info.description = GetIssueDescription(change_info.issue)

  commit_message = change_info.description.replace('\r\n', '\n')
  if change_info.issue:
    commit_message += ('\nReview URL: http://%s/%s' %
                       (GetCodeReviewSetting("CODE_REVIEW_SERVER"),
                        change_info.issue))

  handle, commit_filename = tempfile.mkstemp(text=True)
  os.write(handle, commit_message)
  os.close(handle)

  handle, targets_filename = tempfile.mkstemp(text=True)
  os.write(handle, "\n".join(change_info.FileList()))
  os.close(handle)

  commit_cmd += ['--file=' + commit_filename]
  commit_cmd += ['--targets=' + targets_filename]
  # Change the current working directory before calling commit.
  os.chdir(GetRepositoryRoot())
  output = RunShell(commit_cmd, True)
  os.remove(commit_filename)
  os.remove(targets_filename)
  if output.find("Committed revision") != -1:
    change_info.Delete()

    if change_info.issue:
      revision = re.compile(".*?\nCommitted revision (\d+)",
                            re.DOTALL).match(output).group(1)
      viewvc_url = GetCodeReviewSetting("VIEW_VC")
      change_info.description = (change_info.description +
                                 "\n\nCommitted: " + viewvc_url + revision)
      change_info.CloseIssue()


def Change(change_info):
  """Creates/edits a changelist."""
  if change_info.issue:
    try:
      description = GetIssueDescription(change_info.issue)
    except urllib2.HTTPError, err:
      if err.code == 404:
        # The user deleted the issue in Rietveld, so forget the old issue id.
        description = change_info.description
        change_info.issue = ""
        change_info.Save()
      else:
        ErrorExit("Error getting the description from Rietveld: " + err)
  else:
    description = change_info.description

  other_files = GetFilesNotInCL()

  separator1 = ("\n---All lines above this line become the description.\n"
                "---Repository Root: " + GetRepositoryRoot() + "\n"
                "---Paths in this changelist (" + change_info.name + "):\n")
  separator2 = "\n\n---Paths modified but not in any changelist:\n\n"
  text = (description + separator1 + '\n' +
          '\n'.join([f[0] + f[1] for f in change_info.files]) + separator2 +
          '\n'.join([f[0] + f[1] for f in other_files]) + '\n')

  handle, filename = tempfile.mkstemp(text=True)
  os.write(handle, text)
  os.close(handle)
  
  command = GetEditor() + " " + filename
  os.system(command)

  result = ReadFile(filename)
  os.remove(filename)

  if not result:
    return

  split_result = result.split(separator1, 1)
  if len(split_result) != 2:
    ErrorExit("Don't modify the text starting with ---!\n\n" + result)

  new_description = split_result[0]
  cl_files_text = split_result[1]
  if new_description != description:
    change_info.description = new_description
    if change_info.issue:
      # Update the Rietveld issue with the new description.
      change_info.UpdateRietveldDescription()

  new_cl_files = []
  for line in cl_files_text.splitlines():
    if not len(line):
      continue
    if line.startswith("---"):
      break
    status = line[:7]
    file = line[7:]
    new_cl_files.append((status, file))
  change_info.files = new_cl_files

  change_info.Save()
  print change_info.name + " changelist saved."


def Changes():
  """Print all the changlists and their files."""
  for cl in GetCLs():
    change_info = LoadChangelistInfo(cl, True, True)
    print "\n--- Changelist " + change_info.name + ":"
    for file in change_info.files:
      print "".join(file)


def main(argv=None):
  if argv is None:
    argv = sys.argv
  
  if len(argv) == 1:
    Help()
    return 0;

  # Create the directory where we store information about changelists if it
  # doesn't exist.
  if not os.path.exists(GetInfoDir()):
    os.mkdir(GetInfoDir())

  command = argv[1]
  if command == "opened":
    Opened()
    return 0
  if command == "nothave":
    UnknownFiles(argv[2:])
    return 0
  if command == "changes":
    Changes()
    return 0
  if command == "diff" and len(argv) == 2:
    files = GetFilesNotInCL()
    print GenerateDiff([os.path.join(GetRepositoryRoot(), x[1]) for x in files])
    return 0

  if len(argv) == 2:
    if command == "change":
      # Generate a random changelist name.
      changename = GenerateChangeName()
    elif command == "help":
      Help()
      return 0
    else:
      ErrorExit("Need a changelist name.")
  else:
    changename = argv[2]

  fail_on_not_found = command != "change"
  change_info = LoadChangelistInfo(changename, fail_on_not_found, True)

  if command == "change":
    Change(change_info)
  elif command == "upload":
    UploadCL(change_info, argv[3:])
  elif command == "commit":
    Commit(change_info)
  elif command == "delete":
    change_info.Delete()
  elif command == "try":
    TryChange(change_info, argv[3:])
  else:
    # Everything else that is passed into gcl we redirect to svn, after adding
    # the files. This allows commands such as 'gcl diff xxx' to work.
    args =["svn", command]
    root = GetRepositoryRoot()
    args.extend([os.path.join(root, x) for x in change_info.FileList()])
    RunShell(args, True)
  return 0


if __name__ == "__main__":
    sys.exit(main())
