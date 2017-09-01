# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import distutils.dir_util
import filecmp
import os
import shutil
import subprocess
import sys
import tempfile


def main(arch, outdir, tlb, h, dlldata, iid, proxy, idl, *flags):
  # Copy checked-in outputs to final location.
  THIS_DIR = os.path.abspath(os.path.dirname(__file__))
  source = os.path.join(THIS_DIR, "..", "..", "..",
      "third_party", "win_build_output", outdir.replace('gen/', 'midl/'))
  distutils.dir_util.copy_tree(source, outdir)

  # On non-Windows, that's all we can do.
  if sys.platform != 'win32':
    return

  # On Windows, run midl.exe on the input and check that its outputs are
  # identical to the checked-in outputs.
  tmp_dir = tempfile.mkdtemp()

  # Read the environment block from the file. This is stored in the format used
  # by CreateProcess. Drop last 2 NULs, one for list terminator, one for
  # trailing vs. separator.
  env_pairs = open(arch).read()[:-2].split('\0')
  env_dict = dict([item.split('=', 1) for item in env_pairs])

  args = ['midl', '/nologo'] + list(flags) + [
      '/out', tmp_dir,
      '/tlb', tlb,
      '/h', h,
      '/dlldata', dlldata,
      '/iid', iid,
      '/proxy', proxy,
      idl]
  try:
    popen = subprocess.Popen(args, shell=True, env=env_dict,
                             stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    out, _ = popen.communicate()
    # Filter junk out of stdout, and write filtered versions. Output we want
    # to filter is pairs of lines that look like this:
    # Processing C:\Program Files (x86)\Microsoft SDKs\...\include\objidl.idl
    # objidl.idl
    lines = out.splitlines()
    prefixes = ('Processing ', '64 bit Processing ')
    processing = set(os.path.basename(x)
                     for x in lines if x.startswith(prefixes))
    for line in lines:
      if not line.startswith(prefixes) and line not in processing:
        print line
    if popen.returncode != 0:
      return popen.returncode

    # now compare the output in tmp_dir to the checked-in outputs.
    diff = filecmp.dircmp(tmp_dir, source)
    if diff.diff_files or set(diff.left_list) != set(diff.right_list):
      print >>sys.stderr, 'midl.exe output different from files in %s, see %s' \
          % ( source, tmp_dir)
      sys.exit(1)  # Intentionally skips finally: that deletes the temp dir.
    return 0
  finally:
    if os.path.exists(tmp_dir):
      shutil.rmtree(tmp_dir)


if __name__ == '__main__':
  sys.exit(main(*sys.argv[1:]))
