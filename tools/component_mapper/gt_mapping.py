# Copyright (c) 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from collections import defaultdict
import json
import os
import re


PREDATOR_MAPPING_FILE = (
    "/dir/to/mapping/file/component_mapping.json")
LOCAL_PATH = "/usr/local/dir/to/chromium/"


class GroundTruthMapping(object):
  """A representation of ground truth mappings.

  Ground Truth mappings refer to mappings that have been verified by
  owners and are known to be correct.

  Attributes:
    mapping: a dict of "src/dir/to/path: component label(s)" list all
      mappings in json file.
    complete_mapping: a dict of "/src/dir/to/path: component label(s)",
      including subdirectires and their files.
  """

  def __init__(self):
    self.mappings = {}
    self.complete_mappings = {}

  def load_mapping_from_json_file(self,
                                  mapping_file=PREDATOR_MAPPING_FILE):
    try:
      with open(mapping_file) as data_file:
        data = json.load(data_file)
      for path, _, component in data['path_function_component']:
        self.mappings[path] = component.split('\n')
    except IOError as err:
      print err.args
      raise

  def add_parent_component(self):
    """Add parent component labels.

    Example:
      Label [Blink>GetUserMedia>Tab] becomes [Blink, Blink>GetUserMedia,
      Blink>GetUserMedia>Tab]
    """
    for dir_path, components in self.mappings.items():
      component_set = set(components)
      for tmp_component in components:
        component_list = tmp_component.split('>')
        if len(component_list) > 1:
          component_to_add = component_list[0]
          component_set.add(component_to_add)
          for i in range(1, len(component_list) - 1):
            component_to_add += '>' + component_list[i]
            component_set.add(component_to_add)
        self.mappings[dir_path] = list(component_set)

  def get_subdir_list(self, relative_dir_path, local_append):
    """Generate all directories under given folder.

    Args:
      local_append: path prefix to adjust to local path.
      relative_dir_path: path under /src/.

    Returns:
      sub_dirs_files(list): a list of all sub-directories and files
        under the given directory.

    Example:
      local_append: '/usr/local/google/home/ymzhang/chromium/'
      relative_dir_path: 'src/third_party/WebKit'
      sub_dirs_files = GenerateTestDir(relative_dir_path, local_append)
    """
    dir_list = []
    file_list = []
    for dirName, subdirList, fileList in os.walk(local_append
                                                 + relative_dir_path):
      dir_list.append(dirName.replace(local_append, ""))
      for fname in fileList:
        if ((not fname.endswith(".txt"))
            or (not fname.endswith(".pyc"))):
          file_list.append(os.path.join(dir_list[-1], fname))

    sub_dirs_files = dir_list + file_list
    return sub_dirs_files

  def get_complete_dir(self, dir_path, local_path_append):
    """Get complete directory paths match expression dir_path.

    Args:
      dir_path (string): a directory path string including ".*".
    """
    dirs = dir_path.split('/')
    dir_pre = ""
    dir_index = 0
    for dir_index in range(0, len(dirs)):
      if dirs[dir_index].find(".*") >= 0:
        break
      dir_pre = dir_pre + "/" + dirs[dir_index]

    dir_helper = self.get_subdir_list(dir_pre, local_path_append)
    for tmp_dir in dir_helper:
      if (len(tmp_dir) > 0) and (tmp_dir[0] == '/'):
        # remove the "/" added at the begining in dir_pre
        tmp_dir = tmp_dir[1:]
      dir_pattern = re.compile(dir_path, re.IGNORECASE)
      if ((dir_index + 1  == len(dirs) and dirs[dir_index] == ".*")
          or dir_pattern.match(tmp_dir)):
        self.complete_mappings[tmp_dir] = self.mappings[dir_path]

  def normalize_mapping(self, local_path_append=LOCAL_PATH):
    """Get complete mappings.

    Explicitly list all mappings (e.g. replace dir/.* with a complete
    list of subdirectories and files).
    """
    for dir_path, components in self.mappings.items():
      if dir_path.find(".*") < 0:
        self.complete_mappings[dir_path] = components
      else:
        self.get_complete_dir(dir_path, local_path_append)
