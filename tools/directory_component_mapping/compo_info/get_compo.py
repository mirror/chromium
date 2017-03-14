# Copyright (c) 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import MySQLdb as mdb
import pandas as pd
import math

MONORAIL_HOST = "173.194.230.234"
USER_NAME = "user name to query monorail database"
MONORAIL_DB = "monorail"


class CompoTree(object):
  def __init__(self):
    self.compo_path_list = []
    self.compo_par_list = []
    self.compo_docstring_list = []
    self.deprecated_list = []
    self.size = 0
    self.internal_node = []

  def table_to_dataframe(self, name, connection):
    return pd.read_sql(name , con=connection)

  def GetCompoFromDB(self):
    connection = mdb.connect(host=MONORAIL_HOST, user=USER_NAME, db=MONORAIL_DB)
    component_def = self.table_to_dataframe(
        "SELECT * FROM ComponentDef WHERE project_id=16;", connection)
    component_def = component_def.where((pd.notnull(component_def)), 0.0)
    component_def = component_def[component_def["deprecated"] == 0.0]
    self.compo_path_list = component_def["path"].tolist()
    self.compo_docstring_list = component_def["docstring"].tolist()
    self.deprecated_list = component_def["deprecated"].tolist()
    self.size = len(self.compo_path_list)

  def GetCompoParent(self):
    for compo_path in self.compo_path_list:
      tmp_label = []
      compo_list = compo_path.split('>')
      tmp = compo_list[0]
      tmp_label.append(tmp)
      for i in range(1, len(compo_list)):
        tmp = tmp + ">" + compo_list[i]
        tmp_label.append(tmp)
      self.compo_par_list.append(tmp_label)
