#!/usr/bin/env python
#
# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Shamefully stolen from mac_toolchain.py to install Xcode via swarming.  This
should really be shared.

Only notable changes is the call to gsutil.py passes the full /b/depot_tools/
path.

Also, since Xcode9-beta4 is already installed on the bots, I'm not sure what
should be installed here.

Run with the following to only effect build23-m7.
  ./client/tools/run_on_bots.py  --swarming touch-swarming.appspot.com --isolate-server touch-isolate.appspot.com --priority 20 --dimension pool Chrome --dimension id build23-m7 ~/work/bling/src/build/swarming_xcode_install.py
"""

url = 'gs://chrome-mac-sdk/ios-toolchain-9M214v-1.tgz'
output_dir = '/Applications/Xcode-Swarming.app/'
target_os = 'ios'

############ Shamefully stolen from DownloadAndUnpack ############

temp_name = tempfile.mktemp(prefix='mac_toolchain')
try:
  print 'Downloading new toolchain.'
  subprocess.check_call(['/b/depot_tools/gsutil.py', 'cp', url, temp_name])
  if os.path.exists(output_dir):
    print 'Deleting old toolchain.'
    shutil.rmtree(output_dir)
  EnsureDirExists(output_dir)
  print 'Unpacking new toolchain.'
  tarfile.open(mode='r:gz', name=temp_name).extractall(path=output_dir)
finally:
  if os.path.exists(temp_name):
    os.unlink(temp_name)

############ Shamefully stolen from FinalizeUnpack. ############

# Check old license
try:
  target_license_plist_path = \
      os.path.join(output_dir, *['Contents','Resources','LicenseInfo.plist'])
  target_license_plist = LoadPlist(target_license_plist_path)
  build_type = target_license_plist['licenseType']
  build_version = target_license_plist['licenseID']

  accepted_license_plist = LoadPlist(
      '/Library/Preferences/com.apple.dt.Xcode.plist')
  agreed_to_key = 'IDELast%sLicenseAgreedTo' % build_type
  last_license_agreed_to = accepted_license_plist[agreed_to_key]

  # Historically all Xcode build numbers have been in the format of AANNNN, so
  # a simple string compare works.  If Xcode's build numbers change this may
  # need a more complex compare.
  if build_version <= last_license_agreed_to:
    # Don't accept the license of older toolchain builds, this will break the
    # license of newer builds.
    return
except (subprocess.CalledProcessError, KeyError):
  # If there's never been a license of type |build_type| accepted,
  # |target_license_plist_path| or |agreed_to_key| may not exist.
  pass

print "Accepting license."
target_version_plist_path = \
    os.path.join(output_dir, *['Contents','version.plist'])
target_version_plist = LoadPlist(target_version_plist_path)
short_version_string = target_version_plist['CFBundleShortVersionString']
old_path = subprocess.Popen(['/usr/bin/xcode-select', '-p'],
                             stdout=subprocess.PIPE).communicate()[0].strip()
try:
  build_dir = os.path.join(output_dir, 'Contents/Developer')
  subprocess.check_call(['sudo', '/usr/bin/xcode-select', '-s', build_dir])
  subprocess.check_call(['sudo', '/usr/bin/xcodebuild', '-license', 'accept'])

  if target_os == 'ios' and \
      LooseVersion(short_version_string) >= LooseVersion("9.0"):
    print "Installing packages."
    subprocess.check_call(['sudo', '/usr/bin/xcodebuild', '-runFirstLaunch'])
finally:
  subprocess.check_call(['sudo', '/usr/bin/xcode-select', '-s', old_path])

