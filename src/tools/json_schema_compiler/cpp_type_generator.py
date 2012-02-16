# Copyright (c) 2012 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from code import Code
from model import PropertyType
import cpp_util

class CppTypeGenerator(object):
  """Manages the types of properties and provides utilities for getting the
  C++ type out of a model.Property
  """
  def __init__(self, root_namespace, namespace, cpp_namespace):
    """Creates a cpp_type_generator. The given root_namespace should be of the
    format extensions::api::sub. The generator will generate code suitable for
    use in the given namespace.
    """
    self._type_namespaces = {}
    self._namespace = namespace
    self._root_namespace = root_namespace.split('::')
    self._cpp_namespaces = {}
    self.AddNamespace(namespace, cpp_namespace)

  def AddNamespace(self, namespace, cpp_namespace):
    """Maps a model.Namespace to its C++ namespace name. All mappings are
    beneath the root namespace.
    """
    for type_ in namespace.types:
      self._type_namespaces[type_] = namespace
    self._cpp_namespaces[namespace] = cpp_namespace

  def GetExpandedChoicesInParams(self, params):
    """Returns the given parameters with PropertyType.CHOICES parameters
    expanded so that each choice is a separate parameter and sets a unix_name
    for each choice.
    """
    expanded = []
    for param in params:
      if param.type_ == PropertyType.CHOICES:
        for choice in param.choices.values():
          choice.unix_name = (
              param.unix_name + '_' + choice.type_.name.lower())
          expanded.append(choice)
      else:
        expanded.append(param)
    return expanded

  def GetCppNamespaceName(self, namespace):
    """Gets the mapped C++ namespace name for the given namespace relative to
    the root namespace.
    """
    return self._cpp_namespaces[namespace]

  def GetRootNamespaceStart(self):
    """Get opening root namespace declarations.
    """
    c = Code()
    for namespace in self._root_namespace:
      c.Append('namespace %s {' % namespace)
    return c

  def GetRootNamespaceEnd(self):
    """Get closing root namespace declarations.
    """
    c = Code()
    for namespace in reversed(self._root_namespace):
      c.Append('}  // %s' % namespace)
    return c

  def GetNamespaceStart(self):
    """Get opening self._namespace namespace declaration.
    """
    return Code().Append('namespace %s {' %
        self.GetCppNamespaceName(self._namespace))

  def GetNamespaceEnd(self):
    """Get closing self._namespace namespace declaration.
    """
    return Code().Append('}  // %s' %
        self.GetCppNamespaceName(self._namespace))

  def GetChoiceEnumNoneValue(self, prop):
    """Gets the enum value of the choices in the given model.Property
    indicating no choice has been set.
    """
    return '%s_NONE' % prop.unix_name.upper()

  def GetChoiceEnumValue(self, prop, type_):
    """Gets the enum value of the choices in the given model.Property of the
    given type.

    e.g VAR_STRING
    """
    assert prop.choices[type_]
    return '%s_%s' % (prop.unix_name.upper(), type_.name)

  def GetChoicesEnumType(self, prop):
    """Gets the type of the enum for the given model.Property.

    e.g VarType
    """
    return cpp_util.Classname(prop.name) + 'Type'

  def GetType(self, prop, pad_for_generics=False, wrap_optional=False):
    """Translates a model.Property into its C++ type.

    If REF types from different namespaces are referenced, will resolve
    using self._type_namespaces.

    Use pad_for_generics when using as a generic to avoid operator ambiguity.

    Use wrap_optional to wrap the type in a scoped_ptr<T> if the Property is
    optional.
    """
    cpp_type = None
    if prop.type_ == PropertyType.REF:
      dependency_namespace = self._type_namespaces.get(prop.ref_type)
      if not dependency_namespace:
        raise KeyError('Cannot find referenced type: %s' % prop.ref_type)
      if self._namespace != dependency_namespace:
        cpp_type = '%s::%s' % (self._cpp_namespaces[dependency_namespace],
                               prop.ref_type)
      else:
        cpp_type = prop.ref_type
    elif prop.type_ == PropertyType.BOOLEAN:
      cpp_type = 'bool'
    elif prop.type_ == PropertyType.INTEGER:
      cpp_type = 'int'
    elif prop.type_ == PropertyType.DOUBLE:
      cpp_type = 'double'
    elif prop.type_ == PropertyType.STRING:
      cpp_type = 'std::string'
    elif prop.type_ == PropertyType.ANY:
      cpp_type = 'DictionaryValue'
    elif prop.type_ == PropertyType.ARRAY:
      if prop.item_type.type_ in (
          PropertyType.REF, PropertyType.ANY, PropertyType.OBJECT):
        cpp_type = 'std::vector<linked_ptr<%s> > '
      else:
        cpp_type = 'std::vector<%s> '
      cpp_type = cpp_type % self.GetType(
          prop.item_type, pad_for_generics=True)
    elif prop.type_ == PropertyType.OBJECT:
      cpp_type = cpp_util.Classname(prop.name)
    else:
      raise NotImplementedError(prop.type_)

    if wrap_optional and prop.optional:
      cpp_type = 'scoped_ptr<%s> ' % cpp_type
    if pad_for_generics:
      return cpp_type
    return cpp_type.strip()

  def GenerateForwardDeclarations(self):
    """Returns the forward declarations for self._namespace.

    Use after GetRootNamespaceStart. Assumes all namespaces are relative to
    self._root_namespace.
    """
    c = Code()
    for namespace, types in sorted(self._NamespaceTypeDependencies().items()):
      c.Append('namespace %s {' % namespace.name)
      for type_ in types:
        c.Append('struct %s;' % type_)
      c.Append('}')
    return c

  def GenerateIncludes(self):
    """Returns the #include lines for self._namespace.
    """
    c = Code()
    for dependency in sorted(self._NamespaceTypeDependencies().keys()):
      c.Append('#include "%s/%s.h"' % (
            dependency.source_file_dir,
            self._cpp_namespaces[dependency]))
    return c

  def _NamespaceTypeDependencies(self):
    """Returns a dict containing a mapping of model.Namespace to the C++ type
    of type dependencies for self._namespace.
    """
    dependencies = set()
    for function in self._namespace.functions.values():
      for param in function.params:
        dependencies |= self._PropertyTypeDependencies(param)
      for param in function.callback.params:
        dependencies |= self._PropertyTypeDependencies(param)
    for type_ in self._namespace.types.values():
      for prop in type_.properties.values():
        dependencies |= self._PropertyTypeDependencies(prop)

    dependency_namespaces = dict()
    for dependency in dependencies:
      namespace = self._type_namespaces[dependency]
      if namespace != self._namespace:
        dependency_namespaces.setdefault(namespace, [])
        dependency_namespaces[namespace].append(dependency)
    return dependency_namespaces

  def _PropertyTypeDependencies(self, prop):
    """Recursively gets all the type dependencies of a property.
    """
    deps = set()
    if prop:
      if prop.type_ == PropertyType.REF:
        deps.add(prop.ref_type)
      elif prop.type_ == PropertyType.ARRAY:
        deps = self._PropertyTypeDependencies(prop.item_type)
      elif prop.type_ == PropertyType.OBJECT:
        for p in prop.properties.values():
          deps |= self._PropertyTypeDependencies(p)
    return deps
