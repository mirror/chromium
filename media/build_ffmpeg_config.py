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

src_dir= "%s/../.." % root_build_dir
print "src_dir: %s" % src_dir
print "gen_dir: %s" % sys.argv[4]
print "out_dir: %s" % sys.argv[5]
print "use_system_xcode: %s" % sys.argv[6]
print "hermetic_xcode_path: %s" % sys.argv[7]

# TODO(liberato): how do we get this information from gn?
opus_path= "%s/%s/../third_party/opus" % (root_build_dir, sys.argv[5])
yasm_path="%s/yasm" % root_build_dir

# Sanity
print "opus_path:"
subprocess.call(["ls", "-l", opus_path])
print "yasm_path:"
subprocess.call(["ls", "-l", yasm_path])

print "should use hermetic toolchain:"
subprocess.call(["%s/build/mac/should_use_hermetic_xcode.py" % src_dir, "mac"])

print "sdk path:"
subprocess.call(["%s/build/mac/find_sdk.py" % src_dir, "--print_sdk_path", "10.6"])

#print "========= yasm"
#subprocess.call(["yasm"])

print "========= build_ffmpeg"
# copy our build_ffmpeg over third_party/ffmpeg so that we don't have to modify
# ffmpeg for testing.  normally, this would be committed in third_party/ffmpeg
# and the CL would include a change to DEPS to point to the right sha1.
#subprocess.call(["cp", "-f", "../../media/build_ffmpeg.py", "./chromium/scripts/build_ffmpeg.py"])

# for mac, we should point configure at chromium's libopus
ldflags="--extra-ldflags=-L%s" % opus_path
print ldflags

yasmflags="--yasmexe=%s" % yasm_path
print yasm_path

logflags="--logfile=config.log"
print logflags

# TODO(liberato): we really should check if we're supposed to use the hermetic
# toolchain, via FORCE_MAC_TOOLCHAIN
#os.environ['DEVELOPER_DIR']=sys.argv[6]

subprocess.call(["pwd"])
subprocess.call(["./chromium/scripts/build_ffmpeg.py", sys.argv[2], sys.argv[3], '--', ldflags, yasmflags, logflags])

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
# linux
# uuencode = subprocess.check_output(['uuencode', '--base64', '-'], stdin=tar.stdout)
# mac
uuencode = subprocess.check_output(['uuencode', '-m', 'mac.build.tgz'], stdin=tar.stdout)
tar.wait()
print "========= build files"
print uuencode
print "========= end build files"
