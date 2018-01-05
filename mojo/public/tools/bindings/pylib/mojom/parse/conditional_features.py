# Copyright 2018 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
"""Helpers for processing conditionally enabled features in a mojom."""

from . import ast


def _IsEnabled(definition, enabled_flags):
  """Returns true if a definition is enabled.

  A definition is enabled if it has no EnableIf attribute, or if the value of
  the EnableIf attribute is in enabled_flags.
  """
  try:
    if not definition.attribute_list:
      return True
  except AttributeError:
    return True

  for attribute in definition.attribute_list:
    if attribute.key == 'EnableIf' and attribute.value not in enabled_flags:
      return False
  return True


def _FilterDisabledFromNodeList(node_list, enabled_flags):
  if not node_list:
    return
  assert isinstance(node_list, ast.NodeListBase)
  node_list.items = [
      item for item in node_list.items if _IsEnabled(item, enabled_flags)
  ]
  for item in node_list.items:
    _FilterDefinition(item, enabled_flags)


def _FilterDefinition(definition, enabled_flags):
  """Filters definitions with a body."""
  if isinstance(definition, ast.Enum):
    _FilterDisabledFromNodeList(definition.enum_value_list, enabled_flags)
  elif isinstance(definition, ast.Interface):
    _FilterDisabledFromNodeList(definition.body, enabled_flags)
  elif isinstance(definition, ast.Method):
    _FilterDisabledFromNodeList(definition.parameter_list, enabled_flags)
    _FilterDisabledFromNodeList(definition.response_parameter_list,
                                enabled_flags)
  elif isinstance(definition, ast.Struct):
    _FilterDisabledFromNodeList(definition.body, enabled_flags)
  elif isinstance(definition, ast.Union):
    _FilterDisabledFromNodeList(definition.body, enabled_flags)


def RemoveDisabledDefinitions(mojom, enabled_flags):
  """Removes conditionally disabled definitions from a Mojom node."""
  mojom.definition_list = [
      definition for definition in mojom.definition_list
      if _IsEnabled(definition, enabled_flags)
  ]
  for definition in mojom.definition_list:
    _FilterDefinition(definition, enabled_flags)
