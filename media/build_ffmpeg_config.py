#!/usr/bin/env python
# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import os
import subprocess
import sys

print os.getcwd()
root_build_dir = os.getcwd()

# go to third_party/ffmpeg
os.chdir(sys.argv[1])

print "gen_dir: %s" % sys.argv[4]
print "out_dir: %s" % sys.argv[5]

# TODO(liberato): how do we get this information from gn?
opus_path= "%s/%s/../third_party/opus" % (root_build_dir, sys.argv[5])
yasm_path="%s/yasm" % root_build_dir

# Sanity
print "opus_path:"
subprocess.call(["ls", opus_path])
print "yasm_path:"
subprocess.call(["ls", yasm_path])

#print "========= yasm"
#subprocess.call(["yasm"])
print "========= build_ffmpeg"
# copy our build_ffmpeg over third_party/ffmpeg so that w don't have to modify
# ffmpeg for testing.
#subprocess.call(["cp", "-f", "../../media/build_ffmpeg.py", "./chromium/scripts/build_ffmpeg.py"])

# for mac, we should point configure at chromium's libopus
ldflags="--extra-ldflags=-L%s" % opus_path
print ldflags

yasmflags="--yasmexe=%s" % yasm_path
print yasm_path

subprocess.call(["./chromium/scripts/build_ffmpeg.py", sys.argv[2], sys.argv[3], '--', ldflags, yasmflags])

# Print the config log in case it fails.
print "==== config.log"
subprocess.call(["cat", "ffbuild/config.log"])

# print log files
print "========= Chrome"
subprocess.call(["cat", os.path.join(sys.argv[1], "build.%s.%s" % (sys.argv[3], sys.argv[2]), "Chrome", "config.log")])
print "========= Chromium"
subprocess.call(["cat", os.path.join(sys.argv[1], "build.%s.%s" % (sys.argv[3], sys.argv[2]), "Chromium", "config.log")])
print "========= ChromeOS"
subprocess.call(["cat", os.path.join(sys.argv[1], "build.%s.%s" % (sys.argv[3], sys.argv[2]), "ChromeOS", "config.log")])

# archive the required files and log them
cmd = ['tar', 'cvzf', '-' ]
arch = sys.argv[3]
plat = sys.argv[2]
for os in [ 'Chrome', 'Chromium', 'ChromeOS' ]:
  for filename in [ 'config.h', 'config.asm', 'libavutil/avconfig.h', 'libavutil/ffversion.h', 'libavcodec/bsf_list.c', 'libavformat/protocol_list.c']:
    cmd.append("build.%s.%s/%s/%s" % (arch, plat, os, filename))

tar = subprocess.Popen(cmd, stdout=subprocess.PIPE)
uuencode = subprocess.check_output(['uuencode', '--base64', '-'], stdin=tar.stdout)
tar.wait()
print "========= build files"
print uuencode
print "========= end build files"
