# Copyright (c) 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from collections import defaultdict
import os
import re
import json


FINDIT_MAPPING_FILE = "/path/to/component_mapping.json"
LOCAL_PATH = "/path/to/chromium/"


class GTMapping(object):
  def __init__(self):
    # a dict {/dir/path/in/src: component name}
    self.mapping = {} # /src/...
    # a dict {/dir/path/in/local/machine: component name}
    self.local_mapping = {} # directory local path
    # a dict {/dir/math/in/src: component name}
    # the mapping includes subdirectories/files
    self.complete_mapping = {}

  def LoadMappingFromJsonFile(self, mapping_file = FINDIT_MAPPING_FILE):
    """ load mapping from json file """
    try:
      with open(mapping_file) as data_file:
        data = json.load(data_file)
      for path, function, name in data['path_function_component']:
        self.mapping[path] = name.split('\n')
    except Exception:
      print "invalid ground truth loading"

  def AddParentCompo(self):
    """ add parent component label
    e.g. [Blink>GetUserMedia>Tab]
    --> [Blink, Blink>GetUserMedia, Blink>GetUserMedia>Tab]
    """
    for key, value in self.mapping.items():
      tmp = set(value)
      for compo in value:
        compo_list = compo.split('>')
        if len(compo_list) > 1:
          compo_to_add = compo_list[0]
          tmp.add(compo_to_add)
          for i in range(1, len(compo_list) - 1):
            compo_to_add = compo_to_add + '>' + compo_list[i]
            tmp.add(compo_to_add)
        self.mapping[key] = list(tmp)

  def GetSubDirList(self, relative_dir_path, local_append):
    """generate all directories under given folder

    Args:
    local_append: path prefix to adjust to local path
    relative_dir_path: path under /src/

    Returns:
    dir_list: list of all sub-directories, including itself
    file_list: list of all files

    Example:
    local_append: '/usr/local/google/home/ymzhang/chromium/'
    relative_dir_path: 'src/third_party/WebKit'
    dir_list, file_list = GenerateTestDir(relative_dir_path, local_append)

    """
    dir_list = []
    file_list = []
    for dirName, subdirList, fileList in os.walk(local_append +
                                                 relative_dir_path):
      dir_list.append(dirName.replace(local_append, ""))
      for fname in fileList:
        if ((not fname.endswith(".txt"))
            or (not fname.endswith(".pyc"))):
          file_list.append(os.path.join(dir_list[-1], fname))
    return dir_list + file_list

  def GetCompleteDir(self, dir_path):
    """
    Get complete path represented by .*
    """
    dirs = dir_path.split('/')
    dir_pre = ""
    i = 0
    for i in range(0, len(dirs)):
      if dirs[i].find(".*") >= 0:
        break
      dir_pre = dir_pre + "/" + dirs[i]
    dir_helper = self.GetSubDirList(dir_pre, LOCAL_PATH)
    for tmp_dir in dir_helper:
      if (len(tmp_dir) > 0) and (tmp_dir[0] == '/'):
        # remove the "/" added at the begining in dir_pre
        tmp_dir = tmp_dir[1:]
      dir_pattern = re.compile(dir_path, re.IGNORECASE)
      if ((i + 1  == len(dirs) and dirs[i] == ".*")
          or dir_pattern.match(tmp_dir)):
        self.complete_mapping[tmp_dir] = self.mapping[dir_path]

  def NormalizeMapping(self):
    for key, value in self.mapping.items():
      if key.find(".*") < 0:
        self.complete_mapping[key] = value
      else:
        self.GetCompleteDir(key)
