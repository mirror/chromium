#!/usr/bin/env python
# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Downloads package lists and records them in deps files.
"""

import gzip
import json
import os
import re
import StringIO
import urllib2

DEB_SOURCES = {
    'Debian 8 (Jessie)': [
        "http://ftp.us.debian.org/debian/dists/jessie/main/binary-amd64/Packages.gz",
        "http://ftp.us.debian.org/debian/dists/jessie-updates/main/binary-amd64/Packages.gz",
        "http://security.debian.org/dists/jessie/updates/main/binary-amd64/Packages.gz",
    ],
    'Debian 9 (Stretch)': [
        "http://ftp.us.debian.org/debian/dists/stretch/main/binary-amd64/Packages.gz",
        "http://ftp.us.debian.org/debian/dists/stretch-updates/main/binary-amd64/Packages.gz",
        "http://security.debian.org/dists/stretch/updates/main/binary-amd64/Packages.gz",
    ],
    'Debian 10 (Buster)': [
        "http://ftp.us.debian.org/debian/dists/buster/main/binary-amd64/Packages.gz",
        "http://ftp.us.debian.org/debian/dists/buster-updates/main/binary-amd64/Packages.gz",
        "http://security.debian.org/dists/buster/updates/main/binary-amd64/Packages.gz",
    ],
    'Ubuntu 14.04 (Trusty)': [
        "http://us.archive.ubuntu.com/ubuntu/dists/trusty/main/binary-amd64/Packages.gz",
        "http://us.archive.ubuntu.com/ubuntu/dists/trusty-updates/main/binary-amd64/Packages.gz",
        "http://security.ubuntu.com/ubuntu/dists/trusty-security/main/binary-amd64/Packages.gz",
    ],
    'Ubuntu 16.04 (Xenial)': [
        "http://us.archive.ubuntu.com/ubuntu/dists/xenial/main/binary-amd64/Packages.gz",
        "http://us.archive.ubuntu.com/ubuntu/dists/xenial-updates/main/binary-amd64/Packages.gz",
        "http://security.ubuntu.com/ubuntu/dists/xenial-security/main/binary-amd64/Packages.gz",
    ],
    'Ubuntu 17.04 (Zesty)': [
        "http://us.archive.ubuntu.com/ubuntu/dists/zesty/main/binary-amd64/Packages.gz",
        "http://us.archive.ubuntu.com/ubuntu/dists/zesty-updates/main/binary-amd64/Packages.gz",
        "http://security.ubuntu.com/ubuntu/dists/zesty-security/main/binary-amd64/Packages.gz",
    ],
    'Ubuntu 17.10 (Artful)': [
        "http://us.archive.ubuntu.com/ubuntu/dists/artful/main/binary-amd64/Packages.gz",
        "http://us.archive.ubuntu.com/ubuntu/dists/artful-updates/main/binary-amd64/Packages.gz",
        "http://security.ubuntu.com/ubuntu/dists/artful-security/main/binary-amd64/Packages.gz",
    ],
}

PACKAGE_FILTER = set([
    "gconf-service",
    "libasound2",
    "libatk1.0-0",
    "libc6",
    "libcairo2",
    "libcups2",
    "libdbus-1-3",
    "libexpat1",
    "libfontconfig1",
    "libgcc1",
    "libgconf-2-4",
    "libgdk-pixbuf2.0-0",
    "libglib2.0-0",
    "libgtk-3-0",
    "libnspr4",
    "libnss3",
    "libpango-1.0-0",
    "libpangocairo-1.0-0",
    "libx11-6",
    "libx11-xcb1",
    "libxcb1",
    "libxcomposite1",
    "libxcursor1",
    "libxdamage1",
    "libxext6",
    "libxfixes3",
    "libxi6",
    "libxrandr2",
    "libxrender1",
    "libxss1",
    "libxtst6",
])

distro_package_versions = {}
for distro in DEB_SOURCES:
  package_versions = {}
  for source in DEB_SOURCES[distro]:
    response = urllib2.urlopen(source)
    zipped_file = StringIO.StringIO()
    zipped_file.write(response.read())
    zipped_file.seek(0)
    contents = gzip.GzipFile(fileobj=zipped_file, mode='rb').read()
    package = ''
    for line in contents.split('\n'):
      if line.startswith('Package: '):
        match = re.search('^Package: (.*)$', line)
        package = match.group(1)
      elif line.startswith('Version: '):
        match = re.search('^Version: (.*)$', line)
        version = match.group(1)
        if package in PACKAGE_FILTER:
          package_versions[package] = version
  distro_package_versions[distro] = package_versions

script_dir = os.path.dirname(os.path.realpath(__file__))
with open(os.path.join(script_dir, 'dist-package-versions.json'), 'w') as f:
  f.write(json.dumps(distro_package_versions, sort_keys=True, indent=4,
                     separators=(',', ': ')))
  f.write('\n')
