# Copyright (c) 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# TODO(ymzhang): external dependencies in README.md
import MySQLdb
import pandas

# TODO(ymzhang): check hostname for the IP address.
MONORAIL_HOST = "173.194.230.234"
# Request access to monorail database for your user_name
USER_NAME = "user_name"
MONORAIL_DB = "monorail"


class ComponentTree(object):
  """A representation of monorail components."""

  def __init__(self):
    self.component_defs = []
    self.component_labels = []
    self.component_docstrings = []
    self.deprecated_components = []

  def table_to_dataframe(self, name, connection):
    """Convert data from database into dataframe."""
    return pandas.read_sql(name , con=connection)

  def get_component_from_database(self, host, user, db):
    """Get monorail components from database."""
    connection = MySQLdb.connect(host=host, user=user,
                                 db=db)
    component_def = self.table_to_dataframe(
        "SELECT * FROM ComponentDef WHERE project_id=16;", connection)
    component_def = component_def.where((pandas.notnull(component_def)),
                                        0.0)
    component_def = component_def[component_def["deprecated"] == 0.0]
    self.component_defs = component_def["path"].tolist()
    self.component_docstrings = component_def["docstring"].tolist()
    self.deprecated_components = component_def["deprecated"].tolist()

  def get_component_labels(self):
    """Get complete labels for each components.

    Example:
      [Blink>Layout]:[Blink, Blink>Layout]
    """
    if not self.component_defs:
      self.get_component_from_database(MONORAIL_HOST, USER_NAME,
                                       MONORAIL_DB)

    for component_def in self.component_defs:
      component_list = component_def.split('>')
      tmp_component_label = component_list[0]
      component_label = [tmp_component_label]
      for i in range(1, len(component_list)):
        tmp_component_label += ">" + component_list[i]
        component_label.append(tmp_component_label)

      self.component_labels.append(component_label)
