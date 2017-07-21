# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import collections
import re

from webkitpy.common.path_finder import PathFinder
from webkitpy.common.system.filesystem import FileSystem


# Format of OWNERS files can be found at //src/third_party/depot_tools/owners.py
# In our use case, we only care about lines that are valid email addresses.
# Recognizes 'X@Y' email addresses. Very simplistic. (from owners.py)
BASIC_EMAIL_REGEXP = r'^[\w\-\+\%\.]+\@[\w\-\+\%\.]+$'


class DirectoryOwnersExtractor(object):

    def __init__(self, filesystem=None):
        self.filesystem = filesystem or FileSystem()
        self.finder = PathFinder(filesystem)
        self.owner_map = None

    def list_owners(self, changed_files):
        """Looks up the owners for the given set of changed files.

        Args:
            changed_files: A list of file paths relative to the repository root.

        Returns:
            A dict mapping tuples of owner email addresses to lists of
            owned directories (paths relative to the root of layout tests).
        """
        email_map = collections.defaultdict(list)
        for relpath in changed_files:
            if self.finder.layout_test_name(relpath) is None:
                continue
            owners_file = self.find_owners_file(self.filesystem.dirname(relpath))
            if not owners_file:
                continue
            owners = self.extract_owners(owners_file)
            owned_directory = self.finder.layout_test_name(self.filesystem.dirname(owners_file))
            email_map[tuple(owners)].append(owned_directory)
        return email_map

    def find_owners_file(self, start_directory):
        """Walks up the directory tree to find the first OWNERS file, starting from (and including) start_directory.

        Stops when reaching the root of the repository.

        Args:
            start_directory: A relative path from the root of the repository.

        Returns:
            The path to the first OWNERS file found, relative to the repository root; or None if not found.
        """
        directory = start_directory
        while True:
            owners_file = self.filesystem.join(directory, 'OWNERS')
            if self.filesystem.isfile(self.finder.path_from_chromium_base(owners_file)):
                return owners_file
            if directory == '':
                return None
            directory = self.filesystem.dirname(directory)

    def extract_owners(self, owners_file):
        """Extract owners from an OWNERS file.

        Args:
            owners_file: A relative path to an OWNERS file from the repository root.

        Returns:
            A list of valid owners (email addresses).
        """
        contents = self.filesystem.read_text_file(self.finder.path_from_chromium_base(owners_file))
        email_regexp = re.compile(BASIC_EMAIL_REGEXP)
        addresses = []
        for line in contents.splitlines():
            line = line.strip()
            if email_regexp.match(line):
                addresses.append(line)
        return addresses
