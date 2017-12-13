# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Downloads new Render Test golden files from the last trybot run."""

import os
import sys

from pylib.constants import host_paths

if host_paths.WEBKITPY_PATH not in sys.path:
  sys.path.append(host_paths.WEBKITPY_PATH)

from webkitpy.common.net.git_cl import GitCL, TryJobStatus
from webkitpy.common.host import Host

host = Host()
gc = GitCL(host)
# latest = gc.latest_try_jobs(['linux_android_rel_ng'])
latest = gc.try_job_results(['linux_android_rel_ng'])
print latest
# latest = {Build(builder_name=u'linux_android_rel_ng', build_number=428776): TryJobStatus(status=u'STARTED', result=None)}

for build, status in latest.iteritems():
    results_url = host.buildbot.results_url(build.builder_name, build.build_number)
    test_results = host.buildbot.fetch_results(build)
    print test_results

print "Done"
