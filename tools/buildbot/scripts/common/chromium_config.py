#!/usr/bin/python
# Copyright (c) 2006-2008 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Declares a number of site-dependent variables for use by scripts.

A typical use of this module would be

  import chromium_config as config

  svn_path = config.Master.svn_binary_path
"""

import os
import sys

import chromium_config_private
reload(chromium_config_private)


class Master(chromium_config_private.Master):
  """Buildbot master configuration options."""

  trunk_url = (chromium_config_private.Master.server_url +
               chromium_config_private.Master.repo_root + '/trunk')

  webkit_mirror_url = (chromium_config_private.Master.server_url +
                       chromium_config_private.Master.webkit_root + '/trunk')

  trunk_url_src = trunk_url + '/src' 
  gears_url = trunk_url_src + '/gears'
  branch_url = (chromium_config_private.Master.server_url +
                chromium_config_private.Master.repo_root + '/branches')
  merge_branch_url = branch_url + '/chrome_webkit_merge_branch'
  merge_branch_url_src = merge_branch_url + '/src'
  v8_url = 'http://v8.googlecode.com/svn'

  # Used for the Chromium server.
  class Chromium(chromium_config_private.Master.Chromium):
    # Used for the waterfall URL and the waterfall's WebStatus object.
    master_port = 8010
    # Which port slaves use to connect to the master.
    slave_port = 8012
    # The alternate read-only page.
    master_port_alt = 8014
    # Used by the waterfall display.
    project_name = 'Chromium'
  
  class ChromiumFYI(chromium_config_private.Master.Chromium):
    # Used for the waterfall URL and the waterfall's WebStatus object.
    master_port = 8016
    # Which port slaves use to connect to the master.
    slave_port = 8017
    # The alternate read-only page.
    master_port_alt = 8019
    # Used by the waterfall display.
    project_name = 'Chromium FYI'

  # Used for the Try server.
  class TryServer(chromium_config_private.Master.TryServer):
    master_port = 8011
    slave_port = 8013
    master_port_alt = 8015
    try_job_port = 8018
    project_name = 'Chromium Try Server'

  # Used for the Google Chrome server.
  class Chrome(Chromium):
    project_name = 'Google Chrome'
    master_host = 'chrome-dev.mtv'

  class Experimental(Chromium):
    master_host = 'localhost'
  
  # Used by the SVNPoller.
  if sys.platform == 'win32':
    svn_binary_path = '../depot_tools/release/svn/svn.exe'
  else:
    svn_binary_path = 'svn'

  # Default target platform if none was given to the factory.
  default_platform = 'win32'

  # Used by the waterfall display.
  project_url = 'http://www.chromium.org'

  # Base URL for perf test results.
  perf_base_url = 'http://build.chromium.org/buildbot/perf'

  # Suffix for perf URL.
  perf_report_url_suffix = 'report.html?history=150'

  # Directory in which to save perf-test output data files.
  perf_output_dir = '~/www/perf'

  # URL pointing to builds and test results.
  archive_url = 'http://build.chromium.org/buildbot'

  # File in which to save a list of graph names.
  perf_graph_list = 'graphs.dat'

  # Magic step return code inidicating "warning(s)" rather than "error".
  retcode_warnings = -88

  bot_password = None

  @staticmethod
  def GetBotPassword():
    """Returns the slave password retrieved from a local file, or None.

    The slave password is loaded from a local file next to this module file, if
    it exists.  This is a function rather than a variable so it's not called
    when it's not needed.

    We can't both make this a property and also keep it static unless we use a
    <metaclass, which is overkill for this usage.
    """
    # Note: could be overriden by chromium_config_private.
    if not Master.bot_password:
      # If the bot_password has been requested, the file is required to exist
      # if not overriden in chromium_config_private.
      bot_password_path = os.path.join(os.path.dirname(__file__),
                                       '.bot_password')
      Master.bot_password = open(bot_password_path).read().strip('\n\r')
    return Master.bot_password


class Installer(object):
  """Installer configuration options."""

  # Executable name.
  installer_exe = 'mini_installer.exe'

  # Section in that file containing applicable values.
  file_section = 'CHROME'

  # File holding current version information.
  version_file = 'VERSION'

  # Output of mini_installer project.
  output_file = 'packed_files.txt'


class Archive(chromium_config_private.Archive):
  """Build and data archival options."""

  # List of symbol files archived by official and dev builds.
  # It's sad to have to hard-code these here.
  symbols_to_archive = ['chrome_dll.pdb', 'chrome_exe.pdb',
                        'mini_installer.pdb', 'setup.pdb', 'rlz_dll.pdb']

  # Binary to archive on the source server with the sourcified symbols.
  symsrc_binaries = ['chrome.exe', 'chrome.dll']

  # List of symbol files to save, but not to upload to the symbol server
  # (generally because they have no symbols and thus would produce an error).
  symbols_to_skip_upload = ['icudt38.dll']

  # Extra files to archive in official mode.
  official_extras = [
    ['setup.exe'],
    ['chrome.packed.7z'],
    ['patch.packed.7z'], 
    ['obj', 'mini_installer', 'mini_installer_exe_version.rc'],
  ]

  # Installer to archive.
  installer_exe = Installer.installer_exe

  # Test files to archive.
  tests_to_archive = ['reliability_tests.exe',
                      'automated_ui_tests.exe',
                      'test_shell.exe',
                      'icudt38.dll',
                      'plugins\\npapi_layout_test_plugin.dll',
                     ]
  # Archive everything in these directories, using glob.
  test_dirs_to_archive = ['fonts']
  # Create these directories, initially empty, in the archive.
  test_dirs_to_create = ['plugins', 'fonts']

  # URLs to pass to breakpad/symupload.exe.
  symbol_url = 'not available'
  symbol_staging_url = 'not available' 

  # Directories in which to store built files, for dev, official, and full
  # builds. (We don't use the full ones yet.)
  www_dir_base_dev = chromium_config_private.Archive.www_dir_base + 'snapshots'
  www_dir_base_official = (
      chromium_config_private.Archive.www_dir_base + 'official_builds')
  www_dir_base_full = 'unused'
  symbol_dir_base_dev = www_dir_base_dev
  symbol_dir_base_full = www_dir_base_full
  symbol_dir_base_official = www_dir_base_official

  # Where to find layout test results by default, above the build directory.
  layout_test_result_dir = 'layout-test-results'

  # Where to save layout test results.
  layout_test_result_archive = (
      chromium_config_private.Archive.www_dir_base + 'layout_test_results')

  # Where to save the purity test results.
  purify_test_result_archive = (
      chromium_config_private.Archive.www_dir_base + 'purify')


class IRC(chromium_config_private.IRC):
  """Options for the IRC bot."""
  # Where the IRC bot lives.
  host = 'irc.freenode.net'
  channels = ['#chromium']

  default_topic = 'IRC bot not yet connected'

  whuffie_file = '~/www/irc/whuffie_list.js'
  whuffie_reason_file = '~/www/irc/whuffie_reasons.js'
  topic_file = '~/www/irc/topic_list.js'

  # Any URLs found in IRC topics will be passed as %s to this format before
  # being added to the topic-list page. It must contain exactly one "%s" token.
  # To disable URL mangling, set this to "%s".
  href_redirect_format = 'http://www.google.com/url?sa=D&q=%s'


class Distributed(chromium_config_private.Distributed):
  # File holding current version information.
  version_file = Installer.version_file
