#!/usr/bin/env python
# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import os
import subprocess
import sys

print os.getcwd()
root_build_dir = os.getcwd()
print "root_build_dir: %s" % root_build_dir

# go to third_party/ffmpeg
os.chdir(sys.argv[1])

# TODO(liberato): join?
src_dir = os.path.join(root_build_dir, "..", "..")
platform = sys.argv[2]
arch = sys.argv[3]
gen_dir = sys.argv[4]
out_dir = sys.argv[5]
mac_developer_dir = sys.argv[6]

print "src_dir: %s" % src_dir
print "gen_dir: %s" % sys.argv[4]
print "out_dir: %s" % out_dir
print "mac_developer_dir: %s" % mac_developer_dir

# TODO(liberato): how do we get this information from gn?
opus_path= os.path.join(root_build_dir, out_dir, "..", "third_party", "opus")
yasm_path= os.path.join(root_build_dir, "yasm")
print "opus_path: %s" % opus_path
print "yasm_path: %s" % yasm_path

print "========= build_ffmpeg"
# NOTE: the CL should include a change to DEPS that checks out the right version
# of ffmpeg.

# for mac, we should point configure at chromium's libopus
ldflags="--extra-ldflags=-L%s" % opus_path
print "ldflags=%s" % ldflags

yasmflags="--yasmexe=%s" % yasm_path
print "yasmflags=%s" % yasmflags

# If we have a developer directory, set it.  this lets us use a hermetic xcode
# if we're supposed to.
if mac_developer_dir != "none":
  print "Setting DEVELOPER_DIR to %s" % mac_developer_dir
  os.environ['DEVELOPER_DIR']=mac_developer_dir

subprocess.call(["pwd"])
build_ffmpeg = os.path.join("chromium", "scripts", "build_ffmpeg.py")
subprocess.call([build_ffmpeg, platform, arch, '--', ldflags, yasmflags])

# print log files for debugging.
# then again, we're about to copy these.
print "========= Chrome"
subprocess.call(["cat", os.path.join(sys.argv[1], "build.%s.%s" % (sys.argv[3], sys.argv[2]), "Chrome", "config.log")])
print "========= Chromium"
subprocess.call(["cat", os.path.join(sys.argv[1], "build.%s.%s" % (sys.argv[3], sys.argv[2]), "Chromium", "config.log")])
print "========= ChromeOS"
subprocess.call(["cat", os.path.join(sys.argv[1], "build.%s.%s" % (sys.argv[3], sys.argv[2]), "ChromeOS", "config.log")])

# archive the required files and log them
cmd = ['tar', 'cvzf', '-', '--wildcards', '*.o', '--no-wildcards' ]
arch = sys.argv[3]
plat = sys.argv[2]
# TODO(liberato): we really need the .o filenames too.
for product in [ 'Chrome', 'Chromium', 'ChromeOS' ]:
  os_build_dir= os.path.join("build.%s.%s" % (arch, plat), product);
  # truncate all .o's, since we don't care about the contents.
  # Also, windows....
  subprocess.call(["find", os_build_dir, "-name", "*.o", "-exec", "echo", "dd", "if=/dev/null", "of=%s/{}" % os_build_dir, ";"])
  for filename in [ 'config.h', 'config.asm', os.path.join("libavutil", "avconfig.h"), os.path.join("libavutil", "ffversion.h"), os.path.join("libavcodec", "bsf_list.c"), os.path.join("libavformat", "protocol_list.c")]:
    cmd.append(os.path.join(os_build_dir, filename))

print "tar command: %s" % cmd
tar = subprocess.Popen(cmd, stdout=subprocess.PIPE)
# linux
# uuencode = subprocess.check_output(['uuencode', '--base64', '-'], stdin=tar.stdout)
# mac
uuencode = subprocess.check_output(['uuencode', '-m', 'mac.build.tgz'], stdin=tar.stdout)
tar.wait()
print "========= build files"
print uuencode
print "========= end build files"
