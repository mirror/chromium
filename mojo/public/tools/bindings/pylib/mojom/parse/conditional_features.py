# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
"""Helpers for processing conditionally enabled features in a mojom."""

from . import ast

def _EvalFlag(flag, enabled_flags):
  if flag.startswith('!'):
    return flag.lstrip('!') not in enabled_flags
  return flag in enabled_flags

def _Or(expression, enabled_flags):
  assert('||' in expression)
  flags = expression.split('||')
  for f in flags:
    if _EvalFlag(f, enabled_flags):
      return True
  return False

def _And(expression, enabled_flags):
  assert('&&' in expression)
  flags = expression.split('&&')
  for f in flags:
    if not _EvalFlag(f, enabled_flags):
      return False
  return True

def _IsEnabled(definition, enabled_flags):
  """Returns true if a definition is enabled.

  A definition is enabled if it has no EnabledIf attribute, or if the value of
  the EnableIf attribute is in enabled_flags.
  TODO(evem): change this as its wrong now
  """
  try:
    if not definition.attribute_list:
      return True
  except AttributeError:
    return True

  # TODO(evem): helpful message if someone has done bad syntax? :D
  for attribute in definition.attribute_list:
    if attribute.key == 'EnableIf':
      if '&&' in attribute.value:
        return _And(attribute.value, enabled_flags)
      if '||' in attribute.value:
        return _Or(attribute.value, enabled_flags)
      return _EvalFlag(attribute.value, enabled_flags)
  return True


def _FilterDisabledFromNodeList(node_list, enabled_flags):
  if node_list is None:
    return
  assert isinstance(node_list, ast.NodeListBase)
  node_list.items = [
      item for item in node_list.items if _IsEnabled(item, enabled_flags)
  ]


def _FilterEnum(enum, enabled_flags):
  assert isinstance(enum, ast.Enum)
  _FilterDisabledFromNodeList(enum.enum_value_list, enabled_flags)


def _FilterInterface(interface, enabled_flags):
  assert isinstance(interface, ast.Interface)
  _FilterDisabledFromNodeList(interface.body, enabled_flags)
  for definition in interface.body:
    _FilterDefinition(definition, enabled_flags)


def _FilterMethod(method, enabled_flags):
  assert isinstance(method, ast.Method)
  _FilterDisabledFromNodeList(method.parameter_list, enabled_flags)
  _FilterDisabledFromNodeList(method.response_parameter_list, enabled_flags)


def _FilterStruct(struct, enabled_flags):
  assert isinstance(struct, ast.Struct)
  _FilterDisabledFromNodeList(struct.body, enabled_flags)
  if struct.body is None:
    return
  for definition in struct.body:
    _FilterDefinition(definition, enabled_flags)


def _FilterUnion(union, enabled_flags):
  assert isinstance(union, ast.Union)
  _FilterDisabledFromNodeList(union.body, enabled_flags)


def _FilterDefinition(definition, enabled_flags):
  """Helper for filtering definitions with a body."""
  if isinstance(definition, ast.Enum):
    _FilterEnum(definition, enabled_flags)
  elif isinstance(definition, ast.Interface):
    _FilterInterface(definition, enabled_flags)
  elif isinstance(definition, ast.Method):
    _FilterMethod(definition, enabled_flags)
  elif isinstance(definition, ast.Struct):
    _FilterStruct(definition, enabled_flags)
  elif isinstance(definition, ast.Union):
    _FilterUnion(definition, enabled_flags)


def RemoveDisabledDefinitions(mojom, enabled_flags):
  """Removes conditionally disabled definitions from a Mojom node."""
  mojom.definition_list = [
      definition for definition in mojom.definition_list
      if _IsEnabled(definition, enabled_flags)
  ]
  for definition in mojom.definition_list:
    _FilterDefinition(definition, enabled_flags)