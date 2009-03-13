#!/usr/bin/python
# Copyright (c) 2006-2008 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Seeds a number of variables defined in chromium_config.py.

The recommended way is to fork this file and use a custom DEPS forked from
config/XXX/DEPS with the right configuration data."""


class Master(object):
  # Repository URLs used by the SVNPoller and 'gclient config'.
  server_url = 'http://src.chromium.org'
  repo_root = '/svn'
  webkit_root = '/webkit-mirror'
  repo_root_internal = None
  trunk_internal_url = None
  trunk_internal_url_src = None
  gears_url_internal = None
  # Please change this accordingly.
  master_domain = 'example.com'
  smtp = 'smtp'

  class Chromium(object):
    # Actual server name.
    is_production_host = False
    master_host = 'localhost'
    tree_closing_notification_recipients = []

  class TryServer(object):
    is_production_host = False
    master_host = 'localhost'
    svn_url = ''


class Installer(object):
  # A file containing information about the last release.
  last_release_info = "."


class Archive(object):
  archive_host = 'localhost'
  # Skip any filenames (exes, symbols, etc.) starting with these strings
  # entirely, typically because they're not built for this distribution.
  exes_to_skip_entirely = []
  # Web server base path.
  www_dir_base = "\\\\" + archive_host + "\\www\\"


class IRC(object):
  bot_admins = ['root']
  nickname = 'change_me_buildbot'


class Distributed(object):
  """Not much to describe."""
