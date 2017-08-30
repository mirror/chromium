#!/usr/bin/env python
# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Given a binary, uses dpkg-shlibdeps to check that its package dependencies
are satisfiable on all supported debian-based distros.
"""

import json
import os
import re
import subprocess
import sys

def compare_int(left, right):
  if left == right:
    return 0
  return -1 if left < right else 1

def compare_char(left, right):
  # 'man deb-version' specifies that characters are compared using
  # their ASCII values, except alphabetic characters come before
  # special characters and ~ comes before everything else.
  table = '~$ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz+-.:'
  left = table.find(left)
  right = table.find(right)
  assert left >= 0 and right >= 0
  return compare_int(left, right)

def compare_part(left, right):
  if isinstance(left, int) and isinstance(right, int):
    return compare_int(left, right)
  assert isinstance(left, (str, unicode)) and isinstance(right, (str, unicode))
  # 'man deb-version' specifies that '~' should be matched before the
  # empty string.  Add a '$' to the end of the strings to make this
  # comparison easier.
  left += '$'
  right += '$'
  i = 0
  while i < len(left) and i < len(right):
    comp = compare_char(left[i], right[i])
    if comp != 0:
      return comp
    i += 1
  return compare_int(len(left), len(right))

def get_parts_from_component(component):
  # 'man deb-version' specifies that components should be compared
  # part-by-part, where parts are either strings or numbers.  Strings
  # are compared lexicographically while numbers are compared using
  # magnitude.  Components must start with a string part and end with
  # a number part.  The empty string will be prepended and 0 will be
  # appended to satisfy this requirement.  For example, the component
  # '1.2.3' will be expanded to ['', 1, '.', 2, '.', 3], and the empty
  # component will be expanded to ['', 0].
  part_is_string = True
  parts = []
  part = ''
  for c in component:
    char_is_string = not c.isdigit()
    if char_is_string != part_is_string:
      parts.append(part if part_is_string else int(part))
      part = ''
      part_is_string = char_is_string
    part += c
  if part_is_string:
    parts.append(part)
    part = ''
  parts.append(0 if part == '' else int(part))
  return parts

def compare_component(left, right):
  i = 0
  left = get_parts_from_component(left)
  right = get_parts_from_component(right)
  while i < len(left) and i < len(right):
    comp = compare_part(left[i], right[i])
    if comp != 0:
      return comp
    i += 1
  return compare_int(len(left), len(right))

class DebVersion:
  def __init__(self, version_string):
    self.version_string = version_string
    self.epoch = 0
    self.upstream_version = ''
    self.debian_revision = None

    colon = version_string.find(':')
    if colon >= 0:
      self.epoch = int(version_string[:colon])
    hyphen = version_string.rfind('-')
    if hyphen >= 0:
      self.debian_revision = version_string[hyphen + 1:]
    upstream_version_start = colon + 1
    upstream_version_end = hyphen if hyphen >= 0 else len(version_string)
    self.upstream_version = version_string[upstream_version_start:
                                           upstream_version_end]

  def __str__(self):
    return 'DebVersion(%s)' % self.version_string

  # Comparison algorithm is specified in 'man deb-version'.
  def __cmp__(self, other):
    assert(isinstance(other, DebVersion))

    # Epoch comparison.
    if self.epoch != other.epoch:
      return 1 if self.epoch > other.epoch else 0

    # Upstream version comparison.
    upstream_version_cmp = compare_component(self.upstream_version,
                                             other.upstream_version)
    if upstream_version_cmp != 0:
      return upstream_version_cmp

    # Debian revision comparison.
    if self.debian_revision == None and other.debian_revision == None:
      return 0
    if self.debian_revision == None:
      return -1
    if other.debian_revision == None:
      return 1
    return compare_component(self.debian_revision, other.debian_revision)

if len(sys.argv) != 4:
  print 'Usage: %s /path/to/binary /path/to/sysroot arch' % sys.argv[0]
  sys.exit(1)

binary = os.path.abspath(sys.argv[1])
sysroot = os.path.abspath(sys.argv[2])
arch = sys.argv[3]

cmd = ['dpkg-shlibdeps']
if arch == 'amd64':
  cmd.extend(['-l%s/usr/lib/x86_64-linux-gnu' % sysroot,
              '-l%s/lib/x86_64-linux-gnu' % sysroot])
elif arch == 'i386':
  cmd.extend(['-l%s/usr/lib/i386-linux-gnu' % sysroot,
              '-l%s/lib/i386-linux-gnu' % sysroot])
else:
  print 'Unsupported architecture ' + arch
  sys.exit(1)
cmd.extend(['-l%s/usr/lib' % sysroot, '-O', '-e', binary])

proc = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                        cwd=sysroot)
exit_code = proc.wait()
(stdout, stderr) = proc.communicate()
if exit_code != 0:
  print 'dpkg-shlibdeps failed with exit code ' + str(exit_code)
  print 'stderr was ' + stderr
  sys.exit(1)

def version_statement_satisfied(left_version, op, right_version):
  # Allowed relationship operators are specified in:
  # https://www.debian.org/doc/debian-policy/ch-relationships.html
  if op == '>=':
    return left_version >= right_version
  elif op == '<=':
    return left_version <= right_version
  elif op == '>>':
    return left_version > right_version
  elif op == '<<':
    return left_version < right_version
  assert op == '='
  return left_version == right_version

def get_package_and_version_requirement(dep):
  package_name_regex = '[a-z][a-z0-9\+\-\.]+'
  match = re.match('^(%s)$' % package_name_regex, dep)
  if match:
    return (match.group(1), lambda version: True)
  match = re.match('^(%s) \(([\>\=\<]+) ([\~0-9A-Za-z\+\-\.\:]+)\)$' %
                   package_name_regex, dep)
  if match:
    return (match.group(1), lambda version: version_statement_satisfied(
        version, match.group(2), DebVersion(match.group(3))))
  # At the time of writing this script, Chrome does not have any
  # complex version requirements like 'version >> 3 | version << 2'.
  print ('Conjunctions and disjunctions in package version requirements are ' +
         'not implemented at this time.')
  sys.exit(1)

deps = stdout.replace('shlibs:Depends=', '').replace('\n', '').split(', ')
package_requirements = {}
for dep in deps:
  (package, requirement) = get_package_and_version_requirement(dep)
  package_requirements[package] = requirement

script_dir = os.path.dirname(os.path.abspath(__file__))
deps_file = os.path.join(script_dir, 'dist-package-versions.json')
distro_package_versions = json.load(open(deps_file))

ret_code = 0
for distro in distro_package_versions:
  for package in package_requirements:
    if package not in distro_package_versions[distro]:
      print >> sys.stderr, (
          'Unexpected new dependency %s on distro %s caused by binary %s' % (
              package, distro, os.path.basename(binary)))
      ret_code = 1
      continue
    distro_version = DebVersion(distro_package_versions[distro][package])
    if not package_requirements[package](distro_version):
      print >> sys.stderr, 'Dependency on package %s not satisfiable on %s' % (
          package, distro)
      ret_code = 1
exit(ret_code)
