#!/usr/bin/env python
# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Downloads package lists and records package versions into
dist-package-versions.json.
"""

import StringIO
import gzip
import json
import os
import re
import urllib2

SUPPORTED_DEBIAN_RELEASES = {
    'Debian 8 (Jessie)': 'jessie',
    'Debian 9 (Stretch)': 'stretch',
    'Debian 10 (Buster)': 'buster',
}

SUPPORTED_UBUNTU_RELEASES = {
    'Ubuntu 14.04 (Trusty)': 'trusty',
    'Ubuntu 16.04 (Xenial)': 'xenial',
    'Ubuntu 17.04 (Zesty)': 'zesty',
    'Ubuntu 17.10 (Artful)': 'artful',
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
    "libstdc++6",
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

deb_sources = {}
for release in SUPPORTED_DEBIAN_RELEASES:
  codename = SUPPORTED_DEBIAN_RELEASES[release]
  deb_sources[release] = [
        "http://ftp.us.debian.org/debian/dists/%s/main/binary-amd64/Packages.gz" % codename,
        "http://ftp.us.debian.org/debian/dists/%s-updates/main/binary-amd64/Packages.gz" % codename,
        "http://security.debian.org/dists/%s/updates/main/binary-amd64/Packages.gz" % codename,
  ]
for release in SUPPORTED_UBUNTU_RELEASES:
  codename = SUPPORTED_UBUNTU_RELEASES[release]
  SOURCE_FORMATS = [
      "http://us.archive.ubuntu.com/ubuntu/dists/%s/%s/binary-amd64/Packages.gz",
      "http://us.archive.ubuntu.com/ubuntu/dists/%s-updates/%s/binary-amd64/Packages.gz",
      "http://security.ubuntu.com/ubuntu/dists/%s-security/%s/binary-amd64/Packages.gz",
  ]
  sources = []
  for source_format in SOURCE_FORMATS:
    for repo in ['main', 'universe']:
      sources.append(source_format % (codename, repo))
  deb_sources[release] = sources

distro_package_versions = {}
package_regex = re.compile('^Package: (.*)$')
version_regex = re.compile('^Version: (.*)$')
for distro in deb_sources:
  package_versions = {}
  for source in deb_sources[distro]:
    response = urllib2.urlopen(source)
    zipped_file = StringIO.StringIO()
    zipped_file.write(response.read())
    zipped_file.seek(0)
    contents = gzip.GzipFile(fileobj=zipped_file, mode='rb').read()
    package = ''
    for line in contents.split('\n'):
      if line.startswith('Package: '):
        match = re.search(package_regex, line)
        package = match.group(1)
      elif line.startswith('Version: '):
        match = re.search(version_regex, line)
        version = match.group(1)
        if package in PACKAGE_FILTER:
          package_versions[package] = version
  distro_package_versions[distro] = package_versions

script_dir = os.path.dirname(os.path.realpath(__file__))
with open(os.path.join(script_dir, 'dist-package-versions.json'), 'w') as f:
  f.write(json.dumps(distro_package_versions, sort_keys=True, indent=4,
                     separators=(',', ': ')))
  f.write('\n')
