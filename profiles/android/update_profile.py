#!/usr/bin/env vpython
# Copyright (c) 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""This script is used to update our local AFDO profiles.

This uses profiles of Chrome provided by our friends from Chrome OS. They use gs
a bit differently than we do."""

import argparse
import contextlib
import distutils.spawn
import os
import subprocess
import sys

PROFILE_DIRECTORY = os.path.dirname(__file__)
LOCAL_PROFILE_PATH = os.path.join(PROFILE_DIRECTORY, "afdo.prof")

# We use these to track the local profile; newest.txt is owned by git and tracks
# the name of the newest profile we should pull, and local.txt is the most
# recent profile we've successfully pulled.
NEWEST_PROFILE_NAME_PATH = os.path.join(PROFILE_DIRECTORY, "newest.txt")
LOCAL_PROFILE_NAME_PATH = os.path.join(PROFILE_DIRECTORY, "local.txt")


def GetUpToDateProfileName():
    with open(NEWEST_PROFILE_NAME_PATH) as f:
        return f.read().strip()


def LocalProfileIs(desired_profile):
    try:
        with open(LOCAL_PROFILE_NAME_PATH) as f:
            local_profile = f.read().strip()
    except IOError as e:
        # Assume it either didn't exist, or we couldn't read it. In either case,
        # we should probably grab a new profile (and, in doing so, make this
        # file sane again)
        return False
    return local_profile == desired_profile


def NoteLocalProfileIs(name):
    with open(LOCAL_PROFILE_NAME_PATH, "w") as f:
        f.write(name)


def GetGsutilExecutablePath():
    dl_path = distutils.spawn.find_executable("download_from_google_storage.py")
    if dl_path is not None:
        return os.path.join(os.path.dirname(dl_path), "gsutil.py")
    return None


def CheckCallOrExit(cmd):
    proc = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    stdout, stderr = proc.communicate()
    exit_code = proc.wait()
    if not exit_code:
        return

    complaint_lines = [
            "## %s failed with exit code %d" % cmd[0],
            "## Full command: %s" % cmd,
            "## Stdout:\n" + stdout,
            "## Stderr:\n" + stderr,
    ]
    print >>sys.stderr, "\n".join(complaint_lines)
    sys.exit(1)


def RetrieveProfile(gsutil, gs_path, out_path):
    # The local names of these never change, so if we fail due to a partial
    # write or something, we'll just wipe it all out and try again on our next
    # run.
    compressed_path = out_path + ".bz2"
    CheckCallOrExit([gsutil, "cp", gs_path, compressed_path])
    # NOTE: we can't use Python's bzip module, since it doesn't support
    # multi-stream bzip files. It will silently succeed and give us a garbage
    # profile.
    # bzip2 removes the compressed file on success.
    CheckCallOrExit(["bzip2", "-d", compressed_path])


def main():
    parser = argparse.ArgumentParser("Downloads new Chrome OS profiles")
    parser.add_argument("-f", "--force", action="store_true",
                        help="Fetch a profile even if the local one is current")
    args = parser.parse_args()

    up_to_date_profile = GetUpToDateProfileName()

    # In a perfect world, the local profile should always exist if
    # `LocalProfileIs(up_to_date_profile)`. If it's gone, though, the user
    # probably removed it as a way to get us to download it again.
    if not args.force and LocalProfileIs(up_to_date_profile) \
            and os.path.exists(LOCAL_PROFILE_PATH):
        return 0

    # Can't shell out to download_from_google_storage, since these aren't stored
    # in a format it supports.
    gsutil = GetGsutilExecutablePath()
    if gsutil is None:
        print >>sys.stderr, "error: download_from_google_storage not found"
        return 1

    gs_path = "gs://chromeos-prebuilt/afdo-job/llvm/%s" % up_to_date_profile
    new_tmpfile = LOCAL_PROFILE_PATH + ".new"
    RetrieveProfile(gsutil, gs_path, new_tmpfile)
    os.rename(new_tmpfile, LOCAL_PROFILE_PATH)
    NoteLocalProfileIs(up_to_date_profile)


if __name__ == "__main__":
  sys.exit(main())
