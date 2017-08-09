#!/bin/sh

# Copyright 2016 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# This is a simple wrapper script around the remoting_user_session binary,
# intended only for development use. It is copied into a build
# subdirectory as
# $CHROMIUM_OUTPUT_DIR/remoting/user-session
# and runs the remoting_user_session binary from the same directory via sudo to
# allow testing without making remoting_user_session setuid root. The
# linux_me2me_host.py script is also copied into the remoting/ build directory,
# so it finds this user-session wrapper script in the same directory.

# Swallow 'start' arg
shift
BUILD_DIR="$(realpath "$(dirname "$0")/..")"
exec sudo env LD_LIBRARY_PATH="$BUILD_DIR" \
  "$BUILD_DIR/remoting/remoting_user_session" start --user `whoami` "$@"
